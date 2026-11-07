# ESP32-CAM pan-tilt mount: CAD verification notes

Status: verification of the draft generator parts (2026-11-07). The three
parts already exist in `cad/make_parts.py` (the local draft generator and
the copy in this repository are identical). Nothing has been printed yet.

## Parts and printability (Adventurer 3, 150 x 150 x 150 mm)

Bounding boxes measured from the generated STL output of the current
generator code:

| Part | Bounding box [mm] | Fits build volume | Notes |
|---|---|---|---|
| 09_cam_pan_base | 60.0 x 60.0 x 6.0 | yes, large margin | round base plate, 9 g servo pocket, 4 screw holes |
| 10_cam_tilt_arm | 8.0 x 14.0 x 56.0 | yes | vertical arm, pan-axis hole at the bottom, tilt-axis hole at the top |
| 11_cam_holder | 46.0 x 6.0 x 36.0 | yes | back plate with a bottom lip and a lens window |

All three parts print without splitting. The split-and-screw construction
applies to the height pole (see gap 4), not to these parts.

## Assumed dimensions (to be confirmed on the physical parts)

| Item | Assumption used here | Source / confidence |
|---|---|---|
| ESP32-CAM board (in-stock unit HW-0055) | AI-Thinker type, 27.0 x 40.5 mm, portrait, lens near the top | common variant; **assumed, not measured** |
| 9 g servo (x2, in stock) | SG90 class: body 22.2 x 11.8 mm, height ~31 mm incl. flange, pocket 23.2 x 12.6 mm in the generator | generator parameters |
| PCA9685 board | not modelled in CAD; sits beside the mount | - |
| Reference case on hand (downloaded design) | 33.0 x 46.5 mm footprint, portrait | measured from the STL; consistent with the portrait board assumption |

## Fit-check points and known gaps

1. **Holder size does not match the portrait board (blocking for print).**
   The holder back plate is 46 mm wide x 36 mm tall, which fits a
   landscape board. Under the portrait assumption the board is 27 mm wide
   x 40.5 mm tall: taller than the holder, with ~19 mm of width slack.
   Action: measure the actual HW-0055 board (W x H, lens center offset,
   connector side), then resize the holder to portrait (roughly 34 x 47 mm
   class, like the reference case) and re-derive the lens window position
   from the measured lens offset. The current window (30 x 12 mm at
   z = 20..32 mm) is a landscape assumption and will move.
2. **No mount for the tilt servo (blocking for print).**
   Parts 09-11 contain a pocket only for the pan servo. The tilt "axis" is
   a plain hole in the tilt arm (r = 2.2 mm) plus a shallow mark on the
   holder: a passive pivot with nothing to drive it. The second 9 g servo
   has no home. Decision needed before printing: (a) pocket for the tilt
   servo at the top of the tilt arm, holder fixed to its horn, or
   (b) tilt servo on the holder back, arm as the fixed pivot side.
   Option (a) keeps the moving mass low and matches common pan-tilt kits.
3. **Pan servo pocket cuts through the base plate (check on the servo).**
   The pocket box is 7 mm deep in a 6 mm plate, so the pocket is open at
   the bottom. The servo is then held only by its flange ears resting on
   the plate. Verify with the actual servo that the ears fully cover the
   23.2 mm pocket length and add ear screw holes if they do not; otherwise
   reduce the pocket depth so a floor remains.
4. **Camera height is far below the design intent (decision needed).**
   Assembled height is roughly 62 mm + holder, putting the lens around
   90-100 mm above the desk. The original design note aims for ~200 mm
   with a ~45 degree down-tilt to overlook the whole workspace; at
   ~100 mm the OV2640 field of view covers only part of a 200 mm wide
   workspace. Options: print pole segments (split, screw-joined) to reach
   ~200 mm, or accept a smaller workspace. This does not block the
   control firmware, only the final mount geometry.
5. **Screw sizes are placeholders (check on the hardware).**
   Base holes are r = 1.7 mm (M3 clearance) at +/-24 mm; the design notes
   elsewhere assume brass heat-set inserts, whose pilot holes differ.
   The tilt arm's pan-axis hole (r = 2.6 mm) is sized for a horn center
   screw without a horn screw pattern. Confirm screw/insert sizes against
   the actual servos and the insert stock (HW-0027) before printing.
6. **No cable provision.** The ESP32-CAM needs a 5 V feed; no clip or
   channel is modelled. Add a cable clip to the pole/arm in the final
   revision so the cable cannot pull the camera off aim.

## What is ready despite the gaps

The control side (Pico + PCA9685 firmware, angle limits, sweep) does not
depend on gaps 1-5 and proceeds in parallel. Printing waits until gaps 1
and 2 are resolved and the board has been measured (user task, see the
assembly guide).
