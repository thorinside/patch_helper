# Add selected AUX buses to Patch Helper

Label: wayfinder:map
Status: complete

## Destination

Plan and implement AUX bus records in the existing Patch Helper plugin and companion editor. A user can choose any of the 44 AUX buses, while the editor shows only added rows.

## Notes

- Request reported by the owner from Snailed on Discord: "an official request for the Patch Helper to support aux busses."
- The owner selected planning and implementation in this effort, and arbitrary AUX selection with added rows only.
- The owner approved + AUX immediately underneath Outputs; see the interaction decision below.
- The owner explicitly permits resolving the remaining decisions together before implementation, overriding the one-ticket-per-session limit for this effort.
- Execution is included once the relevant decisions are agreed. Charting itself produces the map and decision tickets.
- Use wayfinder, grilling, and domain-modeling for decisions. Use disting-nt-cpp-plugin-writer for implementation and compatibility checks.
- Tracker defaults to local Markdown because this repository has no configured tracker document. Child issues live in issues/. Status open with no assignee means unclaimed. Claim before work; resolve in an appended Answer. Blocked by lists sibling ticket numbers.
- Read only the map before choosing the first open, unblocked, unclaimed child by number. Read child tickets as needed.
- Preserve existing presets and parameter mappings. The current model records physical connections and does not route signals.
- The companion is helper/ThPh.lua; Flutter rendering and the shared client live in the adjacent nt_helper repository. Coordinate changes across both repositories.
- Keep completed work committed and pushed. A release needs its own approval.
- Implementation is complete in the plugin, Lua companion and adjacent Helper branch. Native sanitizers, ARM startup/object checks and cppcheck pass. Helper analysis is clean; its full 4,018-test suite and the final seven editor tests pass. Physical AUX save/recall remains unverified.

## Decisions so far

- [Define what an AUX row records](issues/01-aux-record-meaning.md): AUX 1 through AUX 44 document bus purpose using Description and Group.
- [Choose AUX row removal and native presentation](issues/02-aux-row-lifecycle.md): Explicit rows persist, removal deletes their data, and the NT supports viewing and selection.
- [Choose stable AUX identities and compatible storage](issues/03-aux-compatibility.md): Separate sparse AUX records and negotiated commands preserve physical identities and existing parameter mappings.
- [Agree the inline AUX picker and row presentation](issues/04-aux-editor-interaction.md): The owner approved the inline section, unused-bus picker, numeric order, and deletion confirmation.

- [Let Lua define table layout and remember resized columns](issues/05-lua-layout-and-widths.md): Lua owns column schemas and defaults; Helper renders reusable primitives and stores user widths locally.

## Not yet specified

The owner added Lua-defined table layout and remembered draggable column widths to this effort. No unresolved design questions. Implementation and verification proceed under the execution exception. Verify empty-row persistence, arbitrary bus identities, all 44 buses with legacy expanders, atomic invalid reads, stale writes, lost acknowledgements, deletion and re-add, native selection, and accessible desktop and compact editing.

## Out of scope

No additional exclusions agreed yet.
