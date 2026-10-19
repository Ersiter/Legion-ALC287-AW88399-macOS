/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef AW88399_DRIVER_HPP
#define AW88399_DRIVER_HPP

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOService.h>
#include <IOKit/audio/IOAudioTypes.h>
#include <libkern/c++/OSData.h>

#include "AW88399Compat.h"
#include "vendor/aw88399/aw88399-common.h"

class VoodooI2CDeviceNub;

class AW88399Driver : public IOService {
    OSDeclareDefaultStructors(AW88399Driver)

    VoodooI2CDeviceNub *nub;
    IOWorkLoop *workLoop;
    IOCommandGate *commandGate;
    IOTimerEventSource *timer;
    IOService *audioEngine;
    struct i2c_adapter i2cAdapter;
    struct i2c_client i2cClient;
    struct regmap registerMap;
    struct aw88399 amplifier;
    bool initialized;
    bool amplifierRunning;
    bool playbackSessionActive;
    bool stopping;
    uint32_t lastError;
    uint64_t audioEngineRegistryID;

    static IOReturn loadFirmwareAction(OSObject *owner, void *data,
                                       void *arg2, void *arg3, void *arg4);
    static IOReturn stopAmplifierAction(OSObject *owner, void *arg1,
                                        void *arg2, void *arg3, void *arg4);
    static void timerFired(OSObject *owner, IOTimerEventSource *sender);

    bool findAudioEngine();
    IOReturn loadFirmware(OSData *data);
    void updatePlaybackState();
    void publishDiagnostics();
    void releaseResources();

public:
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
};

#endif
