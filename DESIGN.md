---
name: Helper companion editor — incumbent reference
description: Existing NT Helper visual conventions for the Patch Helper editor
colors:
  default-seed: "#00BFA5"
typography:
  tab-label:
    fontSize: "16px"
    fontWeight: 600
---

## Overview

This is a scoped record of the existing Helper identity, not a redesign or a new
system for the NT firmware. The source of truth is nt_helper's
`lib/ui/theme/app_theme.dart` and its standard Material 3 widgets. Prototype
compositions may differ; the established component language stays fixed.

## Colors

Helper derives light, dark, and high-contrast schemes from the user's seed using
Material's vibrant scheme. The default seed above is observed in current code.
Use semantic surface/onSurface roles; cable colours are labeled domain data,
not competing interface accents. Dark prototype views represent the existing
low-light studio option, not a forced theme. The shipped editor must also inherit
light and high-contrast modes.

## Typography

Retain the app's platform/system text through Material typography. Body copy,
field labels, and destinations are ordinary UI text; do not introduce display
fonts or decorative monospace. Socket IDs align predictably for quick scanning.

## Layout

The incumbent desktop app has a narrow algorithm sidebar, preset context above
the main slot editor, slot actions at the top, and a compact bottom bar. The new
editor occupies the selected Patch Helper slot. It does not add a second app
navigation system. Use native text fields, menus, buttons, and clear focus order.

The implemented editor keeps the table beside a scrollable minimap at slot
widths of at least 1050 logical pixels. At narrower widths, the minimap opens
above the table and scrolls within a bounded height. The table retains its
columns through horizontal scrolling. This is a local editor layout, not a new
application-wide breakpoint system.

## Elevation & Depth

Helper uses tonal surfaces, section strips, and modest Material elevation. Avoid
invented glass, glows, decorative panels, and dashboard statistics.

## Components

Preserve text-labeled actions, conventional selected states, Material controls,
and clear disabled/loading/error states. An edit acknowledgement and a saved
preset are different states. Do not use a green connected indicator as proof of
physical cable detection. Keep colour names beside swatches and Tag labels.

The socket minimap uses numbered dots and a visible selected state. Selecting
a dot scrolls to and highlights its table row; reduced-motion settings disable
the scroll animation. Empty destinations show as unused. Tag is a plain
integer field, 1–12 or blank, with validation beside the affected row.

## Do's and Don'ts

Evidence: committed Helper screenshots under
`docs/evidence/algorithm-section-header/`, verified 2026-09-17; inspected again
for this round along with current theme source. They establish shell and control
character. Their chosen seed and old algorithm content are not new requirements.
Discard the black capture margins outside the app when composing prototypes.

Current native editor captures are in Helper's
`docs/evidence/patch-helper/editor-desktop.png` and `editor-compact.png`.
Their cable names are sample data; the captures demonstrate the Helper layout,
not physical-device acceptance.
