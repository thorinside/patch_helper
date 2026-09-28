# SD-card Helper companion contract

Source: Substrate spec `d4abe223-d4c5-4784-811b-417aa43586ee`, reread 2026-09-27
and captured in `source-spec.md`; [V1](v1-spec.md) is the current baseline. The user approved the straight table/minimap
and reiterated host-executed SD-card Lua as required. The user then explicitly
selected `/programs/helper/` for companions, separate from the plug-ins directory.

## Installation and execution

- NT binary: `/programs/plug-ins/patch_helper.o`.
- Host Lua companion: `/programs/helper/ThPh.lua`.
- The development archive includes both paths. Copy its folders to the SD root.
- Selecting Patch Helper automatically loads the editor through the existing
  whole-file SD download operation, reusing endpoint/GUID-scoped scratch files.
- Helper runs the downloaded source on the computer; the NT never runs this Lua.
  No bundled hard-coded editor is used when the file is absent or incompatible.
- Background checks refresh the SD source once per minute while active.
  Missing/failed companions show an error and recover automatically after correction.
  The ordinary parameter/spreadsheet views remain available.

Discovery uses `/programs/helper/<GUID>.lua`, with case preserved and a four-character
filename-safe GUID. The script must declare the same GUID. This convention is
shared by future companions, rather than a filename tied to this product.

The first host capability is the `ThPh` socket table, API version 1. This is a
bounded extension to the existing Helper Lua evaluator, not a claim that every
algorithm can now expose arbitrary shared state.

## Lua contract

The module returns `api_version = 1`, `guid = 'ThPh'`, `render(state)`, and
`handle(state, event)`. Both functions are pure, evaluated in a fresh runtime.
`state` is a copy of the last acknowledged map, including `expanders` (an empty
array for a native-only map). It never contains transports or host objects.

`render` returns a version-1 `socket_table`: labels for socket, destination,
colour, tag and group; and ordered groups with `title`, `short`, zero-based
`start`, `count`, and `columns`. The host rejects missing/duplicate/out-of-range
sockets and invalid grids. Lua chooses the input 3×4, output 4×2 and expander
8×1 geometry. Host primitives implement accessible fields, scrolling, selected
rows, colour swatches, and focus using the existing Material theme.

Events are `set_connection`, `set_title`, `add_expander`, `rename_expander`, and
`move_expander`. `handle` returns one declarative action of the same kind. The
host validates the action through the shared model and uses the serialized
PatchMapClient transport. Lua cannot send MIDI directly. Field writes use optimistic display and automatic reconciliation as described below.
Structural operations remain acknowledged transactions.

## Loading and recovery boundary

Only load companion code from a source you trust. Selecting the algorithm loads
its SD companion automatically through the existing whole-file transfer path.
The source is cached in endpoint/GUID-scoped scratch storage and refreshed in
the background; there is no load/reload control or silent bundled fallback.

Execution uses a disposable Dart isolate with a two-second deadline. Source and
serialized result budgets are 64 KiB each. Filesystem/process/network/module
loading and debug globals are removed. An exception or timeout disables edits
until automatic source refresh recovers. These controls protect responsiveness and constrain available
APIs; Dart isolates do not provide a hard per-script memory quota or a security
boundary against deliberately hostile allocation. This is a trusted-companion
development preview, not a general untrusted-code marketplace.

## Acceptance and remaining scope

Automated evidence must cover actual shipped Lua loaded through the SD loader,
its returned grids, declarative actions, missing/invalid/infinite scripts,
acknowledgement failures, and the exact shared C++/Dart wire transcript.

This revision includes editable expander names and order with record-preserving
moves, repeated expander types, connection editing, and persistent state.
The title field/action remain a wire compatibility detail, not an editor feature.
The one-byte wire address preserves storage for 13 eight-output expanders (124 total
sockets); this is a protocol capacity, not a hardware topology claim. Expanders
are manually recorded, never auto-detected or configured for signal routing.

Native text-property editing is required for V1 field parity and is still pending.
Gear-sorted presentation and arbitrary companion adapters are deferred. Regular
preset recall has hardware evidence; end-of-chain merging remains unverified. No physical-device acceptance is implied by tests.

## Live NT properties (revision 4)

An optional `on_change(state, change)` callback returns the same validated table
document as `render`. Without it, the host falls back to `render`. This extends
API 1 without breaking older companions. Every call uses a fresh Lua runtime;
this is a declarative callback, not a persistent Lua closure or event loop.

The `ThPh` adapter supplies `state.properties = {first_socket, colour, tag}` and
`state.revision`. First socket is one-based; colour is the existing palette index;
tag is 0 for none or 1–12. These are stable adapter keys, not global firmware
parameter numbers. Other algorithm adapters can use the same callback contract;
this revision does not claim universal property discovery.

`change = {type = "nt_changed", properties = {key = {previous, value}},
map_changed = boolean}` contains only changed property keys (previous omitted on
first observation). The snapshot is authoritative at its observed map revision.
Rapid changes between reads are coalesced; this is a one-second read watch, not
an interrupt stream or a promise to report every intermediate value.

The supplied Lua reacts to `first_socket` changes by returning `focus_socket`
(zero-based) in its table document. Flutter scrolls/highlights that row without
stealing text focus, and skips navigation while fields are syncing. Colour/tag and
other map changes rerender the acknowledged table. Notifications can return only
a view document; they cannot invoke a write action. User gestures still use
`handle` and the existing acknowledgement path, so refreshes cannot echo edits.

Command 9 checks the existing lease and returns the current revision and property
snapshot. An unchanged revision costs one small request; only a changed revision
causes a consistent full-map read. Reads and edits never overlap. A native edit
during a multi-record read or a failed read triggers automatic reconciliation.
Valid edits update the visible rows and dots immediately, then coalesce for 300 ms.
Desired fields are merged onto a fresh NT snapshot before writing, preserving
unrelated NT changes. A lost acknowledgement is resolved by rereading; already
matching values are not resent. Newer edits made during a write remain queued.
There are no Apply/Discard or manual load/reload controls. Errors use fixed status
indicators and overlays so feedback never shifts the table. The watch stops on
disposal and pauses with the inactive editor/app.

The original selector/colour/tag indices 0–2 remain for preview preset/mapping
compatibility. Older 13-bank maps expose these on a final compatibility page;
ordinary maps omit them from visible pages. Socket N (zero-based) owns
colour parameter `3 + 2*N` and tag `4 + 2*N` through bank eight. Each native
socket has a four-field page: Destination, Cable colour, Tag, Group. An Expander
bank selector chooses which bank's eight pages follow the twenty native pages.
This supports eight new banks within 231 definitions and 30 reserved pages;
older larger maps retain their records but higher-bank mappings need reassignment.
The connected firmware accepts 240 plug-in parameters and rejects 241.

All storage is reserved in calculateRequirements(); construct uses only that
memory. Step calls `NT_updateParameterPages()` when bank selection or inventory
changes. Callback-safe setters project acknowledged records with reentrancy
protection. The old selected-socket property still identifies the most recently
edited native row for Lua callbacks. Preset deserialization invalidates the
lease and reprojects controls. Native text is shown in separate greyed-out
properties; editing remains in Helper until SDK support arrives.

The NT custom view owns its channel / parameter / value strip: pot 1 or encoder
1 selects the socket, pot 2 selects colour/tag, and pot 3 or encoder 2 edits it.
Selection automatically stays visible in the five-column single-line list.
An unused selected socket appears in the bottom row and remains editable for
colour/tag. Long destination/group previews use ellipses. No scroll gesture is
required, and navigation changes no map or property state. Normal native parameter pages remain available.

## Lua choice dialogs

`render` may return `actions`, a list of `{id, label, dialog}` entries. The current
capability supports `id = 'add_expander'` and `dialog = {type = 'choice_dialog',
title, cancel, choices = {{label, value}, ...}}`. Choices are unique model indices
0–3. Missing actions means no toolbar actions, so earlier companions remain
compatible. Invalid/duplicate action IDs and choices fail validation.

Flutter places the action icon at the top right and retains sync status at the
top left, in a fixed-height bar. It renders the Lua title and choices as a modal
choice list; no permanent model dropdown remains. Selecting a model dispatches
`add_expander` through the existing `handle` path. The latest map and Lua action
are revalidated on selection; Lua cannot remap the selected model. Cancel,
Escape and barrier dismissal do not mutate the map. No file-loading buttons or
success narratives are introduced.
