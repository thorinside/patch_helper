# Patch Helper

<!-- impeccable:product-schema 1 -->

## Platform

adaptive

The product spans a native disting NT display and the existing Flutter NT
Helper app on desktop and mobile. Helper currently uses its Material theme
across platforms. This development preview provides desktop and compact Helper
slot layouts.

## Users

Disting NT musicians manually recording their physical cables while patching,
then looking up those records when returning to a saved preset.

## Product Purpose

Keep a physical connection map with its NT preset. The map describes
intended cabling; it does not detect connections or route audio/CV.

## Operating Context

The NT owns persistent map state. Helper offers the larger keyboard-driven
editing surface. Live edits require acknowledgement; saving the NT preset is a
separate operation. The user requested visual prototypes before implementation
on 2026-09-27, after approving the merge of revision 2.

## Capabilities and Constraints

The [V1 spec](docs/v1-spec.md) records the current baseline and open acceptance
items. The editor shows 12 inputs, 8 outputs and eight outputs per expander,
with destination, cable colour, optional numeric Tag (1–12), and group.
Destination supports 63 printable ASCII characters; group and expander names
support 31. Blank destination means unused and preserves other metadata.
New maps support 12 banks; legacy 13-bank maps retain their records.

The host executes `/programs/helper/ThPh.lua` from the NT SD card. Loading,
scratch caching, background source refresh and field synchronization are
automatic. Valid edits appear immediately and reconcile until acknowledged
while the editor session remains open; there is no durable offline outbox yet.
There are no patch-title, Load, Reload, Apply/Discard, or narrative help controls.

The NT currently has a colour/tag parameter page per socket, with dynamic page
updates. Full native field parity is required: destination, group and expander
name must use the NT's existing text-property editor, not a custom keyboard.
The C++ string/property-change bridge is still under investigation.

A one-second active watch supplies Lua `on_change` with authoritative map and
property changes. Local fields merge with unrelated native edits. Socket focus
updates use the minimap highlight without stealing keyboard focus. Loading,
sync, validation and error feedback must never shift the layout.

## Evidence on Hand

Source spec: Substrate d4abe223-d4c5-4784-811b-417aa43586ee, updated as V1.
The owner reported the connected build working well. Specific automated and
hardware checks, and outstanding acceptance, are listed in the V1 spec.
Code and wire fixtures in this repository and the adjacent nt_helper repository.
Helper's committed light/dark screenshots and current AppTheme establish the
incumbent visual language. Prototype cable names are illustrative sample data.

## Product Principles

- Make the physical socket and recorded destination unambiguous.
- Keep editing fast while retaining enough list context to avoid the wrong jack.
- Distinguish sent-to-device state from saved-preset state.
- Reconcile uncertain edits automatically against authoritative device state.

## Approved Editor and Remaining Scope

The user selected a straight table with a coloured clickable socket minimap.
Inputs are 3 rows × 4 columns; outputs 4 rows × 2 columns; each expander
8 rows × 1 column. Groups appear left to right in that order. The editor
offers NTX-8CV, ES-5, ESX-8GT and ESX-8CV. Selecting a coloured minimap dot
scrolls to and highlights its table row. Native text-property editing remains required for V1 parity. End-of-chain
preset merging remains unverified. Gear-sorted presentation and adapters for
additional algorithms are deferred.

The user explicitly requires Flutter visual quality to remain as polished as
the approved prototype; Lua is not a presentation limitation. Tag is a plain
optional integer input rather than a dropdown.

Sync status remains at the top left. Actions sit at the top right; Add expander
opens a Lua-defined model-choice dialog rendered with Flutter. Model selection
is not permanently visible. Cancel and Escape make no change.
