# ESP32-CAM formal case: design (for #9)

Status: design draft, ready for the owner's confirmation. Nothing is
printed until the owner approves (printing is an approval step).
Dimensioned drawing: docs/esp32cam_case_drawing.html.
Generator: cad/esp32cam_case.py (CadQuery, parametric).

## Why a new case

The white case in the 2026-11-28 photos is a temporary one the owner
made. This is the formal replacement, designed around the measured
board and the existing pan-tilt holder.

## Measured basis (owner photos, 2026-11-28)

| Item | Value | Notes |
|---|---|---|
| Board (PCB) | 36 x 39 mm (+/-1.5) | **not** the 27 x 40.5 AI-Thinker nominal; the case fits the board in hand |
| Lens center | 15.6 mm from board left edge; 7.9 mm above board bottom edge | pixel-measured against the ruler |
| Temporary case outer | 46 x 49 mm (+/-2) | the holder v3 pocket was sized to this |

If a true AI-Thinker board (27 x 40.5) is ever fitted instead, this
case will not fit it; that is accepted for this design.

## Design

Two parts, outer 46 x 49 mm (drops into the holder v3 pocket),
total depth 12.6 mm:

- **Front shell** (10.6 mm deep): board pocket with 0.5 mm clearance
  per side; four corner ledges hold the board's front face (corner
  areas are component-free on typical boards - confirmed at the
  test fit); lens aperture dia 11 mm (the ~9.5 mm barrel protrudes
  through it, as it does from the temporary case); LED window
  dia 3.5 mm at the onboard LED (position approximate); cable exit
  slot 10 x 4 mm through the right wall, matching the current
  wiring; 3 vent slots in the top wall; M2 pilot holes in the four
  corners.
- **Back cover** (2 mm plate): 4 vent slots; two ribs whose tips
  stop 0.1 mm behind the board's back face so the closed cover
  keeps the board in place; 4 corner screw holes (M2); two optional
  direct-mount bosses (dia 9, dia 3.2 holes for M3 heat-set
  inserts) for mounting without the holder later.
- Depth stack (front to back): front wall 2.5 / front clear 4.0
  (camera FPC socket) / board 1.6 / back clear 2.5 (microSD side) /
  back plate 2.0.
- microSD access: remove the back cover (4 screws). A tool-less
  side slot was not designed because the board's back side has not
  been photographed; if that is wanted, a back-side photo is the
  input needed.
- Field of view: the aperture (dia 11) sits ~2.5-6.5 mm in front of
  the sensor plane area and the barrel protrudes through it, so
  the case does not narrow the OV2640 field of view; the holder's
  14 x 14 window stays aligned via the holder parameter update
  that accompanies this design.

## Generation verified (2026-11-28, CadQuery 2.8.0)

| Part | Bounding box [mm] | Feature probes |
|---|---|---|
| 20_case_front | 46.0 x 10.6 x 49.0 | aperture, pocket, ledges, cable slot, pilots all present |
| 21_case_back | 46.0 x 6.4 x 49.0 (incl. ribs and bosses) | plate, ribs, boss holes, vents all present |

Assembled depth is 12.6 mm (14.6 mm at the optional bosses).

## Open points for the owner's confirmation

1. Back cover fixing: screws (current design, M2 self-tap x4) or a
   snap fit?
2. Cable exit on the right side, as the current wiring - OK?
3. Optional direct-mount bosses on the back - wanted, or drop them?
4. Board thickness (1.6 mm) and the corner-ledge assumption are
   verified at the first test fit (print the front shell alone
   first, as with the holder).
