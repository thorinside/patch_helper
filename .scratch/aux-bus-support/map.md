# Add selected AUX buses to Patch Helper

Label: wayfinder:map
Status: open

## Destination

Plan and implement AUX bus records in the existing Patch Helper plugin and companion editor. A user can choose any of the 44 AUX buses, while the editor shows only added rows.

## Notes

- Request reported by the owner from Snailed on Discord: "an official request for the Patch Helper to support aux busses."
- The owner selected planning and implementation in this effort, and arbitrary AUX selection with added rows only.
- The proposed entry point is a + button underneath Outputs. The exact interaction is pending discussion.
- Execution is included once the relevant decisions are agreed. Charting itself produces the map and decision tickets.
- Use wayfinder, grilling, and domain-modeling for decisions. Use disting-nt-cpp-plugin-writer for implementation and compatibility checks.
- Tracker defaults to local Markdown because this repository has no configured tracker document. Child issues live in issues/. Status open with no assignee means unclaimed. Claim before work; resolve in an appended Answer. Blocked by lists sibling ticket numbers.
- Read only the map before choosing the first open, unblocked, unclaimed child by number. Read child tickets as needed.
- Preserve existing presets and parameter mappings. The current model records physical connections and does not route signals.
- The companion is helper/ThPh.lua; Flutter rendering and the shared client live in the adjacent nt_helper repository. Coordinate changes across both repositories.
- Keep completed work committed and pushed. A release needs its own approval.

## Decisions so far

No decision tickets resolved during charting. The destination and intake choices are recorded above.

## Not yet specified

Acceptance scenarios and delivery steps will become concrete after AUX fields, row lifecycle, native presentation, and compatibility are settled.

## Out of scope

No additional exclusions agreed yet.
