// Sats Card Kiosk — enclosure v2 (snap-in board retention, no tape)
// part = "base" or "lid".  z = 0 is the OUTER top surface of the lid.
part = "base";

/* ---------- parameters ---------- */
wall     = 2.5;
floorT   = 2.0;
lidTop   = 2.0;
tapFloor = 1.6;    // plastic over the PN532 (card reads through this)
fit      = 0.4;    // lid-into-base friction fit
boardT   = 1.7;    // PCB thickness (+ a hair)
$fn      = 40;

inL = 92; inW = 52; inH = 34;          // internal cavity

pnL = 44;  pnW = 42;                    // PN532 V3 footprint
olL = 28;  olW = 29;                    // SSD1306 PCB footprint
olWinL = 25; olWinW = 16;              // OLED screen window

// snap-clip geometry  (fingers grip the board's two long edges)
clipW   = 6;       // finger width (along the board edge)
clipT   = 1.4;     // finger thickness
clipGap = 0.8;     // flex gap between finger and pocket wall
clipNub = 1.4;     // nub length (must exceed clipGap to overhang the board)
nubH    = 1.2;

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

/* ---------- board pocket cut: footprint widened in Y for clips + flex gap ---------- */
module pocketCut(cx, cy, bL, bW, seatZ) {
  padY = clipGap + clipT;                          // extra room per clip side
  translate([cx - bL/2, cy - bW/2 - padY, seatZ]) cube([bL, bW + 2*padY, 20]);
}

/* ---------- two cantilever snap clips on the board's long (Y) edges ---------- */
module clips(cx, cy, bW, seatZ) {
  backZ = seatZ + boardT;
  for (s = [-1, 1]) {
    yo = cy + s*(bW/2 + clipGap);                  // finger inner face
    // finger: anchored at the pocket ceiling (seatZ), hangs into the box (+z)
    translate([cx - clipW/2, s > 0 ? yo : yo - clipT, seatZ])
      cube([clipW, clipT, boardT + 2]);
    // nub: overhangs inward over the board's back face
    translate([cx - clipW/2, s > 0 ? yo - clipNub : yo, backZ])
      cube([clipW, clipNub, nubH]);
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
  clips(wall + olCX, wall + cY, olW, lidTop - 0.5);   // OLED clips
  clips(wall + pnCX, wall + cY, pnW, tapFloor);        // PN532 clips
}

if (part == "base") base();
else lid();
