# Table and socket minimap study

Open `table-minimap.prototype.html` directly in a browser, or run
`python3 -m http.server 55191 --bind 127.0.0.1 --directory docs/prototypes`
from the repository root and open
http://127.0.0.1:55191/table-minimap.prototype.html.

This is a throwaway browser prototype for evaluating the Helper editor, with
sample data and memory-only editing. It does not communicate with hardware.
The merged model and transport still cover only the 20 native sockets.

## Confirmed direction

The user rejected Ledger, Socket Overview, and I/O Lanes. Those images remain
under `.impeccable/mocks/decision/` as rejected exploration, not approved designs.
The subsequent straight-table image is superseded by this interactive study.

Keep one table: 12 inputs, 8 native outputs, then each expander's eight outputs.
The user explicitly clarified the minimap arrangement on 2026-09-27:

- Inputs: 3 rows of 4 columns.
- Native outputs: 4 rows of 2 columns.
- Each expander: 8 rows of 1 column.
- Groups run left to right in that order.
- Coloured, clickable dots scroll to and highlight the matching table row.

The sample starts with NTX-8CV and ES-5. The selector also offers ESX-8GT and
ESX-8CV. These represent physical cable labels, not electronic chain setup.

## Direction contract

THESIS: A continuous editable table with spatial socket navigation makes long
patch records easy to locate without changing their order.

OWN-WORLD: Inherit Helper's restrained Material-like surfaces and system type.
Cable colours are data. This HTML study approximates the host, not a new theme.

STORY: Identify a jack, click its dot, inspect the highlighted row, edit its
record. Empty records stay visible and use hollow dots.

FIRST VIEWPORT: Table dominates the desktop, minimap stays in its upper right
rail. Narrow views put the collapsible map above the horizontally scrollable
table. All groups retain their geometry and order.

FORM: User-pinned table and minimap; no randomized alternative applies. The
initial seed and rejected concepts are history only.

FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance

## Verification

Browser checks: exact grid dimensions; expander 2 output 8 click selects and
scrolls its row fully into view; Enter activates input 3; ESX-8GT and ESX-8CV each
append eight rows and a minimap column; reset restores the 36-socket sample.
Desktop (1280×800) and compact (707px) screenshots are in `evidence/`.

The static detector reported the table wrapper as unpadded; its cells have their
own insets, as a continuous table requires. Palette advisories compare a temporary
HTML approximation with the limited incumbent reference, not a complete Flutter
tonal ramp. This study does not change Helper's theme.

Documentation review checked the prototype's styles and interaction source
against this direction contract, `PRODUCT.md`, `DESIGN.md`, and Helper's current
`app_theme.dart`. The table order and minimap geometry match the study's scope.
The incumbent design reference remains unchanged: this temporary dark HTML
palette and compact type sizes are not approved application tokens, and no new
design system or synthetic tonal ramp is introduced. Production Material
controls and light/high-contrast treatment remain implementation work.

## Hardware references

- [NTX-8CV](https://www.expert-sleepers.co.uk/ntx8cv.html)
- [ES-5](https://www.expert-sleepers.co.uk/es5.html)
- [ESX-8GT](https://www.expert-sleepers.co.uk/esx8gtusermanual.html)
- [ESX-8CV](https://www.expert-sleepers.co.uk/esx8cv.html)
