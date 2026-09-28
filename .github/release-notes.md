Patch Helper records physical cable connections with your NT preset.
This release includes its SD-card Lua companion for NT Helper 2.56.0 or newer.

Record physical connections for 12 inputs, 8 outputs, and up to eight expander
banks. Edit destination, cable colour, tag, and group in Helper. On the NT,
view every field and edit colour/tag with the pots and encoders.

Download **patch_helper.zip** and copy its `programs` folder to the
SD root. It includes both files:

```text
programs/plug-ins/patch_helper.o
programs/helper/ThPh.lua
```

The companion requires
[NT Helper 2.56.0](https://github.com/No-Such-Device/nt_helper/releases/tag/v2.56.0)
or newer. Helper can install both files from this ZIP through its Gallery.

Hardware testing during development used firmware **1.19.0beta** and covered
loading, native display, two-way edits, dynamic pages, and regular preset recall.
Other firmware and end-of-chain preset merging remain unverified. Text is
read-only on the NT. Wait for sync before leaving Helper's editor, then save the
NT preset to retain changes.

The owner verified Gallery download and installation of both companion files.

The release ZIP is built by CI after native sanitizer tests, static analysis,
ARM object inspection, and emulated startup checks. Those checks do not replace
testing this packaged build on your device.

[Installation and controls](https://github.com/thorinside/patch_helper#readme)
