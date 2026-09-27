# Approved editor direction

The interactive design study is preserved on the
[`feat/editor-prototypes` branch](https://github.com/thorinside/patch_helper/tree/feat/editor-prototypes/docs/prototypes).

The selected direction is a straight editable table: inputs, native outputs,
then each eight-output expander bank. A corner socket minimap uses coloured,
clickable dots to scroll to and highlight a row. Its groups run left to right:
inputs 3 rows × 4 columns, outputs 4 rows × 2 columns, each expander 8 rows × 1
column. Compact layouts have an expandable scrollable minimap and 48-pixel
socket targets. Tag is a plain integer field, 1–12 or blank.

Flutter renders the production controls using Helper's existing Material theme.
The Lua companion supplies sections, labels, grid geometry, and declarative
actions. See [the companion contract](companion-contract.md) for the implemented
boundary and remaining work. Current Flutter captures live in NT Helper under
`docs/evidence/patch-helper/`; they use illustrative records and a test device.
