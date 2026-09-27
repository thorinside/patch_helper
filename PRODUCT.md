# Patch Helper

<!-- impeccable:product-schema 1 -->

## Platform

adaptive

The product spans a native disting NT display and the existing Flutter NT
Helper app on desktop and mobile. Helper currently uses its Material theme
across platforms. This prototype round targets the desktop Helper slot editor;
it does not define new platform-specific mobile layouts.

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
An empty destination means unused. Unused sockets remain visible by default.
Destination-based and socket-based views are in the discovery spec. The title
and destination currently accept 63 printable ASCII characters, group 31.

Revision 2 implements a live USB bridge and Helper client. It does not yet
implement the visible editor, on-device text entry, expanders, or SD-card Lua
companion loading. Physical device acceptance and the exported preset envelope
are unverified. Prototype interactions beyond current capability must be
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

## Open Decisions

The user selected a straight table with a coloured clickable socket minimap.
Inputs are 3 rows × 4 columns; outputs 4 rows × 2 columns; each expander
8 rows × 1 column. Groups appear left to right in that order. The prototype
offers NTX-8CV, ES-5, ESX-8GT and ESX-8CV. Actual expander persistence and
transport, native text entry, ordinary/end-of-chain preset behavior, and the
companion Lua framework remain separate implementation work.
