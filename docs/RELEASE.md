# 0.1.0 — initial macOS audio suite

Includes AW88399Driver source and a matched build of Lilu, the shared-DAC AppleALC fork, VoodooI2C with dual-resource and 7F4C–7F4F matching, VoodooInput plug-in, and VoodooI2CHID. Dependencies are pinned by commit; the package includes hashes and build metadata.

The qualified embedded-firmware baseline on Legion Y9000P IAX10H (83F5), macOS 26.7, has owner-confirmed tweeter/woofer output, internal microphone, volume and mute. Raised pitch and intermittent playback were reported resolved after the shared-DAC correction.

This public package uses local ACF import into the driver personality; its hardware boot qualification remains pending. Import the exact qualified firmware before installation. No vendor firmware, Apple system drivers, complete EFI, or personal logs are included. SimpleHDA/AppleHDA restoration must be obtained separately. HDMI is outside this release.

Read README.md and docs/INSTALL.md before changing an EFI. The CI package is a prerelease until the public firmware delivery variant is confirmed on hardware.
