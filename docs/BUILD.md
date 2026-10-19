# Reproducible build inputs

Use an Intel macOS host with full Xcode selected, Command Line Tools, Git, and Python 3. The successful embedded-firmware baseline used Xcode 26.4 / SDK 26.4 on macOS 26.4 (25E246); the target machine runs 26.7 (25G229). The project builds x86_64 kernel bundles with deployment target 26.0 and a pinned MacKernelSDK; no separate KDK download is required.

```sh
git clone https://github.com/Ersiter/Legion-ALC287-AW88399.git
cd Legion-ALC287-AW88399
python3 tools/build.py > build.log 2>&1
```

`dependencies.lock.json` locks AppleALC, Lilu, MacKernelSDK, VoodooI2C, and VoodooInput by full commit. VoodooI2C's submodules use the recursive gitlinks recorded by its locked commit. The local patch adds dual-resource enumeration and 7F4C–7F4F matching. The build applies it only to that baseline.

The script builds Lilu Debug for the AppleALC development interface, Lilu Release for deployment, the forked AppleALC, VoodooInput Debug/Release, VoodooI2C and VoodooI2CHID, then AW88399Driver. Products are ad-hoc signed and verified. `dist/` must not already exist: use a fresh checkout or move your earlier output aside, especially if it contains imported firmware.

Output:

- `.build/Legion-ALC287-AW88399-0.1.0.zip`: firmware-free matching kext set and installation materials.
- `dist/build-manifest.json`: project/dependency commits, Xcode/SDK versions, and file hashes.
- `dist/SHA256SUMS`: package-content hashes before local firmware import.

CI uses `macos-26-intel` and the same script. Runner images can change; the manifest records the actual selected toolchain. Fixed source inputs provide repeatable builds, not a promise that timestamps, UUIDs, signatures, and bytes match across different Xcode installations. A release archive's outer SHA-256 identifies its exact published bytes.

Tag workflows publish only after the suite build succeeds. Public builds have no amplifier firmware, personal EFI, full hardware logs, or Apple payloads. Firmware import happens on the owner's machine after download. A new CI binary does not inherit hardware qualification merely by compiling.
