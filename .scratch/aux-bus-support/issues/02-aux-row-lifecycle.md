# Choose AUX row removal and native presentation

Parent: ../map.md
Label: wayfinder:grilling
Type: grilling
Status: resolved
Assignee: Codex
Blocked by: none

## Question

When a user removes an AUX row, does its saved information remain available on re-add? Must AUX rows be visible, selectable, or editable on the NT itself? Define what an empty added row means and whether visibility is saved with the preset.

Native parameter definitions currently reserve 231 entries against the documented 240-entry connected-firmware ceiling. Permanent colour and tag pairs for all 44 AUX rows do not fit the current layout.

## Comments

Owner answers received on 2026-10-15:

- Removing a row deletes the row and its information.
- AUX rows are viewable and selectable on the NT. Text editing remains in Helper.

These are direct human choices, preserved here for the next ticket session. This ticket remains open under wayfinder's one-resolution-per-session rule. Its remaining details are empty-row behavior and saved visibility; propose retaining an explicitly added empty row until removal and saving the added-bus inventory with the preset.

## Answer

The owner chose deletion of the row and its information, and native NT viewing and selection with text editing in Helper. The owner also approved saving explicitly added blank rows with the preset until removal. Confirm deletion for a row containing either Description or Group; remove blank rows directly. Re-adding a removed bus starts empty.
