Historical brainstorming capture; superseded by [V1](v1-spec.md).

Source: Substrate d4abe223-d4c5-4784-811b-417aa43586ee
Retrieved 2026-09-27. User's later table/minimap approval refines the layout; SD-card host Lua remains required.

# Disting NT physical connection map

## Status and destination
Owner-authorized capture of a voice brainstorming session. Draft, not approved for implementation. Separate project is likely but undecided; no project created. Build a simple, manageable reference for physical connections involving disting NT and its expanders, not a routing engine.

## Motivation
Neal likes Witch Board's external-effects possibilities but wants an inverse, descriptive view of cables already attached: cable colours, sockets, destinations, and availability without needing to remember a preplanned patch. Repeated cable colours need disambiguation.

## Confirmed scope
Only connections involving NT and its expanders. Exclude arbitrary connections between other equipment. Document physical connections only; do not change signal routing. Existing internal routing page remains responsible for internal signal-flow display. No automatic connection detection: all entries are human-maintained. A saved map describes intended/recorded connections, not verified physical connectivity.

## State and preset ownership
One current map, not a separate library of named maps. The map has an editable title. Owner explicitly revised the earlier device-global idea: all map state should be stored in preset state, making connections visible when recalling a preset. Both regular and end-of-chain presets are relevant; exact lifecycle and merge semantics need investigation. NT owns durable state; Helper accesses/edits it.

## User experience
Default Helper view emphasises connected gear; offer switch to a socket-oriented view. Simple dropdowns and plus buttons, avoiding excessive editable fields. Initial proposed add flow accepted: choose connected gear then cable colour and socket; later simplification explicitly replaces separate gear-name and remote-socket fields with ONE destination text field such as 'reverb left'. Final interaction details remain to reconcile with this simplification.

Show all twelve native NT inputs and eight native outputs automatically. Show unused sockets by default. Destination nonempty means connected/assigned; clearing destination means unused. This is a manual record only.

NT display: compact list of several connections, showing socket, colour, destination. Both NT and Helper should support editing, using appropriate property types and native string editing where available. This supersedes an earlier Helper-only editing preference. Helper is the easier primary editor. Sorting options desired (socket, colour, connected gear were proposed); exact sort keys and controls remain open.

## Connection data
Each connection is separate, including left/right stereo connections. Identified local socket, one editable destination text field, cable colour from a reasonably broad but finite common Eurorack palette, optional numeric 'Tag' from one to twelve, and optional group name. Tag is deliberately generic: do not call it layer. Tags distinguish repeated colours. Simple shared group names allow related cables to be displayed together; grouping is desired but first-version commitment should be confirmed. Avoid splitting destination into multiple fields.

## Expanders
Add expanders through dropdowns and plus controls; enumerate eight sockets per added expander. Distinguish CV-output and gate-output types. Multiple instances of the same type are required. Editable instance names and manual ordering to match physical rack layout were accepted. Mentioned devices: ESX-8CV, ES-5, NTX8CV, and eight-GT gate variants; exact supported models and topology must be verified rather than inferred from speech. Neal reports existing NTX8CV support in Helper. Do not confuse physical output roles with automatic detection.

## Ecosystem extension concept
An NT algorithm could ship a companion Lua extension installed alongside it on the SD card. nt_helper would load and run that Lua on the host application, not on the module. Neal reports existing Helper Lua integration manages algorithm properties but lacks richer shared state and SD-card Lua loading. These are owner-reported starting assumptions, not inspected facts. Investigate both companion discovery/loading and richer bidirectional state exchange. Connection mapping is the first proposed use case. Loading/security, compatibility, protocol, persistence, and error recovery are not yet designed.

## Engineering guidance
Registered skill: disting-nt-cpp-plugin-writer (thorinside/nt_mcp). Core guidance has been read. Inspect actual repositories, pinned API, native string/property support, preset serialization and Helper Lua interfaces before design commitments. Keep bounded real-time work and separate hardware validation from emulator/native evidence. No code or feasibility verification has been performed.

## Provisional acceptance criteria
- A new map shows twelve NT inputs and eight outputs, including unused sockets.
- Users can add supported eight-socket expanders, repeat types, name instances and reorder them.
- Users can edit destination, finite-palette colour, optional numeric Tag and potentially group on both NT and Helper using supported controls.
- Clearing destination marks a socket unused without claiming detected connectivity.
- Compact NT rows identify socket, colour and destination; Helper supports gear-oriented and socket-oriented views.
- Map title and connection state round-trip through the owning preset; regular/end-of-chain details must be specified before acceptance.
- The plugin never changes signal routing.
- Companion Lua and shared-state behavior must receive explicit feasibility evidence and acceptance criteria before implementation.

## Open decisions
Project identity/repository; exact expander models/topology and limits; palette; grouping version-one status; sorting defaults and fields; native editing/property representation; how empty destination interacts with retained colour/tag/group; title and length limits; preset recall and end-of-chain semantics; supported Lua discovery, transport, lifecycle and security; offline and failure behavior; scope split between connection-map plugin and reusable Helper extension framework.

