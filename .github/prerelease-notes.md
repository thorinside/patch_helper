Development preview for testing Patch Helper and its NT Helper companion.

Record physical connections for 12 inputs, 8 outputs, and up to eight expander
banks. Edit destination, cable colour, tag, and group in Helper. On the NT,
view every field and edit colour/tag with the pots and encoders.

Download **patch_helper-preview.zip** and copy its `programs` folder to the
SD root. It includes both files:

```text
programs/plug-ins/patch_helper.o
programs/helper/ThPh.lua
```

The companion requires the development changes in
[NT Helper #152](https://github.com/No-Such-Device/nt_helper/pull/152), including
its dependent companion work. This support is not yet in a released Helper
build. That development build can also install the ZIP through its Gallery.

Hardware testing during development used firmware **1.19.0beta** and covered
loading, native display, two-way edits, dynamic pages, and regular preset recall.
Other firmware and end-of-chain preset merging remain unverified. Text is
read-only on the NT. Wait for sync before leaving Helper's editor, then save the
NT preset to retain changes.

The release ZIP is built by CI after native sanitizer tests, static analysis,
ARM object inspection, and emulated startup checks. Those checks do not replace
testing this packaged build on your device.

[Installation and controls](https://github.com/thorinside/patch_helper#readme)
