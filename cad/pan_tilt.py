# -*- coding: utf-8 -*-
"""
Pan-tilt mount v2 generator (CadQuery) - DRAFT, parameters first.

Fixes the two blocking gaps from docs/pan_tilt_mount.md:
  1. The camera holder is sized from the ESP32-CAM board parameters
     below (portrait), with the lens window placed from the lens
     offset parameters.
  2. The tilt arm carries a pocket for the second 9 g servo; the
     holder bolts to that servo's horn.

The holder grips the white case the stock ESP32-CAM (HW-0055) sits
in, not the bare board. Case dimensions were measured on 2026-11-28
from the owner's ruler photos (see docs/pan_tilt_mount.md); the case
depth is still a placeholder and must be confirmed before printing.
Change only the parameter block, then re-run: outputs go to
cad/out_pan_tilt/. Units are mm. Every part stays inside the
Adventurer 3 build volume (150 x 150 x 150 mm); the generator prints
a warning otherwise.
"""
import os

import cadquery as cq

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out_pan_tilt")
os.makedirs(OUT, exist_ok=True)

# ---------------- Parameters (case measured 2026-11-28) -------------
CASE_W = 46.0       # white case width  [measured from photos, +/-2 mm]
CASE_H = 49.0       # white case height [measured from photos, +/-2 mm]
CASE_D = 12.0       # white case depth  [PLACEHOLDER - not measured yet]
LENS_DX = 0.0       # lens center offset from case center, X (centered)
LENS_DY = -12.5     # lens ~12 mm above the case bottom edge -> below center
LENS_WIN = 14.0     # lens window size (square)
CLEAR = 2.0         # side clearance per side (covers the photo tolerance)

SG_POCKET_L = 23.2  # 9 g servo pocket length (SG90 class, as in v1)
SG_POCKET_W = 12.6  # 9 g servo pocket width

WALL = 3.0
BUILD_MAX = 140.0   # Adventurer 3 working limit incl. margin


def box(x, y, z, cx=0.0, cy=0.0, cz=0.0):
    return cq.Workplane("XY").box(x, y, z).translate((cx, cy, cz))


def hole_y(part, r, x, z, depth=30.0):
    """Through hole along Y at (x, z), centered on y = 0."""
    cutter = cq.Workplane("XY").circle(r).extrude(depth)
    cutter = cutter.rotate((0, 0, 0), (1, 0, 0), -90)
    return part.cut(cutter.translate((x, -depth / 2.0, z)))


# ---------------- Holder v3 (grips the measured white case) ---------
def cam_holder_v3():
    w = CASE_W + 2 * WALL + 2 * CLEAR
    h = CASE_H + 2 * WALL
    # Pocket: the case slides in from the front, against the back plate.
    y_front = WALL / 2.0 + CASE_D + 1.0        # 1 mm slack in front
    rail_len = y_front + WALL / 2.0
    rail_cy = (y_front - WALL / 2.0) / 2.0
    rail_x = CASE_W / 2.0 + CLEAR + 1.0
    back = box(w, WALL, h, 0, 0, h / 2.0)
    lip = box(w, rail_len, 4.0, 0, rail_cy, 2.0)          # bottom rest
    rail_l = box(2.0, rail_len, h, -rail_x, rail_cy, h / 2.0)
    rail_r = box(2.0, rail_len, h, rail_x, rail_cy, h / 2.0)
    p = back.union(lip).union(rail_l).union(rail_r)
    # Lens window through the back plate, from the lens offsets.
    p = p.cut(box(LENS_WIN, WALL + 2.0, LENS_WIN,
                  LENS_DX, 0.0, h / 2.0 + LENS_DY))
    # Tilt pivot: horn screw hole (Y) near the bottom, on the case axis.
    p = hole_y(p, 2.2, 0.0, 6.0)
    return p


# ---------------- Tilt arm v2 (with tilt-servo pocket) ---------------
def cam_tilt_arm_v2():
    p = box(34.0, 14.0, 56.0, 0, 0, 28.0)
    # Pan axis: center hole for the pan servo horn screw (Z, bottom).
    p = p.cut(cq.Workplane("XY").circle(2.6).extrude(10).translate((0, 0, -2)))
    # Tilt servo pocket near the top: servo lies with its shaft on Y.
    pocket = box(SG_POCKET_L, 15.0, SG_POCKET_W, 0, 0, 44.0)
    p = p.cut(pocket)
    # Shaft clearance hole on the pocket axis (Y).
    p = hole_y(p, 3.0, 0.0, 44.0)
    return p


# ---------------- Pole segment (stackable, screw-joined) ------------
def pole_segment(height=70.0):
    p = box(30.0, 30.0, height, 0, 0, height / 2.0)
    p = p.cut(cq.Workplane("XY").circle(5.0).extrude(height + 2)
              .translate((0, 0, -1)))                    # cable channel
    for (x, y) in [(11, 11), (11, -11), (-11, 11), (-11, -11)]:
        p = p.cut(cq.Workplane("XY").circle(1.7).extrude(height + 2)
                  .translate((x, y, -1)))                # M3 through holes
    return p


PARTS = {
    "12_cam_holder_v3": cam_holder_v3,
    "13_cam_tilt_arm_v2": cam_tilt_arm_v2,
    "14_pole_segment_70": pole_segment,
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
