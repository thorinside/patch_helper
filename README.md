# Patch Helper

A cable and internal-bus reference for disting NT, saved with your preset. Record what
is plugged into each socket, then look it up on the NT or edit it in NT Helper.
Connections are entered manually; Patch Helper does not detect cables or change
audio, CV, or routing.

**Latest release: 0.6.0.** Existing physical-socket workflows have development
hardware test evidence; the new AUX lifecycle and this packaged build still need
physical verification. The companion editor requires
[NT Helper 2.56.0](https://github.com/No-Such-Device/nt_helper/releases/tag/v2.56.0)
or newer.

This release adds AUX bus records and a Lua-defined editor layout. AUX editing
and resizable columns require the included plugin and SD companion plus an NT
Helper build containing [PR #157](https://github.com/No-Such-Device/nt_helper/pull/157).
The earlier NT Helper 2.56.0 and 2.57.0 releases do not include that editor.
No new NT Helper release is published alongside this package.

**Compatibility:** presets containing AUX records use **version 3** and cannot
be read by older Patch Helper binaries. Physical AUX save/recall and control
acceptance remain unverified; automated checks do not establish hardware acceptance.

## Install

Download **patch_helper.zip** from [Releases](https://github.com/thorinside/patch_helper/releases/latest).
Copy its `programs` folder to the root of the NT SD card, preserving these paths:

```text
programs/plug-ins/patch_helper.o
programs/helper/ThPh.lua
```

Restart the NT after copying, then add **Patch Helper** to your preset and turn
Bypass off. NT Helper 2.56.0 or newer can also install this ZIP
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

Under **Outputs**, use **+ AUX** to choose any unused **AUX 1–44**. Only added
buses appear, in numeric order. Each row has **Description** and **Group**, both
up to 32 printable ASCII characters. Added blank rows stay in the map.
**Remove** deletes the row and its information, with confirmation when either
field contains text. Re-adding the bus starts blank. AUX records describe signals;
adding one never changes internal routing.

Drag a column boundary to resize it. A focused boundary also supports the left
and right arrow keys. Helper remembers widths on this computer across editor
sessions, separately for the socket and AUX tables. Column order, labels, and
default sizes come from the SD companion. These display preferences are separate
from the NT preset.

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

When the map contains AUX rows, an **AUX buses** page selects an added bus and
shows its complete Description and Group as read-only text. The custom list also
includes added AUX rows, including blank ones. Selecting AUX shows **Read only**;
colour and tag controls apply only to physical sockets.

## Compatibility and limits

Existing cable-only preset save/recall has been checked on hardware. AUX
save/recall and physical control acceptance still need device verification.
The updated plugin reads existing presets, including legacy thirteen-bank maps,
without moving socket IDs or parameter mappings. AUX presets use a new data
version (3) and require the updated plugin and Helper; older binaries cannot read
them. Removing all AUX rows restores the earlier cable-only data format. End-of-chain preset
merging remains unverified. If the editor fails to load, check the Helper build
and exact `programs/helper/ThPh.lua` path; it retries automatically after errors.

[Development and build notes](https://github.com/thorinside/patch_helper/blob/main/docs/development.md) · [V1 specification](https://github.com/thorinside/patch_helper/blob/main/docs/v1-spec.md)
