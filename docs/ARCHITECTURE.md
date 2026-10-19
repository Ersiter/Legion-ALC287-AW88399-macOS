# Shared stereo DAC and amplifier control

## Observed regression

The initial dual-path resource described pin14→DAC02 and pin17→DAC02 inside one speaker group. The two Amp channel declarations both named left/right, but AppleHDA's PathSet accumulated the path channel counts instead of deduplicating converter NID02. The captured HAL format was 48 kHz, 24-bit stereo with a 512-frame buffer. Hardware GET replies showed DAC02 format `0x33` (four channels) and stream assignment `0x12` (tag1, channel offset2).

That mismatch is direct hardware/configuration evidence. A 44.1 kHz song played through a 48 kHz output is normally a resampling case and did not, by itself, establish a buffer-size problem. No buffer or fixed-rate workaround was added. The owner subsequently reported the raised pitch and discontinuities resolved with the shared-DAC correction.

## Current routing

AppleHDA sees one complete stereo software path, pin17→DAC02, including the working Amp volume/mute metadata. The existing AppleALC `IOHDACodecDevice::executeVerb` hook mirrors successful pin17 output control, output amplifier, and power writes to pin14. It leaves DAC02's format and stream assignment to AppleHDA. The hook requires `alcverbs=1`.

When enabling pin17, the mirror powers pin14, reads its actual connection list, finds the DAC02 selector, sets that selector and the original EAPD value, copies the primary pin's current left/right output amp state, then enables pin14. It does not guess selector zero, force unmute, change gain, or allocate another stream. Unsupported connection lists or failed verbs return an error rather than silently reporting success. Calls use the original trampoline, avoiding recursive hook entry.

The guard requires ALC287 vendor ID 0x10ec0287, HDEF, Intel controller 0x7f50, and `alc-layout-id=13`. AppleALC may change the native `layout-id` internally, so that property is not used as the requested-layout guard. Other pins, DAC format verbs, GET queries, microphone/headphone routing, and NVIDIA codecs pass through unchanged.

Both internal-speaker pin defaults remain `0x90170110`; the pin14 EAPD initialization is retained. `ALC287SharedDAC*` IORegistry properties expose the selected pins, discovered selector, last error, and output-enable state.

## Amplifiers and I2C

VoodooI2C originally retained one I2C Serial Bus resource per ACPI device. The SPKR device has resources at 0x34 and 0x35. The patch retains up to eight resources and publishes a nub for each resource; overflow is rejected. It also retains the local masked controller match `0x7f4c8086&0xFFFCFFFF` for 7F4C–7F4F, with the existing controller timing properties.

AW88399Driver binds to those SPKR nubs, validates the two addresses and chip ID, and uses the retained Linux parser/device core to configure the amplifiers. Channel reversal and the board's BSTS quirk remain unchanged. Calibration failures propagate instead of proceeding with invalid values. The output engine is restricted to an ALC287 ancestor; engine state is read as an IORegistry property, avoiding an IOAudioFamily class-link dependency before SimpleHDA has restored that family.

The public packaging reads `AW88399FirmwareData` as OSData from the driver personality. The local import tool installs the exact qualified bytes there. The previous successful build embedded the same bytes in a generated header. The ACF parser, firmware content, channel mapping, protection and amplifier register sequences are unchanged by this delivery change.

## Linux basis and limits

The [Legion Linux audio work](https://github.com/marco-giunta/legion-pro7-gen10-audio/tree/b1690404e65796a3551e820a252e9938df97dce9) supplies the AW88399 side-codec and board reference. Linux restricts the woofer pin17 path to DAC02 and supports sending one stereo stream to both speaker pairs. AppleHDA has a different channel-allocation model, so copying two Linux paths into AppleALC resource XML was insufficient.

The macOS amplifier lifecycle still polls engine state at 20 ms and searches at 100 ms. It is not a synchronous ALSA PCM hook, does not prove hardware crossover topology, and does not establish untested wake or headphone behavior. DSP protection is retained; no gain boost or protection bypass is part of this release.
