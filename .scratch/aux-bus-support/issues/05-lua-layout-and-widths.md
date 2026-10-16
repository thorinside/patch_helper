# Let Lua define table layout and remember resized columns

Parent: ../map.md
Label: wayfinder:grilling
Type: grilling
Status: resolved
Assignee: Codex
Blocked by: none

## Question

Which parts of table behavior belong to Lua, and must this effort include remembered column resizing?

## Answer

The owner wants Lua to specify table layout and Helper to render it. The owner explicitly chose including resizable columns with remembered widths now.

Lua declares stable table and column IDs, order, labels, cell types, default widths, bounds, and record sections with inline actions. Helper renders the reusable declarations and handles focus, drag gestures, keyboard resizing and accessibility. Widths stay in host preferences keyed by companion GUID, table ID and column ID, independent of patch presets and labels. Saved widths are clamped to the current schema. Algorithm-specific record validation and MIDI remain in the Patch Helper adapter.

AUX is the first consumer of these record-section primitives. It does not introduce an AUX-specific rendering widget. Existing companions without column declarations retain their current default layout.
