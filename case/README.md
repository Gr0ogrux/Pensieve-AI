# 3D Printed Case

`pendant_case_v4.scad` is the parametric source (OpenSCAD). `base_shell_v9.stl`
and `lid_shell_v9.stl` are ready-to-print exports of the current design.

## Design

- Two-piece design: an open-top base holding the board and battery,
  and a lid with the mic grill and button cutout
- Held closed with 4x 4mm x 1mm neodymium magnets (2 per half) rather
  than screws — no tools needed to open it
- A solid printed loop (bail) for the necklace cord, positioned at one
  end so the pendant hangs from a stable point rather than flipping
  around on the cord
- Internal divider separates the board and battery pockets, sized to
  each component individually (not one oversized shared pocket)
- Small standoff with relief holes for the push button's legs

## Before printing

**Measure your own parts first.** This design was built around one
specific set of components (XIAO ESP32-S3 Sense, a MakerHawk 803040
battery, a 6x6mm tactile button) with real caliper measurements, not
estimates. If you're using different components, the pocket dimensions
in the `.scad` file's CONFIG section at the top will need adjusting —
they're all named variables, not magic numbers buried in the geometry.

**Print the base alone first and dry-fit everything** — board, battery,
button, magnets — before committing to printing the lid or doing a full
production run. This design went through several iterations that looked
fine on screen but had real fit issues once actually printed; a cheap
test print catches that early.

## Recommended print settings

- **Material**: PETG for the final piece (more durable for something
  worn daily), PLA is fine for test prints
- **Layer height**: 0.16-0.2mm
- **Walls**: 3 perimeters minimum (the shell walls are only ~2.2mm thick)
- **Infill**: 15-20%
- **Supports**: none needed if printed with the open face up
- If switching to PETG, consider slightly loosening the magnet pocket
  tolerances — PETG's slight flexibility can make press-fits feel
  tighter than the same dimensions in PLA

## Magnet polarity

The STL/SCAD can't encode magnet polarity. Before gluing magnets in,
hold a base magnet and a lid magnet near each other — if they repel,
flip one over.

## Regenerating from source

```
openscad -o base_shell.stl -D 'part="base"' pendant_case_v4.scad
openscad -o lid_shell.stl  -D 'part="lid"'  pendant_case_v4.scad
```

Set `part = "preview"` (edit near the bottom of the file) to see both
halves overlaid in OpenSCAD's own viewer.
