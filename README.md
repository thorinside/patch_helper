# Patch Helper

A physical cable reference for disting NT. It records intended connections;
it does not detect cables, route signals, or modify audio/CV.

**Development preview — requires the matching NT Helper branch; not a production release.**

The plug-in displays a preset-owned map containing the twelve native
inputs, eight native outputs, and manually added eight-output expanders. Each socket has a destination, cable colour,
optional Tag (1–12), and group. An empty destination means unused. Clearing it
preserves the other cable metadata. The SD-card Lua companion provides the Helper editor, including the map title.
On-device destination, group and title text editing remain pending.

The NT screen shows four rows at a time. Use **Connection → First socket** to
choose the first displayed socket. **Cable colour** and **Tag** edit that socket;
Tag 0 means none. Switching sockets loads their stored colour/tag without
changing either record.
Unused sockets remain visible. Long destinations are clipped on screen only;
the saved text is retained. Tag is visible in its native control; groups remain stored metadata.

## Development preview

The object is built against the pinned official API v13. Firmware compatibility,
physical-device loading, preset recall, and end-of-chain preset behavior still
need device verification. No firmware compatibility claim is made from a
successful ARM build alone.

For owner testing, the built `plugins/patch_helper.o` belongs under
`programs/plug-ins/` on the SD card. Restart or remount the card, then select
**Patch Helper** from Add algorithm. New instances show unused sockets. The
[fixture](tests/fixtures/native-map.json) exercises populated maps in native
tests. Before editing a hardware preset, export a preset containing the plugin
and confirm where the firmware embeds its custom serialization object; that
outer preset envelope has not yet been verified on a device.

Install `helper/ThPh.lua` at the SD-card root alongside the `programs`
folder. In NT Helper, open the Patch Helper slot and choose **Load SD companion**.
Helper downloads `/helper/ThPh.lua` and runs it on the computer.
The file defines the straight table and clickable socket minimap. Apply each
edited row or press Enter, then use the normal **Save preset** action to keep
acknowledged changes on the NT. Reload after any uncertain write.

Add NTX-8CV, ES-5, ESX-8GT, or ESX-8CV from the dropdown. Each adds eight physical
output records. Expander section actions rename instances and move their whole
banks, retaining cable records. This does not configure an expander's electronic
connection or change routing. See the [companion contract](docs/companion-contract.md)
and [live map protocol](docs/live-map-protocol.md).
See [development notes](docs/development.md) for build commands, format details,
the originating Substrate spec, and the remaining integration work.

While the editor is open and active, Helper checks for NT changes once a second.
Moving First socket on the NT highlights its row in Helper. Native colour/tag
edits update the table through the Lua companion. Clean maps refresh automatically;
if the NT changes the map while Helper has unsent edits, those drafts remain
visible and editing pauses until an explicit reload. A preset/session change
also requires reload. The watch pauses when the editor or app is inactive.
