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

Keep a named physical connection map with its NT preset. The map describes
intended cabling; it does not detect connections or route audio/CV.

## Operating Context

The NT owns persistent map state. Helper offers the larger keyboard-driven
editing surface. Live edits require acknowledgement; saving the NT preset is a
separate operation. The user requested visual prototypes before implementation
on 2026-09-27, after approving the merge of revision 2.

## Capabilities and Constraints

The current development format covers 12 inputs and 8 outputs, one destination
per socket, a finite cable-colour palette, optional Tag 1–12, and optional group.
Each manually added expander contributes eight outputs; repeated types are
supported, with editable names and ordering.
An empty destination means unused. Unused sockets remain visible by default.
Destination-based and socket-based views are in the discovery spec. The title
and destination currently accept 63 printable ASCII characters, group 31.

Revision 3 implements the Helper editor as a host-executed SD-card Lua
companion at `/helper/ThPh.lua`, backed by the USB map bridge. The file is
discovered by plug-in GUID. It includes named, ordered expander banks and the
approved table/minimap. Flutter renders the controls in Helper's existing theme.
On-device text entry remains pending. Physical device acceptance and the
exported preset envelope are unverified. Prototype interactions beyond current capability must be
identified as proposals, not demonstrated as shipped behavior.

## Evidence on Hand

Source spec: Substrate d4abe223-d4c5-4784-811b-417aa43586ee, a discovery draft.
Code and wire fixtures in this repository and the adjacent nt_helper repository.
Helper's committed light/dark screenshots and current AppTheme establish the
incumbent visual language. Prototype cable names are illustrative sample data.

## Product Principles

- Make the physical socket and recorded destination unambiguous.
- Keep editing fast while retaining enough list context to avoid the wrong jack.
- Distinguish sent-to-device state from saved-preset state.
- Recover from uncertain edits by reloading authoritative device state.

## Approved Editor and Remaining Scope

The user selected a straight table with a coloured clickable socket minimap.
Inputs are 3 rows × 4 columns; outputs 4 rows × 2 columns; each expander
8 rows × 1 column. Groups appear left to right in that order. The editor
offers NTX-8CV, ES-5, ESX-8GT and ESX-8CV. Selecting a coloured minimap dot
scrolls to and highlights its table row. Native text entry, the gear-sorted view,
ordinary/end-of-chain preset lifecycle acceptance, and shared-state adapters for
additional algorithms remain separate work.

The user explicitly requires Flutter visual quality to remain as polished as
the approved prototype; Lua is not a presentation limitation. Tag is a plain
optional integer input rather than a dropdown.
