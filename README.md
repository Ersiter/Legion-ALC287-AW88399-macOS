# Legion ALC287 + AW88399

macOS speaker support for the Lenovo Legion Y9000P IAX10H (83F5), combining an AW88399 amplifier driver, a shared-DAC AppleALC fork, and a VoodooI2C resource patch.

## Installation

This is a board-specific package for x86_64 macOS Tahoe. Read [compatibility](docs/COMPATIBILITY.md) before installing. Releases contain Lilu, patched AppleALC, patched VoodooI2C with its plug-ins, VoodooI2CHID, and AW88399Driver. Apple system drivers and amplifier firmware are excluded.

1. Obtain the qualified ACF from your own machine or driver package, then import it into the extracted release:

   ```sh
   python3 tools/import_firmware.py /path/to/aw88399_acf.bin Kexts/AW88399Driver.kext
   ```

2. Follow [installation and rollback](docs/INSTALL.md). The package supplies a minimal OpenCore fragment, not an EFI replacement. Tahoe also needs a separately obtained AppleHDA restoration stack; the qualified setup uses SimpleHDA 1.0.0.
3. Boot the updated EFI and check normal playback, microphone recording, volume, and mute. Do not install two copies of the same kext.

The import tool verifies the exact firmware SHA-256 and length before changing the bundle. The imported bundle contains your firmware and must remain local. See [firmware handling](docs/FIRMWARE.md).

## What works

The maintainer reports both tweeters and woofers, the internal microphone, volume, and mute working on the qualified machine. The raised-pitch and intermittent-playback regression disappeared after the shared-DAC correction. These are machine-owner observations, not compatibility claims for other Legion models.

The public packaging separates firmware from the executable. Its driver core and register sequences are preserved; this packaging change needs its own hardware boot confirmation. HDMI/NVIDIA audio, other boards, sleep/wake behavior, and the 3.5 mm jack are not qualified by this release.

## Why this package exists

Both speaker branches use DAC02. Declaring two AppleHDA software paths caused AppleHDA to allocate four channels and program the shared converter with a channel offset, while HAL still supplied stereo. The fork declares DAC02 once and synchronizes pin14 controls with the primary pin17 path. [Implementation details](docs/ARCHITECTURE.md) explain the observed mismatch and the control sequence.

AW88399Driver controls the two I2C amplifiers using the retained Linux ACF parser, DSP setup, calibration, protection, and start/stop routines. It does not replace the HDA PCM transport. The VoodooI2C patch enumerates both I2C resources of the SPKR device and retains the 7F4C–7F4F controller match needed by this board.

## Build

On macOS with full Xcode selected:

```sh
git clone https://github.com/Ersiter/Legion-ALC287-AW88399.git
cd Legion-ALC287-AW88399
python3 tools/build.py
```

All source dependencies are pinned in [dependencies.lock.json](dependencies.lock.json), including the [AppleALC fork](https://github.com/Ersiter/AppleALC). CI runs the same command and produces a firmware-free package with build metadata and SHA-256 checksums. See [build instructions](docs/BUILD.md) for toolchain and artifact details.

## License and upstream work

AW88399Driver and its Linux-derived core retain GPL-2.0-only notices. AppleALC and Lilu retain BSD-3-Clause; VoodooI2C has its own GPL license. The package contains separately licensed components: see [third-party notices](THIRD_PARTY.md) and [LICENSES](LICENSES).

No Apple driver binaries, complete personal EFI, machine identifiers, full diagnostic logs, or vendor firmware are published here. [Upstream contribution notes](docs/UPSTREAM.md) separate the board resource change, shared-DAC behavior, and I2C resource patch.
