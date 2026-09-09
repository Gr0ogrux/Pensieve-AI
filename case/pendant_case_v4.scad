// ============================================================
// ESP32-S3 Sense "AI Pendant" Case — v4
// Changes from v3:
//   - Cord tunnel REMOVED (it interrupted the skirt seal and let the
//     case flip/rotate on the cord -- exactly what you saw in testing)
//   - New: a solid end bail (loop) at the battery end, opposite the
//     grill, so the cord attaches at a true stable end point
//   - New: magnet-based lid retention (4x1mm discs), replacing the
//     old snap-dimple mechanism
//
// Board:   XIAO ESP32-S3 Sense, measured stack height 8.2mm,
//          footprint 24.8 x 17.9mm (length includes microSD card)
// Battery: MakerHawk 803040, measured 40.5 x 29.4 x 7.7mm
// Button:  6.0 x 6.6mm body, 5.0mm tall unpressed, 3.3mm cap diameter
// Magnets: 4mm diameter x 1mm thick neodymium discs (x4 total: 2 per half)
//
// BEFORE PRINTING: dry-fit the base alone first, same as always --
// this is a tightly packed design and the magnet mounts are new,
// unverified geometry.
//
// MAGNET POLARITY: I can't control this in the STL -- physical
// magnets need to be tested before gluing. Hold a base magnet and a
// lid magnet near each other; if they repel, flip one over before
// gluing it into its pocket.
// ============================================================

// ---------------- CONFIG: BOARD (measured) ----------------
board_len   = 26.5;
board_wid   = 19.5;
board_stack_h = 9.5;
pcb_thickness = 1.8;
usb_c_width   = 10;
usb_c_height  = 4;
usb_c_from_bottom = 3;

// ---------------- CONFIG: BATTERY (measured) ----------------
batt_len = 42;
batt_wid = 31;
batt_h   = 8.7;

// ---------------- CONFIG: BUTTON (measured) ----------------
button_body_len = 6.6;
button_body_wid = 6.0;
button_total_h  = 5.0;
button_cap_d    = 3.3;
hole_clearance  = 0.5;

// ---------------- CONFIG: SHELL ----------------
wall = 2.2;
gap_between_pockets = 6.0;
depth_buffer = 1;
corner_r_cfg = 8;

// ---------------- CONFIG: LID / SKIRT (alignment only now) ----------------
tol = 0.2;
skirt_wall = 1.6;
skirt_h = 3.2;
lid_top_thk = 1.8;

// ---------------- CONFIG: MAGNETS ----------------
magnet_d = 4;         // 4x1mm discs
magnet_t = 1.1;        // slight over-size on thickness for easy fit + glue
magnet_pocket_clearance = 0.2;
magnet_boss_od = 7;    // outer diameter of the lid-side hanging boss
magnet_y_offset = 7;   // distance off centerline for each of the 2 magnets

// ---------------- CONFIG: END BAIL (cord loop) ----------------
bail_extend = 6;       // how far the loop sticks out past the end wall
bail_wid = 11;         // Y size of the loop tab
bail_thick = 3.2;      // material thickness around the hole
bail_hole_d = 4.2;      // fits typical necklace cord/paracord
bail_z_center_offset = 6.5; // how far down from the top the hole sits --
                             // must keep the whole bail (± half its
                             // width) within the base's own height, or
                             // it collides with the lid above box_h

// ---------------- CONFIG: MIC GRILL ----------------
grill_hole_d = 1.1;
grill_rows = 3;
grill_cols = 4;
grill_spacing = 2.5;

// ---------------- DERIVED ----------------
cavity_len = board_len + gap_between_pockets + batt_len;
cavity_wid = max(board_wid, batt_wid);
case_len = cavity_len + wall*2;
case_wid = cavity_wid + wall*2;
corner_r = corner_r_cfg;

tallest_component = max(board_stack_h, batt_h);
cavity_depth_total = tallest_component + depth_buffer + skirt_h;
box_h = wall + cavity_depth_total;
total_h = box_h + lid_top_thk;

board_zone_x = -cavity_len/2 + board_len/2;
batt_zone_x  =  cavity_len/2 - batt_len/2;
rib_x = (board_zone_x + board_len/2 + batt_zone_x - batt_len/2) / 2;

mic_grill_x = board_zone_x;
mic_grill_y = 0;

button_x = rib_x - 2.2;
button_y = 0;
button_standoff_h = box_h - button_total_h - wall;

rib_top_z = wall + tallest_component; // top surface of the divider rib

echo("=== CASE OUTER DIMENSIONS (mm) ===");
echo(length = case_len, width = case_wid, total_height = total_h);
echo(rib_top_z = rib_top_z, gap_above_rib = box_h - rib_top_z);

// ============================================================
// HELPERS
// ============================================================
module profile_2d(inset = 0) {
    offset(r = -inset)
        offset(r = corner_r)
            square([case_len - 2*corner_r, case_wid - 2*corner_r], center = true);
}

module outer_shell(h) {
    linear_extrude(height = h) profile_2d(0);
}

// ============================================================
// BASE (open-top box)
// ============================================================
module base_solid() {
    outer_shell(box_h);
}

module main_cavity() {
    // Restored to a single cavity matching the case's rounded outer
    // shape, covering the ENTIRE interior -- including the space
    // around and above the divider rib. Cutting this down to two
    // separate board/battery-only rectangles (previous attempt) left
    // that space solid, which blocked the lid's skirt and magnet
    // bosses from ever reaching down that far, and buried the magnet
    // pockets under a wall of material with no opening to them.
    translate([0, 0, wall])
        linear_extrude(height = cavity_depth_total + 1)
            profile_2d(wall);
}

// Narrows the board pocket from the full cavity width down to just
// board_wid, WITHOUT touching the main cavity cut -- added as solid
// filler on both sides instead, so the outer perimeter relief (which
// the lid's skirt and the magnet bosses both depend on) stays intact
// everywhere else.
module board_side_fillers() {
    // Height matches the divider rib's height, NOT the full cavity
    // depth -- as an additive block, extending it all the way up
    // (like the old subtractive cavity needed to) meant it intruded
    // into the skirt zone and blocked the lid from seating.
    filler_wid = (cavity_wid - board_wid) / 2;
    filler_h = tallest_component;
    if (filler_wid > 0.1) {
        translate([board_zone_x - board_len/2, board_wid/2, wall])
            cube([board_len, filler_wid, filler_h]);
        translate([board_zone_x - board_len/2, -cavity_wid/2, wall])
            cube([board_len, filler_wid, filler_h]);
    }
}

module divider_rib() {
    difference() {
        translate([rib_x, 0, wall + tallest_component/2])
            cube([gap_between_pockets - 1, cavity_wid - 2*wall - 0.2, tallest_component], center = true);
        translate([rib_x, 0, wall + 2])
            cube([gap_between_pockets + 2, 4, 4], center = true);
        // magnet pockets recessed into the rib's top face
        for (sy = [-1, 1])
            translate([rib_x, sy*magnet_y_offset, rib_top_z - magnet_t + 0.01])
                cylinder(d = magnet_d + magnet_pocket_clearance, h = magnet_t, $fn = 24);
    }
}

module board_retention_grooves() {
    // Repositioned to match the now-correctly-narrow board pocket --
    // these previously sat at the edge of the OLD oversized cavity,
    // nowhere near the actual board, which is why they never engaged.
    groove_z = wall + pcb_thickness/2 + 1.5;
    translate([board_zone_x, board_wid/2 - 0.6, groove_z])
        cube([board_len, 1.2, pcb_thickness + 0.6], center = true);
    translate([board_zone_x, -(board_wid/2 - 0.6), groove_z])
        cube([board_len, 1.2, pcb_thickness + 0.6], center = true);
}

module usb_c_cutout() {
    translate([-case_len/2 - 1, 0, wall + usb_c_from_bottom + usb_c_height/2])
        cube([wall*2 + 2, usb_c_width, usb_c_height], center = true);
}

// Button standoff: a raised platform in the rib zone sized to the
// button's footprint, with a shallow lip so the button doesn't slide
// once it's set in place (glue or friction fit).
//
// The physical button has 4 legs at its corners extending ~2.3mm below
// the body -- rather than let the button balance on those thin legs
// (which would throw off the height calculation and stress the legs
// under repeated presses), the shelf has small relief holes so the
// body's flat underside sits directly on the shelf surface.
button_leg_x = button_body_len/2 - 1.5;
button_leg_y = button_body_wid/2 - 1.5;
button_leg_hole_d = 1.3;
button_leg_hole_depth = 2.8;

module button_standoff() {
    difference() {
        translate([button_x, button_y, wall + button_standoff_h/2])
            cube([button_body_len + 3, button_body_wid + 3, button_standoff_h], center = true);
        // leg relief holes, recessed into the top of the standoff
        for (sx = [-1, 1]) for (sy = [-1, 1])
            translate([button_x + sx*button_leg_x, button_y + sy*button_leg_y,
                       wall + button_standoff_h - button_leg_hole_depth + 0.01])
                cylinder(d = button_leg_hole_d, h = button_leg_hole_depth, $fn = 16);
    }
}

module button_retention_lip() {
    lip_h = 1;
    translate([button_x, button_y, wall + button_standoff_h - lip_h/2])
        difference() {
            cube([button_body_len + 2, button_body_wid + 2, lip_h + 0.5], center = true);
            cube([button_body_len - 0.5, button_body_wid - 0.5, lip_h + 1], center = true);
        }
}

// End bail: rebuilt as a single solid block (previously built from
// two 0.2mm-thick discs hulled together -- thinner than most printer
// nozzles can lay down, which almost certainly printed as a broken or
// missing connection). This version overlaps 1.5mm into the case body
// for a guaranteed solid union, well within the base's own height so
// there's no parting-line weakness through the loop.
module end_bail() {
    hole_z = box_h - bail_z_center_offset;
    block_len = bail_extend + 1.5;
    difference() {
        translate([case_len/2 - 1.5, -bail_wid/2, hole_z - bail_wid/2])
            cube([block_len, bail_wid, bail_wid]);
        translate([case_len/2 - 1.5 + block_len/2, 0, hole_z])
            rotate([90, 0, 0])
                cylinder(d = bail_hole_d, h = bail_wid*2, center = true, $fn = 30);
    }
}

module base_shell() {
    difference() {
        base_solid();
        main_cavity();
        usb_c_cutout();
    }
    divider_rib();
    board_side_fillers();
    board_retention_grooves();
    button_standoff();
    button_retention_lip();
    end_bail();
}

// ============================================================
// LID
// ============================================================
module lid_top() {
    translate([0, 0, box_h])
        linear_extrude(height = lid_top_thk) profile_2d(0);
}

module lid_skirt() {
    translate([0, 0, box_h - skirt_h])
        linear_extrude(height = skirt_h + 0.01)
            difference() {
                profile_2d(wall + tol);
                profile_2d(wall + tol + skirt_wall);
            }
}

module mic_grill_cut() {
    translate([mic_grill_x, mic_grill_y, box_h - 0.5])
        for (i = [0 : grill_cols-1]) for (j = [0 : grill_rows-1])
            translate([(i - (grill_cols-1)/2) * grill_spacing,
                       (j - (grill_rows-1)/2) * grill_spacing, 0])
                cylinder(d = grill_hole_d, h = lid_top_thk + 2, $fn = 16);
}

module button_hole_cut() {
    translate([button_x, button_y, box_h - 0.5])
        cylinder(d = button_cap_d + hole_clearance, h = lid_top_thk + 2, $fn = 24);
}

// Bosses hanging down from the lid to meet the rib's magnet pockets.
module magnet_bosses() {
    for (sy = [-1, 1])
        translate([rib_x, sy*magnet_y_offset, rib_top_z])
            cylinder(d = magnet_boss_od, h = box_h - rib_top_z, $fn = 30);
}

module magnet_pockets_lid() {
    for (sy = [-1, 1])
        translate([rib_x, sy*magnet_y_offset, rib_top_z - 0.01])
            cylinder(d = magnet_d + magnet_pocket_clearance, h = magnet_t, $fn = 24);
}

module lid_shell() {
    difference() {
        union() {
            lid_top();
            lid_skirt();
            magnet_bosses();
        }
        mic_grill_cut();
        button_hole_cut();
        magnet_pockets_lid();
    }
}

// ============================================================
// OUTPUT SELECTOR
// ============================================================
part = "preview"; // "base" | "lid" | "both" | "preview"

if (part == "base") {
    base_shell();
} else if (part == "lid") {
    lid_shell();
} else if (part == "both") {
    base_shell();
    translate([0, case_wid + 15, 0]) lid_shell();
} else {
    color("SteelBlue") base_shell();
    color("LightGray", 0.85) lid_shell();
}
