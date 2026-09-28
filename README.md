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
On-device text editing awaits SDK support. Destination and group are displayed
alongside the existing native controls.

The NT has one parameter page per socket: **Input 1–12**, **Output 1–8**,
then **E1 Out 1–8** and subsequent expander banks, up to 12 banks. Each page has independent
**Cable colour** and **Tag** controls. Their displayed values include the
destination and group respectively: `Purple | From Beads L` and `1 | FX`.
Text follows changes made in Helper; colour and tag remain editable on the NT.
Tag 0 means none. Adding an expander in
Helper adds eight pages on the NT automatically. The display keeps the native
parameter line visible and shows cable records below it, following the last
socket edited on the NT. Long destinations are clipped only on screen;
the saved text is retained.

## Development preview

The object is built against the pinned official API v13. Firmware compatibility,
end-of-chain preset behavior and wider firmware compatibility remain pending.
Loading, native page updates and two-way editor changes have been checked on
the connected v1.19.0beta device. No firmware compatibility claim is made from a
successful ARM build alone.

For owner testing, the built `plugins/patch_helper.o` belongs under
`programs/plug-ins/` on the SD card. Restart or remount the card, then select
**Patch Helper** from Add algorithm. New instances show unused sockets. The
[fixture](tests/fixtures/native-map.json) exercises populated maps in native
tests. Saving and recalling the connected test preset has been verified; end-of-chain
preset merging and standalone preset-file codec integration remain pending.

Install `helper/ThPh.lua` in `/programs/helper/` on the SD card. Opening
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
output records. Older 13-bank maps remain readable; the last bank uses a
**Legacy bank 13** selector page on the NT. Expander section actions rename instances and move their whole
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
