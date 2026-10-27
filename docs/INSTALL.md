# Installation and rollback

## Before installation

Use the exact hardware and software stack in [COMPATIBILITY.md](COMPATIBILITY.md). Start with an existing bootable EFI. Preserve your currently working audio and input-device files and configuration so they can be restored together. This project does not partition disks, modify the system volume, rebuild a kernel collection, or supply GPU drivers.

On Tahoe, obtain the AppleHDA restoration stack separately. The qualified setup uses [SimpleHDA 1.0.0](https://github.com/laobamac/SimpleHDA/releases/tag/v1.0.0), with AppleHDA/AppleHDAController 600.2 and IOAudioFamily 740.1. Follow its installation and platform requirements. This release neither contains nor installs Apple system drivers. Other AppleHDA restoration methods have not been qualified here.

## Populate the amplifier firmware

Run this inside the extracted package:

```sh
python3 tools/import_firmware.py /path/to/aw88399_acf.bin Kexts/AW88399Driver.kext
```

Or download and import the pinned third-party copy locally:

```sh
python3 tools/import_firmware.py --download Kexts/AW88399Driver.kext
```

See [FIRMWARE.md](FIRMWARE.md) for its source and redistribution status. This command does not install the bundle into an EFI.

The input must be 152,416 bytes with SHA-256 `1e927c9bca76d868181c0f81df2bccef3cf19c7d0910219f229360c87babd42c`. On macOS the tool ad-hoc signs and verifies the changed bundle. Without imported firmware the amplifier driver refuses to initialize. Do not upload the modified bundle.

## OpenCore changes

Copy the package kexts into the intended EFI's `EFI/OC/Kexts`. Replace an existing matching bundle rather than keeping a second copy. Keep your unrelated EFI entries. Use `examples/OpenCore-audio.plist` as a fragment and merge its audio entries into your own config; it is not a bootable config.

Use this relative dependency order, with all required plug-ins explicitly enabled:

1. `Lilu.kext`
2. `AppleALC.kext`
3. Your separately obtained `SimpleHDA.kext`
4. `VoodooI2C.kext/Contents/PlugIns/VoodooI2CServices.kext`
5. `VoodooI2C.kext/Contents/PlugIns/VoodooGPIO.kext`
6. `VoodooI2C.kext`
7. `VoodooI2C.kext/Contents/PlugIns/VoodooInput.kext`
8. `AW88399Driver.kext`
9. `VoodooI2CHID.kext`

The I2C controller and GPIO components use the same patched source baseline. Preserve exactly one active VoodooInput instance, including when VoodooPS2Controller also contains one. Preserve the 7F4C–7F4F controller match; replacing only the controller with an upstream binary can break this board's touchpad and amplifier enumeration.

For this board the real root bridge UID is 2; its HDEF property path is:

```text
PciRoot(0x2)/Pci(0x1f,0x3)
  layout-id:     Data 63000000
  alc-layout-id: Data 63000000
```

For version 0.1.1, select layout 99; do not retain an `alcid=13` override from the previous board-specific release. The root UID is not PCI bus number 0x80. A key prefixed with `#` is ignored by OpenCore. Merge `keepsyms=1 alcverbs=1 -vi2c-force-polling` into existing boot arguments without dropping unrelated requirements. `alcverbs=1` enables the existing AppleALC verb hook used by this fork; it is required for the shared speaker control. Do not add a conflicting `alcid` override. Limit the audio entries to Darwin 25.x as shown in the fragment.

Validate the merged configuration with the matching OpenCore `ocvalidate`, then boot that EFI. A successful file copy or config validation is not proof that the intended EFI was booted.

## Runtime checks

Confirm both speaker ranges, microphone recording, volume, mute, and ordinary play/pause/seek. In IORegistry, the ALC287 codec should expose `ALC287SharedDACPrimaryPin=23`, `ALC287SharedDACSecondaryPin=20`, `ALC287SharedDACLastError=0`, and a discovered secondary selector. Both AW services should report ready and no error. While playing 48 kHz / 24-bit stereo, DAC02 should use format `0x31`, with channel offset 0; stream tag 1 therefore reads `0x10`.

These values are diagnostic expectations, not a replacement for listening. The amplifier state follows the ALC287 output engine through a timer; this port does not claim sample-accurate Linux PREPARE/CLEANUP timing.

## Rollback

Boot your preserved working EFI, or restore its matching AppleALC, AW88399Driver, VoodooI2C bundle/plug-ins, VoodooI2CHID, and associated config entries together. Restore the prior boot arguments and HDEF properties if you changed them. Do not remove I2C components still required by the touchpad. No system-volume or kernel-collection rollback is required by this package itself.
