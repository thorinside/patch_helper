Patch Helper v0.6.0 adds selected AUX bus records and a Lua-defined companion editor.

- Record any selected **AUX 1–44** bus with Description and Group, saved with the NT preset.
- Add and remove AUX records through the matching NT Helper editor; view added buses on the native NT display.
- The SD Lua companion declares table columns and record sections. A matching NT Helper build renders them and remembers resized column widths.
- Existing physical socket identities, expander records, and parameter mappings remain stable.

## Compatibility and verification limits

**AUX presets use data format version 3. Older Patch Helper binaries cannot read these presets.**
The updated plugin reads existing cable-only presets, including legacy expander maps.
Presets without AUX records retain their earlier format; removing all AUX records restores the cable-only format.

**Physical-device AUX save/recall and control acceptance remain unverified.**
Existing physical-socket workflows have hardware test evidence from development;
those results do not validate the new AUX lifecycle or this packaged build on
hardware. Firmware compatibility beyond those recorded tests and end-of-chain
preset merging remain unverified.

AUX editing and resizable columns require the included updated plugin and SD
companion plus an NT Helper build containing
[NT Helper PR #157](https://github.com/No-Such-Device/nt_helper/pull/157).
The published NT Helper 2.56.0 and 2.57.0 releases do not include that editor.
**No NT Helper release is published as part of this release.**

## Installation

Download **patch_helper.zip** and copy its `programs` folder to the NT SD root,
preserving both paths:

```
programs/plug-ins/patch_helper.o
programs/helper/ThPh.lua
```

Restart the NT after copying. Wait for the companion editor to finish syncing,
then save the NT preset to retain changes. The companion Lua file is not a Lua
algorithm to add to a preset.

The CI release ZIP is built only after native ASan/UBSan tests, static analysis,
ARM object inspection, and strict-alignment emulated startup checks pass. These
checks do not replace a physical-device test.

[Installation and controls](https://github.com/thorinside/patch_helper#readme)
