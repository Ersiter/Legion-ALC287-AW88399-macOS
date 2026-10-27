# Upstream contribution plan

Keep the working source and matched release package available before proposing upstream changes.

## AppleALC

Submitted [PR #975](https://github.com/acidanthera/AppleALC/pull/975): `ALC287: add layout 99 for Lenovo Legion IAX10H shared-DAC speakers`.

Base: `acidanthera/AppleALC` commit `ff01db21749271916d44605503499ceaa61d94b2` (version 1.9.8). Fork commit: `a9b406e7322c878d488cdc847a5843911dadc344`, branch `feature/alc287-legion-shared-dac`.

The PR adds dedicated layout/platform 99 and pin defaults for this board, preserving existing layout 13 and every previous pin entry. Its shared-DAC control hook is restricted to HDEF, 7F50, ALC287 and requested layout 99. The existing `alcverbs=1` gate remains required. One stereo path avoids AppleHDA's four-channel allocation for a duplicated converter; pin14 controls follow the primary pin17 path.

The earlier layout-13 implementation has owner-confirmed playback results. Dedicated layout 99 requires separate boot qualification. Submission is not upstream acceptance; check the PR for review and CI status.

## VoodooI2C

Base: `5c0c99cc451b03790c684ce6552c3f61620ad46b` (2.9.1). The patch is in `patches/VoodooI2C/`. It changes parser resource storage, per-resource nub enumeration, nub address initialization, and the 7F4C–7F4F controller personality. A proposal should retain the existing single-resource behavior and document SPKR's two resources. The controller ID addition can be reviewed separately from the parser/API change.

Suggested title: `Enumerate multiple I2C Serial Bus resources for ACPI devices`.

## AW88399Driver

Retain Linux/AWINIC source provenance and licenses. The timer-based macOS lifecycle and personality-based firmware delivery are implementation differences from Linux; they must be described explicitly. HDMI/NVIDIA changes are not included in these proposals.

The VoodooI2C and AW88399 items remain preparation notes, not submitted PRs. No upstream acceptance is claimed.
