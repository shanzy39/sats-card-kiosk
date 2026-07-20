// Sats Card Kiosk — flat enclosure
// part = "base" or "lid"; rendered separately to two STLs.
// z = 0 is the OUTER top surface of the lid (where a card taps).
part = "base";

/* ---------- parameters ---------- */
wall     = 2.5;   // side wall thickness
floorT   = 2.0;   // base floor thickness
lidTop   = 2.0;   // lid top-plate thickness
tapFloor = 1.6;   // plastic left over the PN532 (card reads through this)
fit      = 0.4;   // clearance for friction fit
$fn      = 48;

// internal cavity (roomy — holds ESP32 + the jumper-wire nest)
inL = 92;
inW = 52;
inH = 34;         // internal depth of the base

// board pockets (with clearance)
pnL = 44;  pnW = 42;         // PN532
olL = 28;  olW = 29;         // OLED PCB
olWinL = 25; olWinW = 16;    // OLED screen window (through-hole)

// USB slot (short wall) — generous so the cable reaches regardless of height
usbW = 14; usbZ = 2; usbH = 12;

// derived
extL = inL + 2*wall;
extW = inW + 2*wall;
baseH = floorT + inH;
lipH  = 7;

// window positions along the length
olCX = 22;
pnCX = 64;
cY   = inW/2;

/* ---------- base ---------- */
module base() {
  difference() {
    cube([extL, extW, baseH]);
    translate([wall, wall, floorT]) cube([inL, inW, inH + 1]);   // hollow, open top
    translate([-1, wall + inW/2 - usbW/2, floorT + usbZ])         // USB slot
      cube([wall + 2, usbW, usbH]);
  }
}

/* ---------- lid ---------- */
module lid() {
  difference() {
    union() {
      cube([extL, extW, lidTop]);                                // top plate: z 0..lidTop
      translate([wall + fit, wall + fit, lidTop])                // lip into the base
        difference() {
          cube([inL - 2*fit, inW - 2*fit, lipH]);
          translate([wall, wall, -1])
            cube([inL - 2*fit - 2*wall, inW - 2*fit - 2*wall, lipH + 2]);
        }
    }
    // OLED screen window (through)
    translate([wall + olCX - olWinL/2, wall + cY - olWinW/2, -1])
      cube([olWinL, olWinW, lidTop + 2]);
    // OLED PCB recess from underside (leaves a ~1.2mm ledge the PCB rests on)
    translate([wall + olCX - olL/2, wall + cY - olW/2, lidTop - 0.8])
      cube([olL, olW, 14]);
    // PN532 recess from underside (leaves tapFloor over it)
    translate([wall + pnCX - pnL/2, wall + cY - pnW/2, tapFloor])
      cube([pnL, pnW, 14]);
    // engraved text on the OUTER top face (mirrored in X so it reads correctly
    // when the lid is printed top-face-down and flipped over for use)
    translate([wall + pnCX, wall + cY + 12, -0.1]) mirror([1,0,0])
      linear_extrude(0.9) text("TAP", size=6, halign="center", valign="center", font="Helvetica:style=Bold");
    translate([extL/2, wall + 2.5, -0.1]) mirror([1,0,0])
      linear_extrude(0.9) text("SATS KIOSK", size=4.5, halign="center", valign="baseline", font="Helvetica:style=Bold");
    // engraved card outline over the tap zone
    translate([wall + pnCX, wall + cY - 1, -0.1])
      linear_extrude(0.7)
        difference() {
          square([pnL - 8, pnW - 10], center=true);
          square([pnL - 10, pnW - 12], center=true);
        }
  }
}

if (part == "base") base();
else lid();
