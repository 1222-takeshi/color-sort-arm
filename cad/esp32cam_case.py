# -*- coding: utf-8 -*-
"""
Formal ESP32-CAM case generator (CadQuery) - DRAFT, parameters first.

Two printable parts that drop into the pan-tilt holder v3 pocket
(cad/pan_tilt.py): a front shell (lens aperture, LED window, side
cable exit, top vents, PCB corner ledges) and a screw-on back cover
(vents, PCB retaining ribs, two optional direct-mount bosses).
Removing the back cover gives access to the microSD slot on the
board's back side.

Frame: X = width as seen looking at the front of the case, Z = up,
Y = depth from the FRONT face (Y = 0) to the back (Y = OUTER_D).
The board measurements come from the owner's ruler photos of the
stock unit (2026-11-28, see docs/esp32cam_case.md): the board in
hand is 36 x 39 mm, NOT the 27 x 40.5 mm AI-Thinker nominal, so the
case follows the measured board. Units are mm. Every part stays
inside the Adventurer 3 build volume (150 x 150 x 150 mm); the
generator prints a warning otherwise.
"""
import os

import cadquery as cq

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out_esp32cam_case")
os.makedirs(OUT, exist_ok=True)

# ---------------- Parameters (board measured 2026-11-28) -------------
PCB_W = 36.0        # board width  [measured from photos, +/-1.5 mm]
PCB_H = 39.0        # board height [measured from photos, +/-1.5 mm]
PCB_T = 1.6         # board thickness [assumed]
OUTER_W = 46.0      # case outer width  (matches the holder v3 pocket)
OUTER_H = 49.0      # case outer height (matches the holder v3 pocket)
BACK_WALL = 2.0     # back cover plate thickness
BACK_CLEAR = 2.5    # room behind the board (microSD slot side)
FRONT_CLEAR = 4.0   # room in front of the board (camera FPC socket)
FRONT_WALL = 2.5    # front wall thickness
OUTER_D = BACK_WALL + BACK_CLEAR + PCB_T + FRONT_CLEAR + FRONT_WALL  # 12.6
SHELL_D = OUTER_D - BACK_WALL   # front shell depth: 10.6

PCB_X = (OUTER_W - PCB_W) / 2.0   # board left edge, case coords: 5.0
PCB_Y = (OUTER_H - PCB_H) / 2.0   # board bottom edge, case coords: 5.0
LENS_X = PCB_X + 15.6   # lens center from the case left edge [measured]
LENS_Z = PCB_Y + 7.9    # lens center from the case bottom edge [measured]
LENS_APERTURE = 11.0    # the ~9.5 mm lens barrel protrudes through it
LED_X = PCB_X + 4.0     # onboard white LED [approximate, from photos]
LED_Z = PCB_Y + PCB_H - 3.5
LED_WIN = 3.5
CLEAR = 0.5             # board pocket clearance per side
SCREW_INSET = 2.25      # corner screw axis from the outer faces
BUILD_MAX = 140.0       # Adventurer 3 working limit incl. margin

# Y planes (front face = 0): front wall inside 2.5, board front 6.5,
# board back 8.1, shell back rim / cover inside 10.6, case back 12.6.
Y_BOARD_FRONT = FRONT_WALL + FRONT_CLEAR            # 6.5
Y_BOARD_BACK = Y_BOARD_FRONT + PCB_T                # 8.1
Y_SHELL_BACK = SHELL_D                              # 10.6


def box(x, y, z, cx=0.0, cy=0.0, cz=0.0):
    return cq.Workplane("XY").box(x, y, z).translate((cx, cy, cz))


def cyl_y(r, y0, y1, x, z):
    """Cylinder along Y from y0 to y1 at (x, z)."""
    c = cq.Workplane("XY").circle(r).extrude(y1 - y0)
    c = c.rotate((0, 0, 0), (1, 0, 0), -90)
    return c.translate((x, y0, z))


# ---------------- Front shell -----------------------------------------
def case_front():
    cx, cz = OUTER_W / 2.0, OUTER_H / 2.0
    p = box(OUTER_W, SHELL_D, OUTER_H, cx, SHELL_D / 2.0, cz)
    # Board pocket: open at the back, floor is the front wall inside.
    pw, ph = PCB_W + 2 * CLEAR, PCB_H + 2 * CLEAR
    p = p.cut(box(pw, SHELL_D, ph, PCB_X + PCB_W / 2.0,
                  FRONT_WALL + SHELL_D / 2.0, PCB_Y + PCB_H / 2.0))
    # Corner ledges: hold the board's front face at its 4 corners
    # (corner areas are component-free on typical boards; verify at
    # the test fit).
    for lx in (PCB_X - CLEAR, PCB_X + PCB_W + CLEAR - 2.5):
        for lz in (PCB_Y - CLEAR, PCB_Y + PCB_H + CLEAR - 2.5):
            p = p.union(box(2.5, Y_BOARD_FRONT - FRONT_WALL, 2.5,
                            lx + 1.25, (FRONT_WALL + Y_BOARD_FRONT) / 2.0,
                            lz + 1.25))
    # Lens aperture and LED window through the front wall.
    p = p.cut(cyl_y(LENS_APERTURE / 2.0, -1.0, FRONT_WALL + 1.0,
                    LENS_X, LENS_Z))
    p = p.cut(cyl_y(LED_WIN / 2.0, -1.0, FRONT_WALL + 1.0, LED_X, LED_Z))
    # Cable exit through the right wall (matches the current wiring).
    p = p.cut(box(OUTER_W - (PCB_X + pw) + 2.0, 6.0, 4.0,
                  OUTER_W - 1.0, (FRONT_WALL + Y_SHELL_BACK) / 2.0,
                  OUTER_H * 0.45))
    # Top vents: 3 slots through the top wall, clear of the corners.
    for vx in (12.0, 23.0, 34.0):
        p = p.cut(box(10.0, 3.0, 3.0, vx, (FRONT_WALL + Y_SHELL_BACK) / 2.0,
                      OUTER_H - 1.0))
    # Corner screw pilots (M2 self-tap) from the back rim.
    for sx in (SCREW_INSET, OUTER_W - SCREW_INSET):
        for sz in (SCREW_INSET, OUTER_H - SCREW_INSET):
            p = p.cut(cyl_y(0.9, Y_SHELL_BACK - 6.0, Y_SHELL_BACK + 1.0,
                            sx, sz))
    return p


# ---------------- Back cover -------------------------------------------
def case_back():
    cx, cz = OUTER_W / 2.0, OUTER_H / 2.0
    p = box(OUTER_W, BACK_WALL, OUTER_H, cx, Y_SHELL_BACK + BACK_WALL / 2.0,
            cz)
    # Retaining ribs on the inner face: tips stop 0.1 mm behind the
    # board's back face, so the closed cover keeps the board in place.
    for rz in (PCB_Y + 10.0, PCB_Y + PCB_H - 10.0):
        p = p.union(box(20.0, BACK_CLEAR - 0.1, 2.0, cx,
                        Y_BOARD_BACK + (BACK_CLEAR - 0.1) / 2.0 + 0.1, rz))
    # Optional direct-mount bosses on the back face (M3 insert holes).
    for bx in (cx - 12.0, cx + 12.0):
        boss = cq.Workplane("XY").circle(4.5).extrude(2.0)
        boss = boss.rotate((0, 0, 0), (1, 0, 0), -90)
        p = p.union(boss.translate((bx, OUTER_D, 8.0)))
        p = p.cut(cyl_y(1.6, Y_SHELL_BACK - 1.0, OUTER_D + 3.0, bx, 8.0))
    # Back vents: 4 horizontal slots through the plate.
    for vz in (19.0, 25.0, 31.0, 38.0):
        p = p.cut(box(24.0, BACK_WALL + 2.0, 2.0, cx,
                      Y_SHELL_BACK + BACK_WALL / 2.0, vz))
    # Corner screw clearance holes (M2).
    for sx in (SCREW_INSET, OUTER_W - SCREW_INSET):
        for sz in (SCREW_INSET, OUTER_H - SCREW_INSET):
            p = p.cut(cyl_y(1.1, Y_SHELL_BACK - 1.0, OUTER_D + 1.0, sx, sz))
    return p


PARTS = {
    "20_case_front": case_front,
    "21_case_back": case_back,
}

if __name__ == "__main__":
    for name, fn in PARTS.items():
        part = fn()
        shape = part.val()
        bb = shape.BoundingBox()
        dx, dy, dz = bb.xlen, bb.ylen, bb.zlen
        warn = ""
        if max(dx, dy, dz) > BUILD_MAX:
            warn = "  <-- WARNING: exceeds Adventurer 3 working limit"
        cq.exporters.export(part, os.path.join(OUT, name + ".step"))
        cq.exporters.export(part, os.path.join(OUT, name + ".stl"))
        print(f"{name}: {dx:.1f} x {dy:.1f} x {dz:.1f} mm{warn}")
    print("done ->", OUT)
