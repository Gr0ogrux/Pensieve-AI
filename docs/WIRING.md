# Wiring

## Button

Wire one leg of the button to pin **D0** on the XIAO board, and the
other leg to any **GND** pin. No resistor needed — the firmware uses
the board's internal pull-up resistor.

Most small tactile buttons have 4 legs that are really just 2
electrical pairs (diagonal legs are connected to each other internally).
You only need one leg from each pair — check with a multimeter's
continuity mode if you're not sure which legs pair together.

## Battery

**The XIAO ESP32-S3 does not have a battery connector** — only two bare
solder pads on the underside of the board, labeled **BAT+** and
**BAT-** (small text, use a magnifying glass or your phone's camera
zoomed in to confirm you're reading them correctly).

This means either:
- Soldering a matching connector onto the pads (if your battery has a
  detachable connector and you have a matching part), or
- Soldering the battery's wires directly to the pads (permanent, but
  requires no extra parts)

### If soldering directly to the pads

This involves a LiPo battery, which is less forgiving of mistakes than
low-voltage signal wiring. A few things that matter here specifically,
not just generally good soldering practice:

1. **Identify polarity carefully.** Red is conventionally positive and
   black negative, but verify with a multimeter if you have one rather
   than assuming.
2. **If you need to cut the battery's existing wires** (e.g. to remove
   a connector that doesn't match anything you have), cut ONE wire at
   a time, not both together — cutting both simultaneously with a tool
   that spans both risks momentarily bridging positive and negative
   through the metal cutters, which is a real short-circuit risk on a
   LiPo cell.
3. **Double-check polarity again immediately before soldering** —
   positive wire to the pad marked +, negative to the pad marked -.
   Reversed polarity here can damage the board's charging circuit or
   the battery itself.
4. **Insulate both joints completely** (heat-shrink tubing or
   electrical tape) before final assembly — these joints will sit close
   to other components inside a sealed case, and an exposed short on a
   LiPo connection is a genuine fire/damage risk, not just "it won't work."
5. **Test power-up somewhere you can watch it** for the first minute or
   two (not inside the sealed case yet) — check for any unusual heat,
   smell, or the charge LED behaving normally.

Note: soldering directly to the pads means there's no way to fully
power the board off short of physically disconnecting a wire — it's
always live as long as the battery's connected. Not dangerous, just
worth knowing (the board will slowly drain the battery even sitting
idle, so give it an occasional charge if it goes unused for a while).

### Charging

Check your specific board's actual charge current before assuming a
number — it's been inconsistently documented in different places
(Seeed's wiki and datasheet have listed different figures for the same
board). At a conservative 50mA, a 1000mAh battery takes roughly 20-24
hours for a full charge from empty; budget accordingly rather than
expecting a quick top-up.
