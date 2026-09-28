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
NT holds authoritative state. Helper is the primary editor; all table fields
must also be available through the NT's native properties and editors.

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
| Destination | One editable string, such as `reverb left`; up to 63 printable ASCII characters |
| Cable colour | None, Black, White, Grey, Red, Orange, Yellow, Green, Blue, Purple, Pink, Brown |
| Tag | Optional integer 1–12; stored 0 means absent |
| Group | Optional shared text, up to 31 printable ASCII characters |

A blank destination means unused. Clearing it preserves colour, tag, and group.
The colour remains visible in the minimap even when destination is blank.
Unsupported characters and overlong values are rejected, not silently truncated.
These limits describe the current implementation, not general NT firmware limits.

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

The sync indicator stays at the top left in a fixed-size slot. A compact action
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
(up to 31 printable ASCII characters) and moved to match rack order; moving a
bank carries all eight connection records. This records physical hardware and
does not configure its electronic connection or infer its presence.

New maps allow 12 banks (116 total sockets). Older 13-bank preview maps remain
readable/editable (124 sockets); their last bank uses a compatibility selector
page on the NT. Add is disabled at the new-map limit and during pending field
synchronization. Structural operations use acknowledged transactions and are not
blindly replayed after an uncertain result.

## NT pages and full field editing

Each socket has its own parameter page: Input 1–12, Output 1–8, E1 Out 1–8, and
subsequent banks. Page count changes automatically via `NT_updateParameterPages()`.
Colour and tag use independent parameters. Saved parameter mappings retain
preview indices 0–2, and per-socket colour/tag pairs at `3 + 2*N`, `4 + 2*N`.
The current implementation reserves 235 parameters and supports those two
fields on the native pages. The algorithm display shows socket, colour and
destination; display clipping never truncates the saved destination.

**Required before full V1 parity is accepted:** surface destination and group as
native text-input properties alongside colour and numeric tag, using the NT's
existing text editor and property-change path. All table fields must be visible
and editable on the module; socket identity stays read-only. Do not substitute
a custom encoder keyboard or mark text editing as implemented just because a
formatted display string is available. Retain independent socket pages and
preset/mapping compatibility while respecting the measured parameter limit.
Expander names must also remain visible and editable through native text entry.

Investigation on 2026-09-28 identified firmware text-input unit 18 in Helper.
The pinned and current public API v13 header exposes `parameterChanged(self,p)`
and numeric display formatter `parameterString(self,p,v,buff)`; its enum does
not declare unit 18. The connected nt-mcp examples checked so far demonstrate
formatting and file selection, not delivery of an edited text value to a plugin.
The native string/property-change bridge still needs a verified implementation
example or contract; changing only the unit number is not yet validated.

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

The action dialog is covered by Flutter tests for all four choices, cancel and
Escape, bank limits, invalid Lua schemas, stale actions and stable geometry.
Full Helper suite: 3921 tests passed, with a clean analyzer.
The running macOS app opens and cancels the updated SD Lua dialog; the sync
indicator remains at the top left. Native sanitizer tests, ARM object
inspection and strict-alignment Unicorn startup checks are separate from hardware
acceptance. No hardware acceptance is inferred solely from those tests.

Outstanding: full native text-property editing and field parity; end-of-chain
preset merge/lifecycle behavior; persistence of pending edits across editor/app
lifecycle changes; wider firmware acceptance and production release review.

Deferred rather than silently claimed complete: gear-first presentation, sorting
by gear/colour, group-based presentation, arbitrary GUID shared-state adapters,
and expander topology detection. Editable patch titles and manual companion load,
reload, Apply/Discard, and unsent-edit workflows were explicitly removed by the
owner and must not return as default UX.
