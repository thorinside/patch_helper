# Agree the inline AUX picker and row presentation

Parent: ../map.md
Label: wayfinder:prototype
Type: prototype
Status: resolved
Assignee: Codex
Blocked by: 01, 02

## Question

How does + underneath Outputs add an arbitrary unused AUX bus without making the editor crowded? Use a rough prototype for the owner to react to. Decide picker labels, duplicate prevention, row order, removal affordance, compact layout, and minimap representation.

Lua currently declares contiguous start/count groups. Its sole choice-dialog action is add_expander in the top toolbar. Inline actions and selected noncontiguous AUX buses need coordinated Lua contract and Flutter changes. Link prototype assets from this ticket and record the live human feedback before resolution.

## Answer

The owner approved the interaction in [the AUX interaction sketch](../assets/aux-interaction-prototype.md): an AUX buses section immediately after Outputs, + AUX selecting an unused bus, numeric row order, Description and Group fields, and Remove. Blank rows persist. Removing populated rows asks for confirmation. The existing Material table and choice-dialog styling remain. Minimap markers use the selected AUX numbers without suggesting physical cable colours.
