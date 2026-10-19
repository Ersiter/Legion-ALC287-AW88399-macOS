# Upstream contribution plan

Keep the working source and matched release package available before proposing upstream changes.

## AppleALC

Base: `acidanthera/AppleALC` commit `c44a22fb41876e9c19a93e89454e2efa18006735` (version 1.9.8). Fork change: `5f2c884f52384ba6a12485f08ead75ebdb7faf5a`.

The fork contains three file changes: the shared-DAC control hook, one stereo platform path with complete Amp metadata, and the corresponding layout13 pin defaults. A proposal must explain why two software paths caused a four-channel converter assignment, and why copying identical left/right bindings did not deduplicate the DAC. Include sanitized HAL/HDA format values and owner feedback; do not attach complete IORegistry logs or private identifiers.

Before an upstream PR, discuss board isolation and the layout13 scope with maintainers: the hook is guarded by HDEF/controller/codec/requested-layout, while the resource change affects other ALC287 users selecting layout13. Upstream may prefer a new board-specific layout. Do not claim this fork's layout13 is a universal replacement. The existing `alcverbs=1` gate is also an upstream integration consideration.

Suggested title: `ALC287: keep Legion dual speakers on one stereo converter`.

## VoodooI2C

Base: `5c0c99cc451b03790c684ce6552c3f61620ad46b` (2.9.1). The patch is in `patches/VoodooI2C/`. It changes parser resource storage, per-resource nub enumeration, nub address initialization, and the 7F4C–7F4F controller personality. A proposal should retain the existing single-resource behavior and document SPKR's two resources. The controller ID addition can be reviewed separately from the parser/API change.

Suggested title: `Enumerate multiple I2C Serial Bus resources for ACPI devices`.

## AW88399Driver

Retain Linux/AWINIC source provenance and licenses. The timer-based macOS lifecycle and personality-based firmware delivery are implementation differences from Linux; they must be described explicitly. HDMI/NVIDIA changes are not included in these proposals.

These are preparation notes, not submitted PRs or upstream acceptance claims.
