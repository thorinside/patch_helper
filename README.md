# Patch Helper

A physical cable reference for disting NT. It records intended connections;
it does not detect cables, route signals, or modify audio/CV.

**Development preview — requires the matching NT Helper branch; not a production release.**

The plug-in displays a preset-owned map containing the twelve native
inputs, eight native outputs, and manually added eight-output expanders. Each socket has a destination, cable colour,
optional Tag (1–12), and group. An empty destination means unused. Clearing it
preserves the other cable metadata. The SD-card Lua companion provides the Helper editor.
New destinations, groups and expander names accept up to 32 printable ASCII
characters. Older longer destinations remain intact until replaced.
On-device text editing awaits SDK support. Each native socket page has separate
**Destination**, **Cable colour**, **Tag**, and **Group** properties. Destination
and Group are greyed out and read-only; colour and tag remain editable.

The NT keeps **Input 1–12** and **Output 1–8** pages. With expanders present,
**Expander bank** selects one of up to **eight banks**, shows its name, and updates
the eight expander socket pages to that bank. Helper shows every bank together.
Text updates from Helper automatically. Tag 0 means none; blank text displays a dash.
Older maps with more than eight banks retain all their records and can use
**Other sockets** on the NT. Numeric mappings through bank eight remain stable;
old mappings for higher banks need reassignment to the compatibility controls.

The custom NT view has a compact channel / parameter / value row at the top,
with four single-line socket rows below it. Its controls are:

| Control | Action |
| --- | --- |
| Pot 1 or encoder 1 turn | Select a socket/channel |
| Pot 2 | Select Cable colour or Tag |
| Pot 3 or encoder 2 turn | Edit that value |

All configured sockets can be selected. Selection automatically stays visible
and brightens the selected channel label. Unused channels appear in the bottom row while
selected, with up to three connected rows above; colour and tag remain editable. Text fields remain
read-only on the NT and are visible on the normal parameter pages. Pot 3 uses
pickup after selection changes or encoder/remote edits to avoid accidental jumps.
Each row shows socket, destination, colour, tag and group. Long destination/group
previews use an ellipsis without changing stored text; full values remain on the
normal parameter pages. Other unused channels stay filtered out. No scroll
gesture is needed; navigation does not change the patch.
The custom top row replaces the firmware row only in the algorithm display;
normal native parameter pages are retained.

## Development preview

The object is built against the pinned official API v13. Firmware compatibility,
end-of-chain preset behavior and wider firmware compatibility remain pending.
Loading, native page updates and two-way editor changes have been checked on
the connected v1.19.0beta device. No firmware compatibility claim is made from a
successful ARM build alone.

For owner testing, the built `plugins/patch_helper.o` belongs under
`programs/plug-ins/` on the SD card. Restart or remount the card, then select
**Patch Helper** from Add algorithm. New instances have an empty connection list. The
[fixture](tests/fixtures/native-map.json) exercises populated maps in native
tests. Saving and recalling the connected test preset has been verified; end-of-chain
preset merging and standalone preset-file codec integration remain pending.

`make package` builds `build/patch_helper-preview.zip`, containing:

```text
programs/plug-ins/patch_helper.o
programs/helper/ThPh.lua
README.md
```

The development CI artifact uses the same package command. Extract the ZIP at
the SD root, or install it through the matching NT Helper gallery support.
Helper installs the Lua companion before the C++ object and creates its folder
if needed. The companion is not a separate Lua algorithm. A failed upload is
reported without retrying the file in another directory.

For a manual install, put `helper/ThPh.lua` in `/programs/helper/` on the SD card. Opening
Patch Helper in NT Helper automatically opens its editor. Helper runs the Lua
on the computer and caches the downloaded source in scratch storage. It checks
for updated Lua in the background. Missing files produce an error.

Edit directly in the table: valid changes sync automatically after a short
pause, with no Apply or reload controls. The small status dot indicates when
the editor is up to date. Cable-colour changes also update the **Sockets** dots,
including sockets without destination names. Save the preset normally to keep
changes across preset loads. Status/error feedback does not move the table.

Use the **+** action at the top right to choose NTX-8CV, ES-5, ESX-8GT, or ESX-8CV
in a dialog. The sync indicator remains at the top left. Each adds eight physical
output records. Older 13-bank maps remain readable; banks beyond eight use the
**Other sockets** selector page on the NT. Expander section actions rename instances and move their whole
banks, retaining cable records. This does not configure an expander's electronic
connection or change routing. See the [companion contract](docs/companion-contract.md)
and [live map protocol](docs/live-map-protocol.md).
See the [V1 spec](docs/v1-spec.md) for the as-built baseline and remaining acceptance work.
See [development notes](docs/development.md) for build commands, format details,
the originating Substrate spec, and the remaining integration work.

While the editor is open and active, Helper checks for NT changes once a second.
Native colour/tag edits update the table through Lua. Local field changes merge
with fresh NT records and retry after temporary failures until acknowledged;
values already applied are not written twice. The watch pauses when the editor
or app is inactive.
