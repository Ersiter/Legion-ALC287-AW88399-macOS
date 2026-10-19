# Third-party source and licenses

This suite combines separately licensed kernel bundles and source patches. The top-level AW driver license does not replace the licenses of AppleALC, Lilu, VoodooI2C, VoodooGPIO, VoodooInput, or VoodooI2CHID. Their full corresponding source is available through the commits in dependencies.lock.json and the recorded submodule gitlinks; the modified I2C source is reproducible by applying the included patch.

| Material | Source / revision | License |
| --- | --- | --- |
| AW88399 common core | marco-giunta/legion-pro7-gen10-audio, `b1690404e65796a3551e820a252e9938df97dce9` | GPL-2.0-only; AWINIC notices retained |
| AW88395 parser/device core | torvalds/linux, `58161ead29e50d70a317ea26b472e0c7508e69da`, sound/soc/codecs/aw88395 | GPL-2.0-only; AWINIC notices retained |
| macOS AW compatibility and IOKit adapter | AW88399Driver/ | GPL-2.0-only |
| AppleALC | Locked fork and upstream base in dependencies.lock.json | BSD-3-Clause |
| Lilu | Locked source in dependencies.lock.json | BSD-3-Clause |
| MacKernelSDK | Locked source in dependencies.lock.json | Upstream notices apply; build dependency, no Apple runtime driver shipped |
| VoodooI2C and local patch | Locked source plus patches/VoodooI2C | GPL-3.0, as supplied upstream |
| VoodooGPIO / HID satellites | Recursive gitlinks of locked VoodooI2C | Their respective upstream licenses |
| VoodooInput | Locked source in dependencies.lock.json | GPL-2.0 |

The vendored AW source modifies Linux includes/API calls for the macOS compatibility layer and preserves the original firmware parser and protection sequences. The AW88399 calibration path propagates failed reads/writes. The public package replaces the private embedded ACF with an owner-populated personality property; no firmware byte array is vendored.

SimpleHDA is an externally obtained prerequisite, not a release component. AppleHDA and IOAudioFamily are Apple system software and are not distributed here. Firmware redistribution permission has not been established; both the raw ACF and bundles populated with it remain local.
