# Development contract

## Source and scope

Started from Substrate spec `d4abe223-d4c5-4784-811b-417aa43586ee`,
**Disting NT physical connection map and Helper companion interface**.
The user authorized beginning development in this repository and supporting
work in `../nt_helper` on 2026-09-27. The spec remains an unapproved discovery
draft; this slice does not imply that its remaining decisions are settled.

This first slice proves a preset-data boundary: native socket inventory,
validation, atomic decoding, serialization, an NT display, and matching Helper
code. It does not yet provide an end-to-end patch editing workflow.

## Verified API findings

The official [distingNT_API](https://github.com/expertsleepersltd/distingNT_API)
is pinned at `9aeda6d41484815b80416b903a564510da6026cd` (API v13).
`_NT_factory` exposes `serialise`, `deserialise`, and `parameterString`.
The last callback formats numeric parameters; it is not an arbitrary writable
string property. There is no factory callback for native string editing in
this revision. Preset callbacks support custom state, and `midiSysEx` provides
the custom USB transport specified in [live-map-protocol.md](live-map-protocol.md).

Helper has a built-in algorithm string-write path. Its bundled Lua controllers
are pure projections of immutable slot snapshots and currently expose numeric
controls. Neither fact demonstrates arbitrary plug-in string writes or shared
state. Do not add a string widget and claim it solves this limitation.

## Prototype decisions

- Provisional algorithm GUID: `ThPh`. No collision found in the inspected local
  plugin sources or Helper algorithm metadata. Check the community index before
  first release; preserve the GUID after release.
- Custom slot member: `patch_helper`, version 1. See the shared fixture.
- Twenty native sockets, IDs 0–11 for inputs 1–12, 12–19 for outputs 1–8.
  The array must contain each ID exactly once, in any input order. Writers
  normalize it into socket order. No auxiliary bus is a physical socket.
- Required map fields: `version`, `title`, `connections`. Required connection
  fields: `socket`, `destination`, `colour`, `tag`, `group`.
- Colour IDs 0–11: None, Black, White, Grey, Red, Orange, Yellow, Green, Blue,
  Purple, Pink, Brown. Tag 0 means absent; 1–12 are the optional visible tags.
- Title/destination buffers are 64 bytes including terminator, aligned with
  the API's 64-byte display-string boundary. Group buffers are 32 bytes.
  Printable ASCII only in this preview; these are explicit prototype memory
  and display choices, not claimed firmware text limits. Revisit before UI
  design and release. Both implementations reject unsupported input.
- Empty destination means unused; metadata remains intact. Groups are stored
  without providing grouping UI yet. The palette and retention policy remain
  reviewable development defaults, not owner-approved spec decisions.
- Missing custom state initializes a default map. Malformed present state,
  unknown versions/fields, and duplicate sockets fail closed. Decode into a
  candidate first, so failure cannot partially replace a valid map. Unrelated
  firmware-owned slot properties are skipped/preserved.
- All persistent state is per-instance SRAM requested from the host. Audio
  processing is an empty callback. No global mutable map, routing parameters,
  filesystem I/O, or audio-thread allocation exists.

## Build and verification

Requirements: ARM GCC, Python 3, a C++17 native compiler, and nlohmann/json
headers for native tests only (`brew install nlohmann-json`, or your platform
equivalent). No JSON library is linked into the ARM object.

```sh
git submodule update --init
python3 -m pip install -r tools/requirements-arm-smoke.txt
make verify
# Override only when headers are outside the compiler's normal search path:
make test JSON_INCLUDE=/custom/include
```

Native tests use AddressSanitizer/UndefinedBehaviorSanitizer and a desktop
adapter for the firmware JSON cursor API. They exercise the actual factory
callbacks, malformed-state rejection and atomicity, fixture round trips,
socket identity, display bounds, and unchanged audio/CV buffers. The adapter
does not prove firmware parser or physical preset lifecycle behavior.

`make inspect` verifies ELF32 little-endian ARM relocatable output, exported
`pluginEntry`, and an allowlist of runtime and pinned API imports. Section
sizes and demangled imports are printed for review. `step` projects native
controls when needed and never reads or writes the audio/CV buffer.
Deserialization uses the instance-owned scratch map, not a whole map on stack.

`make arm-smoke` runs the built ARM object under Unicorn with strict data
alignment checks, explicit host-function stubs, halfword-aligned parameter
storage, and both zeroed and poisoned instance memory. It runs at two code/data
address layouts and rejects instance reads/writes beyond the declared SRAM size,
including accesses performed by stubbed `memcpy` and `memset`. It exercises construction,
initial parameter callbacks, the first `step`, and `draw`. This is not an NT
firmware/ELF-loader emulator and does not replace hardware acceptance.

The 2026-09-27 build-flags audit compared the skill's `templates/Makefile` and
pinned API `examples/Makefile`: Cortex-M7, FPv5-D16 hard float, Thumb, `-Os`,
`-fPIC`, no RTTI/exceptions, function/data sections, and no unwind tables.
C++17 is required by our source; a single compilation unit produces the
relocatable object directly with `-c`, matching the upstream example approach.
`-mno-unaligned-access` is an additional hardware safeguard: the earlier object
failed the ARM startup test while reading its default title with an unaligned
word load. Makefile changes invalidate the hardware object to prevent stale
flags. The import allowlist is observed compatibility, not a complete firmware
export catalogue; the device reported `memcmp` as unavailable.

The follow-up allocation/PIC audit found no heap allocation in the ARM object.
`requirements()` declares the entire instance, including both maps and mutable
parameter definitions. Placement `new` initializes that host-supplied block;
it does not allocate from a heap. The hardware translation unit rejects builds
without `-fPIC`. A colour table previously had external inline linkage, producing
GOT relocations that the GNU linker in the smoke test resolved automatically.
The table now has internal linkage, retaining PIC while eliminating the GOT.
Object inspection rejects GOT dependencies and unexpected imports (including
heap allocators). The previous alignment build still crashed on the physical NT;
GOT elimination is a loader-compatibility hypothesis, not confirmed causation.

`make static-check` runs cppcheck. It suppresses only two warning categories
in the unmodified upstream headers: host-owned aggregate members without a
constructor, and the upstream private non-explicit JSON-stream constructor.
Plugin warnings remain errors. No production release is created by CI.

The fixture is also stored in nt_helper at
`test/fixtures/patch_map/native-map.json`. Keep both copies identical when the
format changes. Helper's `PatchMapPresetCodec` accepts one full slot object;
it validates GUID `ThPh` and preserves the rest of that slot on writes.
The direct placement of `patch_helper` in that slot is a provisional integration
contract. Confirm the firmware's actual custom-data envelope from an exported
device preset before wiring this codec into user-facing preset operations.
The native harness proves the callback payload, not that outer envelope.

## Next integration slices

1. Verify the revision-2 USB bridge on hardware: callback dispatch, USB reply
   delivery, preset replacement, and dirty/preset-save semantics. Automated
   tests cover wire compatibility, conflicts, and interrupted transfers.
2. Implement the NT text-entry workflow against real API capabilities; expose
   title/destination/metadata edits and retain native preset ownership.
3. Add Helper state snapshots and host-owned declarative write actions, then
   the connection-map editor (gear/socket views). Avoid embedding raw MIDI or
   a mutable second state store in Lua.
4. Resolve expander models/topology, naming, limits, sorting, grouping UI, and
   ordinary versus end-of-chain preset behavior with the owner.
5. Treat SD-card companion discovery/execution as a separate framework slice:
   manifest/version compatibility, trust, isolated execution budgets, loading,
   removal, and failure fallback. Existing bundled controllers do not provide
   a sandbox for arbitrary downloaded Lua.

No production tag, device deployment, or Substrate approval has been performed.

## Editor revision supersedes the initial-slice limitations

The SD-card host Lua requirement is implemented in this revision; see
[companion-contract.md](companion-contract.md). Native-only presets retain
version 1. Version 2 adds named expander objects and eight connection records
per instance. Moves preserve each bank's cable data. The map and deserialization
scratch space are per-instance host-allocated SRAM, about 27 KiB combined.
ARM stack inspection found deserialization at 48 bytes plus its bounded parser
calls (largest individual frame 320 bytes), not a full expanded map on stack.
The audio callback updates native page count and the legacy selector range when inventory
changes; it leaves all audio/CV buffers untouched. Older initial-slice notes
above describe revision 1 and must not be used to defer the Lua companion again.


## Owner hardware check, 2026-09-27

The owner confirmed that revision `275e282` loads, enables, and no longer
crashes the NT. Loading the companion then timed out in the plug-in handshake.
A direct SD download confirmed `/programs/helper/ThPh.lua` matched the repository
source exactly. The reply omitted `F0`; the native host stub had incorrectly
accepted that. Tests now require the opening delimiter and compare complete
wire frames. The corrected callback also accepts independently retained input
delimiters. Companion/map round-trip acceptance remains a separate check.


## Per-socket native pages

The native editor now exposes one page per active socket with independent colour
and tag parameters. Definitions/page arrays are instance-owned fixed SRAM,
reserved up front; no post-construction allocation or GOT is introduced. The
three old parameter indices are retained but hidden from pages. New parameters
append indices 3–250. `NT_updateParameterPages()` notifies the host when expander
inventory changes. Native and ARM startup tests cover all parameter callbacks,
page indices, independent rows, maximum bank bounds and the host notification.
