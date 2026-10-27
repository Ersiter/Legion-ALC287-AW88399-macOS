# 0.1.1-rc.1 — dedicated Legion layout 99

Updates the pinned AppleALC dependency to `a9b406e7322c878d488cdc847a5843911dadc344`, submitted as [AppleALC PR #975](https://github.com/acidanthera/AppleALC/pull/975). The Legion shared-DAC speaker configuration now uses dedicated **layout 99**, preserving upstream layout 13 for other machines.

When upgrading, change HDEF `layout-id` and `alc-layout-id` to Data `63000000` (99), and replace any conflicting `alcid=13` override with 99. Keep `alcverbs=1` enabled. The supplied OpenCore fragment and installation guide use the new layout.

The package also includes the optional pinned firmware download/import helper:

```sh
python3 tools/import_firmware.py --download Kexts/AW88399Driver.kext
```

The helper downloads and verifies the qualified ACF on the user's computer. CI and release archives contain no amplifier firmware. Existing local-file import remains available. Keep any firmware-populated bundle private.

AW88399Driver, Lilu, VoodooI2C and VoodooInput source baselines are unchanged. No HDMI/NVIDIA driver is included. Tahoe's AppleHDA restoration remains a separate prerequisite.

The previous layout-13 implementation had owner-confirmed tweeter/woofer output, microphone, volume/mute and resolution of intermittent/raised-pitch playback. The dedicated layout-99 resource registration and public firmware delivery variant have not yet been boot-qualified. This is a prerelease; a successful CI build is not a hardware playback result.

See docs/INSTALL.md for setup and rollback.
