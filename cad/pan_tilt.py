# -*- coding: utf-8 -*-
"""
Pan-tilt mount v2 generator (CadQuery) - DRAFT, parameters first.

Fixes the two blocking gaps from docs/pan_tilt_mount.md:
  1. The camera holder is sized from the ESP32-CAM board parameters
     below (portrait), with the lens window placed from the lens
     offset parameters.
  2. The tilt arm carries a pocket for the second 9 g servo; the
     holder bolts to that servo's horn.

All board dimensions are ASSUMED (AI-Thinker type) until the physical
board (stock unit HW-0055) is measured. Change only the parameter
block after measuring, then re-run: outputs go to cad/out_pan_tilt/.
Units are mm. Every part stays inside the Adventurer 3 build volume
(150 x 150 x 150 mm); the generator prints a warning otherwise.
"""
import os

import cadquery as cq

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out_pan_tilt")
os.makedirs(OUT, exist_ok=True)

# ---------------- Parameters (measure the real board, then edit) ----
BOARD_W = 27.0      # ESP32-CAM board width  [ASSUMED, AI-Thinker type]
BOARD_H = 40.5      # ESP32-CAM board height [ASSUMED]
LENS_DX = 0.0       # lens center offset from board center, X [ASSUMED]
LENS_DY = 9.0       # lens center offset from board center, Z [ASSUMED]
LENS_WIN = 14.0     # lens window size (square)

SG_POCKET_L = 23.2  # 9 g servo pocket length (SG90 class, as in v1)
SG_POCKET_W = 12.6  # 9 g servo pocket width

WALL = 3.0
BUILD_MAX = 140.0   # Adventurer 3 working limit incl. margin


def box(x, y, z, cx=0.0, cy=0.0, cz=0.0):
    return cq.Workplane("XY").box(x, y, z).translate((cx, cy, cz))


def hole_y(part, r, x, z, depth=30.0):
    """Through hole along Y at (x, z)."""
    cutter = cq.Workplane("XY").circle(r).extrude(depth)
    cutter = cutter.rotate((0, 0, 0), (1, 0, 0), 90)
    return part.cut(cutter.translate((x, -depth / 2.0, z)))


# ---------------- Holder v2 (portrait, parametric) -------------------
def cam_holder_v2():
    w = BOARD_W + 2 * WALL + 2.0        # side clearance 1 mm per side
    h = BOARD_H + 2 * WALL
    back = box(w, WALL, h, 0, 0, h / 2.0)
    lip = box(w, WALL + 3.0, 4.0, 0, 1.5, 2.0)          # bottom rest
    rail_l = box(2.0, WALL + 2.0, h, -(BOARD_W / 2.0 + 1.0), 1.0, h / 2.0)
    rail_r = box(2.0, WALL + 2.0, h, (BOARD_W / 2.0 + 1.0), 1.0, h / 2.0)
    p = back.union(lip).union(rail_l).union(rail_r)
    # Lens window from the lens offset parameters.
    p = p.cut(box(LENS_WIN, WALL + 2.0, LENS_WIN,
                  LENS_DX, 0.0, h / 2.0 + LENS_DY))
    # Tilt pivot: horn screw hole (Y) near the bottom, on the board axis.
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
    "12_cam_holder_v2": cam_holder_v2,
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
