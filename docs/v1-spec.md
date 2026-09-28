# Patch Helper V1

As-built baseline and remaining acceptance work, 2026-09-28.
Original Substrate spec: `d4abe223-d4c5-4784-811b-417aa43586ee`.
This revision preserves that spec's identity and replaces its brainstorming
requirements with the decisions made during development. The original capture
is retained in [source-spec.md](source-spec.md).

V1 names this product baseline; it does not create a production release or
assert that the outstanding acceptance items below have passed.

## Purpose and ownership

Patch Helper records physical connections involving disting NT and its
expanders. It neither detects cables nor changes audio, CV, or internal routing.
A recorded connection describes the user's intended patch, not verified wiring.
There is one map per algorithm instance, owned by its containing NT preset.
NT holds authoritative state. Helper is the primary editor. All table fields must also be visible on the NT;
text editing on the module awaits firmware/SDK support.

The implementation is the C++ algorithm `ThPh` in `thorinside/patch_helper`,
with companion support in `No-Such-Device/nt_helper`. The host runs Lua downloaded
from the NT SD card. This is not a Lua audio algorithm running on the NT.

## Installation and loading

- C++ object: `/programs/plug-ins/patch_helper.o`.
- Lua companion: `/programs/helper/ThPh.lua`; the filename is the case-sensitive
  algorithm GUID, establishing a reusable convention for future companions.
- Selecting Patch Helper in Helper automatically loads the editor. There are
  no Load, Reload, Apply, or Discard buttons on the happy path.
- Use Helper's existing whole-file SD transfer and endpoint/GUID-scoped scratch
  cache. Cached Lua opens promptly and the source refreshes in the background,
  initially and once per minute while active. Failed loads retry automatically.
- Missing, malformed, or incompatible Lua produces an error. No silently bundled
  replacement hides the missing SD file. Normal parameter views remain available.

## Socket records

The table includes all 12 native inputs and 8 native outputs, including unused
sockets, followed by eight rows for each expander in physical order. Stereo
connections have separate rows. Auxiliary buses are not physical sockets.

| Field | V1 representation |
| --- | --- |
| Socket | Stable local socket identity; read-only label |
| Destination | One editable string, such as `reverb left`; up to 32 printable ASCII characters for new edits |
| Cable colour | None, Black, White, Grey, Red, Orange, Yellow, Green, Blue, Purple, Pink, Brown |
| Tag | Optional integer 1–12; stored 0 means absent |
| Group | Optional shared text, up to 32 printable ASCII characters |

A blank destination means unused. Clearing it preserves colour, tag, and group.
The colour remains visible in the minimap even when destination is blank.
Unsupported characters and overlong values are rejected, not silently truncated.
Existing preset destinations up to 63 characters remain readable and are never
silently truncated. Unrelated colour, tag or group edits retain that text; a
replacement destination must fit 32 characters. The hidden legacy title retains
its old storage limit. Each 32-character field has room for its terminator.

There is no editable patch title. The old serialized `title` field and transport
operation remain solely for compatibility with preview data.

## Helper interface

The primary view is the ordered socket table, with destination, colour, tag,
and group editable in place. There is no gear-first view in V1. Keep the
interface free of narrative/help paragraphs and save reminders.

The **Sockets** minimap uses coloured clickable dots:

- Inputs: 3 rows by 4 columns.
- Outputs: 4 rows by 2 columns.
- Each expander: 8 rows by 1 column; banks run left to right.

Selecting a dot scrolls its table row into view and highlights it. Empty,
uncoloured sockets are hollow. Dots and rows update together after local or
NT changes. On narrower windows the minimap is collapsible.

The sync indicator stays at the top left in a fixed-size slot. Beside it, show
the current NT slot name (its existing 32-character limit), falling back to
Patch Helper. This read-only Flutter header label follows slot renames without
reloading the Lua editor. Keep it on one line in the existing fixed-height bar,
using an ellipsis when space is narrow. It is not a separate patch-title field.
A compact action
bar sits at the top right. Its Add expander action opens a Lua-defined choice
dialog rendered by Flutter, containing NTX-8CV, ES-5, ESX-8GT and ESX-8CV. Choosing
one adds it; Escape, Cancel, or dismissing the dialog makes no change. The model
selector is absent from the main editor. Revalidate availability before applying
a choice if state changed while the dialog was open.

Layout stability is mandatory. Loading, sync, validation, and failure feedback
must not shift the table, headers, or controls. Use fixed status geometry and
tooltips/overlays. Preserve keyboard focus during background updates; all action
icons and coloured socket controls have accessible names.

## Expanders

Supported models are NTX-8CV, ES-5, ESX-8GT and ESX-8CV. Every manually recorded
instance has eight outputs. Repeated models are allowed. Instances can be named
(up to 32 printable ASCII characters) and moved to match rack order; moving a
bank carries all eight connection records. This records physical hardware and
does not configure its electronic connection or infer its presence.

New maps allow eight banks (84 total sockets), supporting eight NTX-8CV instances.
Older preview maps up to 13 banks remain readable/editable (124 sockets); banks
beyond eight use an Other sockets compatibility selector on the NT. Add is disabled at the new-map limit and during pending field
synchronization. Structural operations use acknowledged transactions and are not
blindly replayed after an uncertain result.

## NT pages and full field editing

Native inputs and outputs each have a permanent parameter page: Input 1–12
and Output 1–8. Every socket page contains four separate fields in table order:
Destination, Cable colour, Tag, Group. Destination and Group are fixed-value
`kNT_unitHasStrings` properties, greyed out with `NT_setParameterGrayedOut()`;
colour and numeric tag remain editable. The text callback returns the full value,
without joining it to colour/tag. Empty text displays a dash. New 32-character
values and preserved 63-character destinations fit the SDK's 64-byte buffer.

With expanders present, an Expander bank page selects bank 1–8 and displays
its read-only, greyed-out name. The following eight socket pages show the selected
bank (E1 Out 1–8 through E8 Out 1–8). Bank changes refresh page names and parameter
indices via `NT_updateParameterPages()`; the shared read-only text properties
follow that explicit selection. Helper continues to show all banks together.
There are 20 pages without expanders, 29 with up to eight banks, and one extra
compatibility page for older larger maps. Selecting a bank does not edit cables.

This layout uses 231 parameters. Numeric colour/tag indices remain independent
for all 84 sockets (`3 + 2*N`, `4 + 2*N`), preserving preview indices 0–2 and all
mappings through bank eight. Native socket text occupies 171–210, selected-bank
text 211–226, Bank 227, Name 228, and compatibility text 229–230 (plugin-local
indices; firmware common parameters add their offset). Older mappings targeting
banks 9–12 overlap the new text region and are not retained as per-bank mappings;
those map records remain intact and editable through the compatibility selector.
Do not describe this preview transition as preserving those higher-bank mappings.

**Owner decision, 2026-09-28:** the firmware author confirmed that native editable
text properties are not exposed in the C++ SDK yet, and that the planned limit
is 32 characters. The owner accepts display-only native text for the current
revision, with editing in Helper. Do not implement a custom keyboard or claim
that native text editing works. Destination, group and expander names are
limited to 32 characters for new edits in preparation for that API.

The SDK's `parameterString(self, p, v, buffer)` callback returns an entire
NULL-terminated value, not a single character; its buffer is at least 64 bytes.
Use `kNT_unitHasStrings` for native formatted values until editable strings are
supported. Separate greyed-out text properties are now implemented in Patch
Helper, using the bank selector to support eight expanders within the measured
parameter limit. The earlier concatenated display and four-bank proposal are
superseded.

Investigation on 2026-09-28 verified the diagnostic SysEx `0x53` receive path
and `parameterString` / `0x50` readback, but that does not expose a native text
editor. The connected v1.19.0beta build converts SDK units 18–99 to type 0.
Native text editing is now a future firmware/SDK integration, not a current
V1 acceptance requirement. Native expander names are displayed on the bank page.

## NT connected-socket view

The custom NT view renders a channel / parameter / value control strip in the
first 12 pixels. This replaces the firmware row only in the custom algorithm
view; normal native parameter pages remain available. The C++ API can retain
the firmware row with `draw() == false`, but does not expose a focus setter.
A device probe confirmed `NT_setParameterFromUi()` changes the requested value
without moving native focus, so retaining that row would misrepresent selection.

Pot 1 and encoder 1 turn select any configured socket in physical order. Pot 2
selects the editable Cable colour or Tag field; the read-only text fields remain
on the normal parameter pages. Pot 3 and encoder 2 turn edit the selected value.
Use the SDK UI setter with the common-parameter offset and existing numeric
indices, so native edits persist and synchronize with Helper through the normal
map revision path. Values and selections clamp at their bounds. Pot positions
initialize through `setupUi`; pot 3 uses pickup after selection, encoder or
remote changes to prevent jumping to an unrelated physical knob position.

The top strip matches the native system row's three separate cells: channel at
x=0, parameter at x=60 and value at x=138, with 9-pixel gaps and the firmware's
background tones. Text uses the native normal font at baseline 8; the list
starts below the reserved top 12 pixels.

No scroll gesture is used. Only the three pots and two encoder turns are claimed;
encoder presses and other buttons remain firmware-owned. Selection automatically
stays visible and brightens its channel label (15 selected, 8 unselected). Navigating changes no map fields,
parameter values or revision.

The list keeps four single-line rows at baselines 21, 34, 47 and 60. Connected
sockets appear in physical order. When an unused channel is selected, show it
in the bottom row with up to three nearby connected sockets above it. It remains
editable for colour/tag and disappears from the list on deselection if still
unused. In an empty map the selected unused row is still shown at the bottom.
Recompute visibility on drawing, so edits and inventory changes cannot hide the
selected channel. All configured channels remain selectable with pot 1/encoder 1.

Every row shows socket, destination, cable colour, tag and group. Destination
has a 27-character preview and group a 15-character preview, using the tiny font.
Overflow ends with an ASCII ellipsis; stored 32-character text and legacy longer
destinations remain intact, and the full strings remain on native parameter
pages. Empty text/tag displays a dash. Row backgrounds remain black; selection
uses channel-label brightness without a highlight bar.

## Synchronization and Lua events

Valid field edits update the table and dots immediately and coalesce for 300 ms.
They synchronize automatically until acknowledged. Fresh NT snapshots are merged
with desired fields, preserving unrelated native changes. If an acknowledgement
is lost, reread before retrying; already-applied values are not written twice.
Newer local edits made while an earlier write is in flight remain queued.

While editor and app are active, a one-second watch checks the map revision.
Only changed revisions trigger a consistent full read. Reads and writes are
serialized. Watch activity pauses when inactive and stops on disposal.

Lua exports `api_version = 1`, `guid`, `render(state)`, `handle(state,event)`, and
optionally `on_change(state,change)`. State includes the map, revision and adapter
properties `first_socket`, `colour`, and `tag`. `on_change` receives changed
property keys and `map_changed`; it returns only a view, never write operations.
Rapid native changes are coalesced, not promised as an interrupt-by-interrupt log.
Without the callback, Helper reruns `render`.

Lua defines socket grids, labels and dialog choices. Flutter renders the controls
and owns focus, accessibility, scrolling and overlays. Declarative write actions
are validated against the model and current state before transport. This is the
ThPh adapter, not a universal shared-state API for every plugin GUID.

Current limitation: desired fields live in the open editor session. They are not
a durable offline outbox across editor disposal, application exit, or reconnect.
That boundary must be resolved or explicitly accepted before claiming eventual
consistency across those lifecycle changes.

## Persistence, transport and runtime

Preset serialization uses the `patch_helper` member: version 1 for native-only
maps and version 2 for expanded maps. Missing state creates an empty map;
malformed present state is rejected atomically. Unrelated firmware slot data
is not owned by this serializer. Preset replacement invalidates the editor lease.

The development USB SysEx protocol uses manufacturer namespace 0x7d, GUID ThPh,
leases and revision-checked writes. See [live-map-protocol.md](live-map-protocol.md)
for packet layout. This is not a claim of registered production manufacturer ID.

Lua evaluation runs in a fresh, disposable host isolate with a two-second deadline
and 64 KiB source/result budgets. Dangerous standard globals are removed. These
controls do not provide a hard memory quota against hostile Lua; this preview
assumes trusted SD companions.

All plugin memory is declared in `calculateRequirements()` and constructed in
host-provided aligned storage. No later heap allocation is allowed. Build PIC,
Thumb Cortex-M7 hard float with unaligned accesses disabled. Audio processing
never alters buses. Audit imports; do not assume libc symbols such as `memcmp`
are supplied by the firmware.

## Evidence and remaining acceptance

Built and exercised: preset-owned map, SD Lua loading/cache, table and minimap,
all four expander models, naming/reordering, automatic field reconciliation,
property-change callbacks, independent colour/tag pages and dynamic page updates.
The owner reported the connected build working well on 2026-09-28.

Hardware evidence on v1.19.0beta (2026-09-16 build): native pages grew from 20 to
28 after adding NTX-8CV; native colour/tag changes appeared in Helper; Helper
destination changes appeared on the NT; saving and recalling `Patch Pages Test`
retained the map and pages. The firmware accepted 240 plugin parameters but
rejected 241 and 251, including in an empty preset. This is a measured firmware
constraint, not a timeless SDK guarantee.

The revised four-field layout was exercised on the device for Input 1 and E8
Out 1 using an eight-bank test map; the working preset was restored afterward.
The connected-list screen clears the top control strip. The owner reported that
partial custom-control overrides interfered with native editing; the final
control design explicitly owns channel, parameter and value navigation. Callback
tests cover the pots, encoders, pickup, selection visibility, unused-socket editing, single-line columns,
ellipsis bounds and unchanged state while navigating. Physical control feel remains an owner acceptance check.

The action dialog is covered by Flutter tests for all four choices, cancel and
Escape, bank limits, invalid Lua schemas, stale actions and stable geometry.
Full Helper suite: 3924 tests passed, with a clean analyzer.
The running macOS app opens and cancels the updated SD Lua dialog; the sync
indicator remains at the top left. Native sanitizer tests, ARM object
inspection and strict-alignment Unicorn startup checks are separate from hardware
acceptance. No hardware acceptance is inferred solely from those tests.

Outstanding: end-of-chain
preset merge/lifecycle behavior; persistence of pending edits across editor/app
lifecycle changes; wider firmware acceptance and production release review.

Deferred rather than silently claimed complete: gear-first presentation, sorting
by gear/colour, group-based presentation, arbitrary GUID shared-state adapters,
and expander topology detection. Editable patch titles and manual companion load,
reload, Apply/Discard, and unsent-edit workflows were explicitly removed by the
owner and must not return as default UX.
