// Sats Card Kiosk — enclosure v2 (snap-in board retention, no tape)
// part = "base" or "lid".  z = 0 is the OUTER top surface of the lid.
part = "base";

/* ---------- parameters ---------- */
wall     = 2.5;
floorT   = 2.0;
lidTop   = 2.0;
tapFloor = 1.6;    // plastic over the PN532 (card reads through this)
fit      = 0.4;    // lid-into-base friction fit
boardT   = 2.6;    // slot height under the catch — roomy so boards slide fully under
$fn      = 40;

inL = 92; inW = 52; inH = 50;          // internal cavity (deepened for wire nest)

pnL = 44;  pnW = 42;                    // PN532 V3 footprint
olL = 28;  olW = 29;                    // SSD1306 PCB footprint
olWinL = 25; olWinW = 16;              // OLED screen window

// snap-clip geometry  (fingers grip the board's two long edges)
clipW   = 8;       // finger width (along the board edge) — wider grab
clipT   = 1.4;     // finger thickness
clipGap = 1.6;     // clip stand-off from the board edge — clips sit farther apart
clipNub = 2.4;     // nub length (overhang over the board = clipNub - clipGap = 0.8mm)
nubH    = 1.8;     // nub height — taller catch + longer lead-in ramp
fingerH = boardT + 4;   // finger length into the box — taller so the catch reaches

usbW = 14; usbZ = 2; usbH = 12;

extL = inL + 2*wall;  extW = inW + 2*wall;  baseH = floorT + inH;  lipH = 7;
olCX = 22;  pnCX = 64;  cY = inW/2;

/* ---------- base (unchanged — already fits) ---------- */
module base() {
  difference() {
    cube([extL, extW, baseH]);
    translate([wall, wall, floorT]) cube([inL, inW, inH + 1]);
    translate([-1, wall + inW/2 - usbW/2, floorT + usbZ]) cube([wall + 2, usbW, usbH]);
  }
}

/* ---------- board pocket cut ----------
   Widened in X (the clip axis) so the fingers + flex gap fit; the board is
   located tightly in Y by the pocket walls (the header edge is a Y edge, so it
   stays clip-free). */
module pocketCut(cx, cy, bL, bW, seatZ) {
  padX = clipGap + clipT;
  translate([cx - bL/2 - padX, cy - bW/2, seatZ]) cube([bL + 2*padX, bW, 20]);
}

/* ---------- two cantilever snap clips on the board's LEFT & RIGHT (X) edges ----------
   Placed on the side edges so they never touch the pin headers, which sit on a
   Y edge. Each finger hangs from the pocket ceiling into the box and its nub
   overhangs inward over the board's back face. */
module clips(cx, cy, bL, seatZ) {
  backZ = seatZ + boardT;
  for (s = [-1, 1]) {
    xo = cx + s*(bL/2 + clipGap);                  // finger inner face
    // finger post — taller so the catch reaches over a seated board
    translate([s > 0 ? xo : xo - clipT, cy - clipW/2, seatZ])
      cube([clipT, clipW, fingerH]);
    // wedge nub: flat underside at backZ retains the board; sloped top is a
    // lead-in ramp so the board cams the finger outward as you press it in
    if (s > 0)
      hull() {
        translate([xo - clipNub, cy - clipW/2, backZ])       cube([clipNub, clipW, 0.01]);
        translate([xo - 0.01,    cy - clipW/2, backZ + nubH]) cube([0.01,   clipW, 0.01]);
      }
    else
      hull() {
        translate([xo,        cy - clipW/2, backZ])       cube([clipNub, clipW, 0.01]);
        translate([xo,        cy - clipW/2, backZ + nubH]) cube([0.01,   clipW, 0.01]);
      }
  }
}

/* ---------- lid ---------- */
module lid() {
  difference() {
    union() {
      cube([extL, extW, lidTop]);
      translate([wall + fit, wall + fit, lidTop])
        difference() {
          cube([inL - 2*fit, inW - 2*fit, lipH]);
          translate([wall, wall, -1]) cube([inL - 2*fit - 2*wall, inW - 2*fit - 2*wall, lipH + 2]);
        }
    }
    // OLED screen window (through)
    translate([wall + olCX - olWinL/2, wall + cY - olWinW/2, -1]) cube([olWinL, olWinW, lidTop + 2]);
    // board pockets
    pocketCut(wall + olCX, wall + cY, olL, olW, lidTop - 0.5);   // OLED (front on window ledge)
    pocketCut(wall + pnCX, wall + cY, pnL, pnW, tapFloor);        // PN532 (front under tapFloor)
    // engraving on the outer top face (mirrored in X so it reads correctly
    // once the lid is printed top-face-down and flipped over for use)
    translate([wall + pnCX, wall + cY + 13, -0.1]) mirror([1,0,0])
      linear_extrude(0.9) text("TAP", size=6, halign="center", valign="center", font="Helvetica:style=Bold");
    translate([extL/2, wall + 2.5, -0.1]) mirror([1,0,0])
      linear_extrude(0.9) text("SATS KIOSK", size=4.5, halign="center", valign="baseline", font="Helvetica:style=Bold");
    translate([wall + pnCX, wall + cY - 1, -0.1])
      linear_extrude(0.7) difference() { square([pnL-8, pnW-10], center=true); square([pnL-10, pnW-12], center=true); }
  }
  clips(wall + olCX, wall + cY, olL, lidTop - 0.5);   // OLED clips (left/right edges)
  clips(wall + pnCX, wall + cY, pnL, tapFloor);        // PN532 clips (left/right edges)
}

if (part == "base") base();
else lid();
