# Vendored Linux core

AW88399 core: marco-giunta/legion-pro7-gen10-audio at
`b1690404e65796a3551e820a252e9938df97dce9` (legion_audio).
AW88395 parser/device core: torvalds/linux at
`58161ead29e50d70a317ea26b472e0c7508e69da`, sound/soc/codecs/aw88395/.

All copied SPDX and AWINIC copyright notices are retained (GPL-2.0-only).
Includes and Linux APIs are adapted through AW88399Compat. The calibration
path returns read/write failures. Firmware, gains and protection parameters
are not replaced by this port. See ../../THIRD_PARTY.md for component notices.

The vendor ACF is not present. Its redistribution terms have not been verified.
