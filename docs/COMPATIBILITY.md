# Compatibility

## Qualified hardware and stack

| Component | Qualified value |
| --- | --- |
| Machine | Lenovo Legion Y9000P IAX10H, machine type 83F5 |
| Board / chipset | LNVNB161216 / HM870 |
| CPU | Intel Core Ultra 9 275HX |
| Operating system | macOS Tahoe 26.7, build 25G229, Darwin 25, x86_64 |
| HDA controller | Intel 8086:7F50, ACPI HDEF under root bridge UID 2 |
| Codec | Realtek ALC287, vendor/device 10EC:0287 |
| Audio subsystem | 17AA:3907 in the amplifier/board evidence; not inferred from generic AppleALC tables |
| Codec layout | Requested `alc-layout-id=13`; AppleALC may report an internal native layout of 7 |
| Speaker pins | Primary woofer pin17 and tweeter pin14, both connected to DAC02 |
| Amplifiers | Two AW88399, chip ID 0x2183, I2C addresses 0x34 and 0x35 |
| Amplifier ACPI node | `\_SB.PC02.I2C2.SPKR`, HID `AWDZ8399` |
| I2C controllers | Intel 8086:7F4C (I2C0), 8086:7F4E (I2C2) |
| AppleHDA source | Separately installed SimpleHDA 1.0.0 restoration stack; AppleHDA/AppleHDAController 600.2, IOAudioFamily 740.1 |
| Boot arguments relevant to this setup | `keepsyms=1 alcverbs=1 -vi2c-force-polling` |

The board subsystem value above comes from the amplifier investigation. Earlier Windows codec references included 17AA:3906; neither this value nor an AppleALC support-table entry establishes all boards as equivalent. The AppleALC runtime guard uses controller 7F50, codec 10EC0287, HDEF, and requested layout13. AW88399Driver additionally limits I2C addresses and output-engine matching. Do not interpret these guards as automatic board compatibility detection.

## Qualification status

| Function | Status |
| --- | --- |
| Internal tweeters and woofers | Working, reported by the machine owner after the shared-DAC fix |
| Internal microphone | Recording confirmed by the machine owner |
| Volume and mute | Working, reported by the machine owner |
| Raised pitch / intermittent music playback | Resolved, reported by the machine owner after the shared-DAC fix |
| Firmware-in-personality public packaging | Build and import qualification tracked separately; physical boot confirmation pending |
| HDMI / DisplayPort / NVIDIA HDA | Outside this release; no endpoint or playback claim |
| 3.5 mm jack, sleep/wake, long-duration playback | Not qualified |
| Other Lenovo models or firmware hashes | Not qualified |

The owner feedback applies to the original embedded-firmware build preserved on 2026-10-19. The public build changes only firmware delivery to an owner-populated personality property; it is not labeled as an independently hardware-tested binary. Do not generalize results from Linux or another user's EFI screenshots to this machine.
