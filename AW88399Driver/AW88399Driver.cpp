/* SPDX-License-Identifier: GPL-2.0-only */
#include <IOKit/IOLib.h>
#include <libkern/c++/OSString.h>

#include "AW88399Driver.hpp"
#include "AW88399I2CProtocol.h"
#include "VoodooI2CDeviceNubAPI.hpp"
#include "vendor/aw88395/aw88395_device.h"

OSDefineMetaClassAndStructors(AW88399Driver, IOService);

static const UInt32 kAudioEnginePollIntervalMs = 20;
static const UInt32 kAudioEngineSearchIntervalMs = 100;

static UInt32 audioEngineState(IOService *engine) {
    if (engine == nullptr)
        return kIOAudioEngineStopped;
    OSObject *property = engine->copyProperty("IOAudioEngineState");
    OSNumber *state = OSDynamicCast(OSNumber, property);
    UInt32 value = state == nullptr ? kIOAudioEngineStopped : state->unsigned32BitValue();
    OSSafeReleaseNULL(property);
    return value;
}

static int awI2CWriteRead(void *context,
                          const uint8_t *writeBytes,
                          uint16_t writeLength,
                          uint8_t *readBytes,
                          uint16_t readLength) {
    VoodooI2CDeviceNub *nub = static_cast<VoodooI2CDeviceNub *>(context);
    UInt8 registerAddress;

    if (nub == nullptr || writeBytes == nullptr || readBytes == nullptr ||
        writeLength != 1 || readLength != 2)
        return kIOReturnBadArgument;

    registerAddress = writeBytes[0];
    return nub->writeReadI2C(&registerAddress, 1, readBytes, 2);
}

static int awI2CWrite(void *context, const uint8_t *bytes, uint16_t length) {
    VoodooI2CDeviceNub *nub = static_cast<VoodooI2CDeviceNub *>(context);
    UInt8 message[3];

    if (nub == nullptr || bytes == nullptr || length != sizeof(message))
        return kIOReturnBadArgument;

    message[0] = bytes[0];
    message[1] = bytes[1];
    message[2] = bytes[2];
    return nub->writeI2C(message, sizeof(message));
}

static int awRegmapRead(void *context, u8 reg, u16 *value) {
    aw88399_i2c_transport transport = {context, awI2CWriteRead, awI2CWrite};
    return aw88399_read_register(&transport, reg, value);
}

static int awRegmapWrite(void *context, u8 reg, u16 value) {
    aw88399_i2c_transport transport = {context, awI2CWriteRead, awI2CWrite};
    return aw88399_write_register(&transport, reg, value);
}

static int awRegmapRawWrite(void *context, u8 reg, const u8 *data, u16 length) {
    VoodooI2CDeviceNub *nub = static_cast<VoodooI2CDeviceNub *>(context);
    if (nub == nullptr || data == nullptr || length == 0 || length == UINT16_MAX)
        return kIOReturnBadArgument;

    UInt16 messageLength = static_cast<UInt16>(length + 1);
    UInt8 *message = static_cast<UInt8 *>(IOMalloc(messageLength));
    if (message == nullptr)
        return kIOReturnNoMemory;

    message[0] = reg;
    memcpy(message + 1, data, length);
    IOReturn result = nub->writeI2C(message, messageLength);
    IOFree(message, messageLength);
    return result;
}

bool AW88399Driver::start(IOService *provider) {
    if (!IOService::start(provider))
        return false;

    nub = nullptr;
    workLoop = nullptr;
    commandGate = nullptr;
    timer = nullptr;
    audioEngine = nullptr;
    initialized = false;
    amplifierRunning = false;
    playbackSessionActive = false;
    stopping = false;
    lastError = kIOReturnNotReady;
    audioEngineRegistryID = 0;
    bzero(&i2cAdapter, sizeof(i2cAdapter));
    i2cAdapter.nr = -1;
    bzero(&i2cClient, sizeof(i2cClient));
    i2cClient.adapter = &i2cAdapter;
    bzero(&registerMap, sizeof(registerMap));
    bzero(&amplifier, sizeof(amplifier));

    nub = OSDynamicCast(VoodooI2CDeviceNub, provider);
    if (nub == nullptr)
        return false;

    OSNumber *address = OSDynamicCast(OSNumber, nub->getProperty("i2cAddress"));
    if (address != nullptr)
        i2cClient.addr = address->unsigned8BitValue();
    if (address == nullptr ||
        (i2cClient.addr != 0x34 && i2cClient.addr != 0x35)) {
        IOLog("AW88399: refusing unexpected I2C address\n");
        lastError = kIOReturnBadArgument;
        publishDiagnostics();
        return false;
    }

    publishDiagnostics();
    registerMap.context = nub;
    registerMap.read = awRegmapRead;
    registerMap.write = awRegmapWrite;
    registerMap.raw_write = awRegmapRawWrite;
    int result = aw88399_init(&amplifier, &i2cClient, &registerMap);
    if (result != 0) {
        IOLog("AW88399: chip initialization failed (%d)\n", result);
        lastError = static_cast<uint32_t>(result);
        publishDiagnostics();
        releaseResources();
        return false;
    }

    int rawChannel = i2cClient.addr - 0x34;
    aw88399_dev_set_channel(&amplifier, 1 - rawChannel);
    amplifier.bsts_unreliable = true;

    OSNumber *chipID = OSNumber::withNumber(amplifier.aw_pa->chip_id, 16);
    if (chipID == nullptr) {
        lastError = kIOReturnNoMemory;
        publishDiagnostics();
        releaseResources();
        return false;
    }
    setProperty("AW88399ChipID", chipID);
    chipID->release();

    workLoop = IOWorkLoop::workLoop();
    if (workLoop == nullptr) {
        lastError = kIOReturnNoMemory;
        publishDiagnostics();
        releaseResources();
        return false;
    }

    commandGate = IOCommandGate::commandGate(this);
    if (commandGate == nullptr || workLoop->addEventSource(commandGate) != kIOReturnSuccess) {
        lastError = kIOReturnNoResources;
        publishDiagnostics();
        releaseResources();
        return false;
    }

    timer = IOTimerEventSource::timerEventSource(this, timerFired);
    if (timer == nullptr || workLoop->addEventSource(timer) != kIOReturnSuccess) {
        lastError = kIOReturnNoResources;
        publishDiagnostics();
        releaseResources();
        return false;
    }

    OSData *firmware = OSDynamicCast(OSData, getProperty("AW88399FirmwareData"));
    if (firmware == nullptr) {
        IOLog("AW88399: import the machine ACF into this kext before installation\n");
        lastError = kIOReturnNotFound;
        publishDiagnostics();
        releaseResources();
        return false;
    }

    firmware->retain();
    IOReturn firmwareResult = commandGate->runAction(
        loadFirmwareAction, firmware, nullptr, nullptr, nullptr);
    firmware->release();
    if (firmwareResult != kIOReturnSuccess) {
        IOLog("AW88399: synchronous ACF/DSP initialization failed (%d)\n",
              firmwareResult);
        lastError = firmwareResult;
        publishDiagnostics();
        releaseResources();
        return false;
    }

    registerService();
    return true;
}

IOReturn AW88399Driver::loadFirmware(OSData *data) {
    if (data == nullptr || stopping)
        return kIOReturnBadArgument;

    int result = aw88399_load_firmware_data(
        &amplifier, static_cast<const u8 *>(data->getBytesNoCopy()), data->getLength());
    if (result != 0) {
        IOLog("AW88399: ACF/DSP initialization failed (%d)\n", result);
        lastError = static_cast<uint32_t>(result);
        publishDiagnostics();
        return kIOReturnError;
    }

    initialized = true;
    lastError = kIOReturnSuccess;
    findAudioEngine();
    publishDiagnostics();
    timer->setTimeoutMS(audioEngine == nullptr ?
                        kAudioEngineSearchIntervalMs : kAudioEnginePollIntervalMs);
    return kIOReturnSuccess;
}

IOReturn AW88399Driver::loadFirmwareAction(OSObject *owner, void *data,
                                          void *arg2, void *arg3, void *arg4) {
    (void)arg2;
    (void)arg3;
    (void)arg4;
    AW88399Driver *service = OSDynamicCast(AW88399Driver, owner);
    return service == nullptr ? kIOReturnBadArgument :
                               service->loadFirmware(static_cast<OSData *>(data));
}

IOReturn AW88399Driver::stopAmplifierAction(OSObject *owner, void *arg1,
                                           void *arg2, void *arg3, void *arg4) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    AW88399Driver *service = OSDynamicCast(AW88399Driver, owner);
    if (service == nullptr || service->amplifier.aw_pa == nullptr)
        return kIOReturnBadArgument;

    return aw88399_stop(service->amplifier.aw_pa) == 0 ?
           kIOReturnSuccess : kIOReturnError;
}

bool AW88399Driver::findAudioEngine() {
    OSDictionary *matching = IOService::serviceMatching("IOAudioEngine");
    if (matching == nullptr)
        return false;

    OSIterator *iterator = IOService::getMatchingServices(matching);
    matching->release();
    if (iterator == nullptr)
        return false;

    IOService *service = nullptr;
    while ((service = OSDynamicCast(IOService, iterator->getNextObject())) != nullptr) {
        IOService *engine = service;
        const char *name = engine == nullptr ? nullptr : engine->getName();
        bool alc287Codec = false;
        for (IORegistryEntry *entry = engine; entry != nullptr;
             entry = entry->getParentEntry(gIOServicePlane)) {
            OSNumber *vendorID = OSDynamicCast(
                OSNumber, entry->getProperty("IOHDACodecVendorID"));
            if (vendorID != nullptr && vendorID->unsigned32BitValue() == 0x10ec0287) {
                alc287Codec = true;
                break;
            }
        }
        if (name != nullptr && !engine->isInactive() && alc287Codec &&
            strncmp(name, "AppleHDAEngineOutput",
                    sizeof("AppleHDAEngineOutput") - 1) == 0) {
            audioEngine = engine;
            audioEngine->retain();
            audioEngineRegistryID = engine->getRegistryEntryID();
            iterator->release();
            return true;
        }
    }

    iterator->release();
    return false;
}

void AW88399Driver::updatePlaybackState() {
    if (!initialized || stopping)
        return;

    bool oldPlaybackSessionActive = playbackSessionActive;
    bool oldAmplifierRunning = amplifierRunning;
    uint32_t oldError = lastError;
    uint64_t oldEngineID = audioEngineRegistryID;
    uint32_t oldEngineState = audioEngineState(audioEngine);
    bool oldHadEngine = audioEngine != nullptr;

    if (audioEngine != nullptr && audioEngine->isInactive()) {
        if (playbackSessionActive || amplifierRunning) {
            int result = aw88399_stop(amplifier.aw_pa);
            if (result != 0) {
                lastError = static_cast<uint32_t>(result);
                IOLog("AW88399: inactive engine stop failed (%d)\n", result);
            }
        }
        playbackSessionActive = false;
        amplifierRunning = false;
        audioEngine->release();
        audioEngine = nullptr;
        audioEngineRegistryID = 0;
    }

    if (audioEngine == nullptr)
        findAudioEngine();

    bool isRunning = audioEngine != nullptr &&
                     audioEngineState(audioEngine) == kIOAudioEngineRunning;
    if (isRunning && !playbackSessionActive) {
        playbackSessionActive = true;
        aw88399_start(&amplifier, AW88399_SYNC_START);
        amplifierRunning = amplifier.aw_pa->status == AW88399_DEV_PW_ON;
        if (!amplifierRunning) {
            lastError = kIOReturnError;
            IOLog("AW88399: playback start did not power the amplifier\n");
        } else {
            lastError = kIOReturnSuccess;
        }
    } else if (!isRunning && playbackSessionActive) {
        playbackSessionActive = false;
        if (amplifierRunning) {
            int result = aw88399_stop(amplifier.aw_pa);
            amplifierRunning = false;
            if (result != 0) {
                lastError = static_cast<uint32_t>(result);
                IOLog("AW88399: playback stop failed (%d)\n", result);
            }
        }
    }
    bool hasEngine = audioEngine != nullptr;
    uint32_t engineState = audioEngineState(audioEngine);
    bool engineChanged = oldHadEngine != hasEngine ||
                         oldEngineID != audioEngineRegistryID ||
                         (hasEngine && oldEngineState != engineState);
    if (engineChanged || oldPlaybackSessionActive != playbackSessionActive ||
        oldAmplifierRunning != amplifierRunning || oldError != lastError) {
        publishDiagnostics();
    }
}

void AW88399Driver::publishDiagnostics() {
    setProperty("AW88399I2CAddress", i2cClient.addr, 8);
    setProperty("AW88399Ready", initialized ? kOSBooleanTrue : kOSBooleanFalse);
    setProperty("AW88399PlaybackActive", playbackSessionActive ?
                kOSBooleanTrue : kOSBooleanFalse);
    setProperty("AW88399AmplifierRunning", amplifierRunning ?
                kOSBooleanTrue : kOSBooleanFalse);
    setProperty("AW88399LastError", lastError, 32);

    if (audioEngine != nullptr) {
        const char *name = audioEngine->getName();
        OSString *engineName = name == nullptr ? nullptr :
                               OSString::withCString(name);
        if (engineName != nullptr) {
            setProperty("AW88399AudioEngineName", engineName);
            engineName->release();
        }
        setProperty("AW88399AudioEngineRegistryID", audioEngineRegistryID, 64);
        setProperty("AW88399AudioEngineState", audioEngineState(audioEngine), 32);
    } else {
        removeProperty("AW88399AudioEngineName");
        removeProperty("AW88399AudioEngineRegistryID");
        removeProperty("AW88399AudioEngineState");
    }

    if (nub != nullptr) {
        if (lastError != kIOReturnSuccess) {
            nub->setProperty("AW88399LastError", lastError, 32);
            nub->setProperty("AW88399I2CAddress", i2cClient.addr, 8);
        } else {
            nub->removeProperty("AW88399LastError");
            nub->removeProperty("AW88399I2CAddress");
        }
    }
}

void AW88399Driver::timerFired(OSObject *owner, IOTimerEventSource *sender) {
    AW88399Driver *service = OSDynamicCast(AW88399Driver, owner);
    if (service == nullptr || sender == nullptr || service->stopping)
        return;

    service->updatePlaybackState();
    sender->setTimeoutMS(service->audioEngine == nullptr ?
                         kAudioEngineSearchIntervalMs : kAudioEnginePollIntervalMs);
}

void AW88399Driver::releaseResources() {
    stopping = true;

    if (timer != nullptr) {
        timer->cancelTimeout();
        if (workLoop != nullptr)
            workLoop->removeEventSource(timer);
        OSSafeReleaseNULL(timer);
    }

    if (amplifierRunning && amplifier.aw_pa != nullptr && commandGate != nullptr)
        commandGate->runAction(stopAmplifierAction, nullptr, nullptr, nullptr, nullptr);
    amplifierRunning = false;
    playbackSessionActive = false;

    OSSafeReleaseNULL(audioEngine);
    if (commandGate != nullptr) {
        commandGate->disable();
        if (workLoop != nullptr)
            workLoop->removeEventSource(commandGate);
        OSSafeReleaseNULL(commandGate);
    }
    OSSafeReleaseNULL(workLoop);

    aw_devm_release(&i2cClient.dev);
    amplifier.aw_pa = nullptr;
    initialized = false;
}

void AW88399Driver::stop(IOService *provider) {
    releaseResources();
    IOService::stop(provider);
}
