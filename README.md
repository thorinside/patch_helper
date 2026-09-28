# Patch Helper

A physical cable reference for disting NT, saved with your preset. Record what
is plugged into each socket, then look it up on the NT or edit it in NT Helper.
Connections are entered manually; Patch Helper does not detect cables or change
audio, CV, or routing.

**Development preview.** Tested on disting NT firmware **1.19.0beta**. Other
firmware versions have not been verified. The companion editor requires the
[NT Helper development work in PR #152](https://github.com/No-Such-Device/nt_helper/pull/152),
including its dependent companion changes; it is not yet in a released Helper build.

## Install

Download the plugin ZIP from [Releases](https://github.com/thorinside/patch_helper/releases).
Copy its `programs` folder to the root of the NT SD card, preserving these paths:

```text
programs/plug-ins/patch_helper.o
programs/helper/ThPh.lua
```

Restart the NT after copying, then add **Patch Helper** to your preset and turn
Bypass off. The matching Helper development build can also install this ZIP
through its Gallery installer, placing both files in their respective folders.

The Lua file stays on the SD card. Helper downloads, caches, and runs it on the
computer to describe the editor; Flutter renders the controls. The NT runs the
C++ plugin and holds the connection data. The companion is not a Lua algorithm
to add to the NT preset.

## In NT Helper

Select the Patch Helper slot; its editor loads automatically. Edit the table in
socket order: **12 inputs**, **8 outputs**, then **8 outputs per expander**.

| Field | Values and defaults |
| --- | --- |
| Destination | Up to 32 printable ASCII characters; initially blank (unused) |
| Cable colour | None, Black, White, Grey, Red, Orange, Yellow, Green, Blue, Purple, Pink, Brown; initially None |
| Tag | Blank or an integer from 1 to 12; initially blank |
| Group | Up to 32 printable ASCII characters; initially blank |

For example, record `From Beads L`, Purple, tag `1`, and group `FX` on Input 1.
Clearing a destination marks the socket unused while retaining its other fields.
Group is a text label; it does not sort or regroup the table.

Click a coloured dot in **Sockets** to scroll to and highlight its row. Use **+**
at the top right to add an expander: NTX-8CV, ES-5, ESX-8GT, or ESX-8CV. You can
record up to **eight expanders**, including repeated types. Section controls
rename them (up to 32 printable ASCII characters) or move whole banks with their
connection records. Adding a bank does not configure the physical expander.

Valid edits synchronize automatically. Changes made on the NT also appear in
Helper while the editor is active. Hover over the dot beside the slot name for
sync status or error details. Wait for **Up to date** before leaving the editor,
then **save the NT preset** to retain changes when it is recalled. Pending edits
are not retained after the editor closes.

## On the NT

The custom display shows a channel/parameter/value strip above four socket
rows, with destination, colour, tag, and group on each line.

| Control | Action |
| --- | --- |
| Pot 1 or encoder 1 | Select a socket |
| Pot 2 | Select Cable colour or Tag |
| Pot 3 or encoder 2 | Change its value |

Selection stays visible and brightens the channel label. Unused sockets can
still be selected and edited; the selected unused socket appears on the bottom
row. Long text is shortened on this display without changing the stored value.
Pot 3 uses pickup after selection changes to avoid value jumps.

The normal parameter pages show **Destination**, **Cable colour**, **Tag**, and
**Group** for each socket. Text is greyed out and read-only on the NT; edit it in
Helper. Tag `0` means no tag. **Expander bank** selects which bank's eight socket
pages are shown. Helper displays all banks together.

## Preview limits

Regular preset save/recall has been checked on hardware. End-of-chain preset
merging remains unverified. If the editor fails to load, check the Helper build
and exact `programs/helper/ThPh.lua` path; it retries automatically after errors.

[Development and build notes](https://github.com/thorinside/patch_helper/blob/main/docs/development.md) · [V1 specification](https://github.com/thorinside/patch_helper/blob/main/docs/v1-spec.md)
