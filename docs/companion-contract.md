# SD-card Helper companion contract

Source: Substrate spec `d4abe223-d4c5-4784-811b-417aa43586ee`, reread 2026-09-27
and captured in `source-spec.md`. The user approved the straight table/minimap
and reiterated host-executed SD-card Lua as required. The user then explicitly
selected a top-level `helper` directory instead of the plug-ins directory.

## Installation and execution

- NT binary: `/programs/plug-ins/patch_helper.o`.
- Host Lua companion: `/helper/ThPh.lua`.
- The development archive includes both paths. Copy its folders to the SD root.
- In Helper, select the Patch Helper algorithm's standard view and choose
  **Load SD companion**. It uses the existing whole-file SD download operation.
- Helper runs the downloaded source on the computer; the NT never runs this Lua.
  No bundled hard-coded editor is used when the file is absent or incompatible.
- **Reload companion & map** re-downloads the source and opens a fresh map lease.
  Missing/failed companions leave ordinary parameter/spreadsheet views available.

Discovery uses `/helper/<GUID>.lua`, with case preserved and a four-character
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
PatchMapClient transport. Lua cannot send MIDI directly. No automatic write
replay occurs. The view changes to acknowledged state only after success.

## Loading and recovery boundary

Only load companion code from a source you trust. There is an explicit load
button; no scripts run merely by inserting a card. Source is held only for that
editor/device session, and a reconnect creates a new session. There is no
persistent source cache or silent bundled fallback.

Execution uses a disposable Dart isolate with a two-second deadline. Source and
serialized result budgets are 64 KiB each. Filesystem/process/network/module
loading and debug globals are removed. An exception or timeout disables edits
until reload. These controls protect responsiveness and constrain available
APIs; Dart isolates do not provide a hard per-script memory quota or a security
boundary against deliberately hostile allocation. This is a trusted-companion
development preview, not a general untrusted-code marketplace.

## Acceptance and remaining scope

Automated evidence must cover actual shipped Lua loaded through the SD loader,
its returned grids, declarative actions, missing/invalid/infinite scripts,
acknowledgement failures, and the exact shared C++/Dart wire transcript.

This revision includes editable expander names and order with record-preserving
moves, repeated expander types, title/connection editing, and persistent state.
The one-byte wire address bounds storage to 13 eight-output expanders (124 total
sockets); this is a protocol capacity, not a hardware topology claim. Expanders
are manually recorded, never auto-detected or configured for signal routing.

NT-side text entry, a gear-sorted alternative, arbitrary companion discovery,
and regular/end-of-chain preset lifecycle acceptance remain separate work from
the approved Helper table. No physical-device acceptance is implied by tests.

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
stealing text focus, and skips navigation during unsent edits. Colour/tag and
other map changes rerender the acknowledged table. Notifications can return only
a view document; they cannot invoke a write action. User gestures still use
`handle` and the existing acknowledgement path, so refreshes cannot echo edits.

Command 9 checks the existing lease and returns the current revision and property
snapshot. An unchanged revision costs one small request; only a changed revision
causes a consistent full-map read. Reads and edits never overlap. A native edit
during a multi-record read, failed read, callback failure, or expired lease stops
editing until explicit reload. No polling opens a new lease or retries a write.

Draft conflicts retain the last displayed map and all unsent fields. The host
checks for drafts both before and after evaluating Lua so typing during an
in-flight refresh cannot be lost. Reload still asks before discarding drafts.
The periodic watch stops on disposal and pauses with the inactive editor/app.

Native First socket retains index 0; Cable colour and Tag append indices 1 and 2.
The plug-in projects the selected record through callback-safe host setters with
reentrancy protection; only changed native cable values advance map revision.
Preset deserialization invalidates the lease and reprojects controls. Native text
entry and physical-device acceptance remain pending.
