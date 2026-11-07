# Pan-tilt mount: user tasks (printing, assembly, stream check)

These steps need the physical parts and are done by the owner. Everything
else (CAD, firmware, tests) is prepared in this repository. Do the steps
in order; steps 1-2 unblock the final CAD revision.

## 1. Measure the ESP32-CAM board (about 10 min)

Needed to fix the holder size (see docs/pan_tilt_mount.md, gap 1).

- Measure the board width and height with calipers or a ruler, and the
  lens center offset from the board edges (left edge and top edge).
- Note which edge carries the pin header / reset button.
- A photo of the board next to a ruler, front and back, covers all of
  this at once. Share the values/photo and the CAD is updated to match.

## 2. Confirm the in-stock parts (about 10 min)

- 2x 9 g servos with horns and horn screws, 1x PCA9685 board,
  1x Raspberry Pi Pico, jumper wires, and a 5-6 V supply for the servos
  (a USB power bank or a 5 V adapter is fine for the bench check).
- Check each servo horn fits its output shaft; set both servos aside for
  this project.

## 3. Print the parts (after the CAD revision is merged)

Print list and settings are finalized with the CAD revision (holder size
and the tilt servo mount depend on step 1). Adventurer 3, standard
profile; the three current parts need no supports except the holder lip.

## 4. Assemble (about 30 min, after printing)

1. Fit the pan servo into the base pocket; confirm the flange ears sit
   flat (gap 3 in the verification notes) and fix with the ear screws.
2. Power the pan servo from the PCA9685 (channel 0) with the horn off,
   run the firmware sweep once, and confirm the servo centers at the
   middle of the sweep. Then attach the tilt arm to the horn, pointing
   straight up at the center position.
3. Fit the tilt servo per the revised CAD, attach the holder, and slide
   the ESP32-CAM into the holder. Do not force the board; the holder is
   sized from the step 1 measurements.
4. Route the ESP32-CAM power cable along the arm with a clip so it
   cannot pull on the holder.

## 5. Wire and first power-up (about 15 min)

Follow the wiring table in firmware/pico/README.md. Rules:

- Servo power goes to the PCA9685 V+ from the separate 5-6 V supply,
  never from the Pico. Pico GND and supply GND are common.
- First run: camera holder empty. Confirm pan sweeps smoothly between
  its limits and tilt holds the look-down angle. Only then mount the
  ESP32-CAM.

## 6. ESP32-CAM stream check (about 20 min)

1. Flash the standard CameraWebServer example to the ESP32-CAM
   (AI-Thinker board profile), with the home Wi-Fi credentials.
2. Open the stream URL shown on the serial monitor in a PC browser and
   confirm a stable image for a few minutes.
3. From the final mount position, confirm the workspace fits in the
   frame; if it does not, note the visible area - the pole height
   decision (gap 4) uses this result.

Record the results of steps 1, 5 and 6 (values, photos, stream OK/NG) so
the issue can be updated and the vision integration (#7) can start from
verified numbers.
