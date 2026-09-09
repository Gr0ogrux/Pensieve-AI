# 3D Printed Case

`Pensieve AI.stl` is the ready-to-print export of the current wearable enclosure design, modeled in Tinkercad[cite: 1, 2].

## Design

- **Integrated alignment system:** Uses locating pins on the lid and mating holes on the base to ensure both halves line up squarely every time it snaps shut.
- **Magnetic latching:** Held closed with 3mm x 1mm neodymium disc magnets instead of screws, making battery checks and microSD access completely toolless[cite: 2].
- **Wearable bail:** Includes a solid integrated loop at the top end for a necklace cord or lanyard, keeping the pendant oriented face-forward rather than flipping around while worn[cite: 2].
- **Dedicated internal compartments:** An internal divider separates the board assembly from the battery compartment, holding each component securely to prevent internal rattles[cite: 2].
- **Acoustic and control cutouts:** Features an acoustic port aligned with the Sense daughterboard's PDM MEMS microphone capsule and dedicated clearances for the push button and USB-C port[cite: 2].

## Before printing

**Check your component dimensions first[cite: 2].** This enclosure was designed specifically around:
- Seeed Studio XIAO ESP32-S3 Sense[cite: 2]
- MakerHawk 803040 LiPo battery[cite: 2]
- 6x6mm tactile push button[cite: 2]
- 3mm x 1mm neodymium disc magnets

Tolerances are tuned around these specific physical parts[cite: 2]. If you use a different battery size or button style, you may need to import the STL into your CAD software to tweak internal clearances.

**Dry-fit everything before final assembly[cite: 2].** Test-fit the stacked XIAO board, battery, button, and magnets inside the shell[cite: 2]. Running a quick test print before final assembly ensures your printer’s calibration and shrinkage rates match the designed tolerances[cite: 2].

## Recommended print settings

- **Material:** PETG for the daily wearable version (more durable and temperature-resistant for everyday carry); PLA works well for rapid test prints[cite: 2].
- **Layer height:** 0.16–0.20 mm[cite: 2].
- **Walls / Perimeters:** 3 perimeters minimum (keeps the shell rigid under pocket or pendant use)[cite: 2].
- **Infill:** 15–20% (Gyroid or Grid)[cite: 2].
- **Supports:** None needed when oriented with the flat surfaces against the build plate[cite: 2].
- **Tolerances note:** PETG flows and flexes slightly differently than PLA, so test your pin-and-hole friction fit and magnet pockets after cooling[cite: 2].

## Magnet polarity

The STL file cannot encode magnet polarity[cite: 2]. Before applying superglue or epoxy into the magnet pockets, place a base magnet and lid magnet together to verify attraction[cite: 2]. If they repel, flip one side over before setting them into the recesses[cite: 2].