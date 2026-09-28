# Development live-map protocol (revision 2)

The plug-in remains the source of truth. Helper reads a complete map before
editing, sends one complete connection or title per action, and updates its
snapshot only after acknowledgement. A failed or uncertain write requires a
full reload. Nothing is automatically replayed after a timeout or receive error.
This is a development USB bridge, not a claimed firmware feature or release.

## Addressing and frame

Use a direct USB connection to one NT. The experimental/non-commercial MIDI
manufacturer ID `7D` is deliberately separate from Expert Sleepers' commands.
Do not route this protocol to multiple NTs on the same MIDI endpoint: the public
plug-in API does not expose the configured device SysEx ID or incoming port.
Replies go to USB only. `NT_sendMidiSysEx` receives the opening `F0` from
the plug-in and appends `F7` when `end=true`. Incoming callback data tolerates
either delimiter being retained or omitted. Native tests compare complete
wire replies, including both delimiters.

Every request is `F0 <header> <payload> F7`. The 20-byte header is:

| Offset | Bytes | Meaning |
|---|---|---|
| 0 | 6 | `7D 54 68 50 68 01` (ThPh, protocol version 1) |
| 6 | 1 | Command: 1 open, 2 read connection, 3 write connection, 4 write title |
| 7 | 1 | Zero-based current algorithm slot, whose GUID must be ThPh |
| 8 | 4 | Request ID, echoed by the reply |
| 12 | 4 | Nonzero editing lease, chosen by the client |
| 16 | 4 | Expected map revision |

Integers use four little-endian base-128 bytes (28 bits). All bytes inside the
MIDI delimiters are 7-bit. The response echoes the header with command OR `40`,
replaces revision with the current revision, then adds one status byte and data.
Status is 0 success, 1 invalid request, 2 expired session, 3 revision conflict.
The largest valid frame is 122 bytes, comfortably below Helper's 1024-byte limit.
Wrong prefixes, unsupported commands, invalid frames, and nonmatching slots are
ignored. Valid requests with invalid payloads return status 1 without mutation.

Text is a one-byte length followed by printable ASCII bytes (no terminator).
Limits remain 63 characters for title/destination and 31 for group; these are
explicit development format choices. Reject invalid text rather than truncate.

| Command | Request payload | Successful response data |
|---|---|---|
| Open | empty | title text |
| Read connection | socket ID | socket ID, colour, tag, destination text, group text |
| Write connection | socket ID, colour, tag, destination text, group text | empty |
| Write title | title text | empty |

Sockets, palette, tags, empty-destination semantics, and preset data version
remain as specified in `development.md`. Writes replace one record atomically,
including its metadata. A title write does not alter connections.

## Session and conflict behavior

Open installs a new lease and returns the current revision and title. It does
not modify the map. Opening another editor invalidates the previous editor's
lease. Helper uses a random initial lease, advances it for each reload, and
matches replies against prefix, command, slot, request ID, and lease.

Every read after open must use the same revision. Helper publishes the loaded
snapshot only after all 20 records arrive, so a conflicting read cannot produce
a mixed map. Each successful write increments revision exactly once. Duplicate
writes carrying an old revision are rejected. Requests are serialized with
ordinary firmware traffic through Helper's existing MIDI scheduler.

Construction or successful preset deserialization clears the lease and
revision. A stale editor cannot modify a newly loaded preset, even if the slot
index and GUID are the same. Failed deserialization preserves both state and
session. At revision exhaustion, writes fail until a new open resets revision;
this cannot silently wrap while retaining the previous lease.

After a timeout, malformed reply, conflict, or expiry, Helper disables further
writes until a complete reload. That reload shows whether an unacknowledged
write reached the NT. The bridge does not save the preset to the SD card.
The user must use the normal Save preset workflow; automatic dirty marking and
physical save/reload behavior remain device acceptance items.

## Evidence and next step

`tests/fixtures/midi-session.json` is identical to Helper's
`test/fixtures/patch_map/midi-session.json`. C++ tests exercise the actual
factory callback against its requests and replies. Dart tests drive the client
through the same exchange, plus conflicts and lost acknowledgements. Scheduler
tests cover shared-queue ordering, fragmented replies, endpoint filtering, and
no write replay after receive errors.

The native object and tests establish a transport foundation. Helper's visible
editor is the next part of revision 2; this document does not claim a completed
editing UI. On-device text editing, expander topology, companion Lua loading,
and physical-device acceptance remain separate work.

## Editor-compatible extension (revision 3)

The prefix remains protocol 1. Extended Open sends payload `02`; its reply is
`title text, expander count`. Legacy empty Open still returns only title.
Old binaries reject extended Open, so Helper reports that the plug-in needs
updating rather than silently omitting expanders.

Additional commands:

| Command | Request | Reply data |
|---|---|---|
| 5 Add expander | type, name text | empty |
| 6 Read expander | zero-based index | type, name text |
| 7 Rename expander | index, name text | empty |
| 8 Move expander | from index, to index | empty |

Types 0–3 are NTX-8CV, ES-5, ESX-8GT, ESX-8CV. Names are at most 31 printable
ASCII characters. Command 6 is a read; 5/7/8 increment revision once on success.
A move shifts complete eight-socket banks and their metadata atomically. Native
IDs stay 0–19; expander IDs follow in physical list order. Capacity is thirteen
banks, derived from the 7-bit socket ID. Invalid payloads and overflow fail
without mutation. Maximum frame size stays 122 bytes.

Helper reads title/count, every expander, and every configured socket under one
lease/revision before exposing a map. Preset version 2 requires an `expanders`
array of `{type, name}` objects and exactly 20 + 8×count connection records.
Version 1 remains readable and is still written for maps without expanders.
The shared expanded-session fixture exercises Lua-dispatched edits against both
Dart and the actual C++ factory, including rename, move and reload.

## Revision 4 watch request

Command `09`, empty payload, checks the lease but deliberately accepts an older
revision. Reply header carries current map revision; payload is title text,
expander count, one-based First socket, cable colour index, tag. All four numeric
fields occupy one MIDI data byte each. It is read-only, does not advance revision,
and never opens a lease. Trailing request bytes are invalid. A changed revision
requires rereading banks/connections at that revision; conflict fails closed.

Native colour/tag edits advance the same map revision as Helper writes. At
revision exhaustion they invalidate the lease before resetting the revision.
Selection-only changes do not mutate the map revision. `live-session.json`
contains read frames interleaved with native parameter callbacks and is exercised
by the actual C++ factory and the Dart client/Lua companion tests.
