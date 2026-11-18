# Assembly check, calibration, and first-motion check (for #4)

Status: procedure, pre-hardware. Written in advance so the session runs
straight through once the servos and frame are on the bench. Nothing has
been measured yet; all value fields are filled in during the real session
and pasted back into #4 (see Sign-off).

Servo map (STS3215, 7.4 V, 4096 steps/rev, center 2048): base = 1,
shoulder = 2, elbow = 3, wrist_pitch = 4, wrist_roll = 5, gripper = 6.

## Prerequisites

- #1 bring-up done: IDs 1-6 assigned and verified per docs/servo_bringup.md.
- #2 power check passed per docs/power_check.md: bench PSU (LONGWEI
  LW-K3010D) set to 7.4 V, or the 12 V adapter + buck chain. Never feed
  12 V directly to the 7.4 V servos; the bus adapter passes its input
  voltage straight through.
- Tools: calipers, ruler (300 mm), multimeter, PC with the `color_sort`
  binary built from this repo; this sheet printed or open, pen for tables.

## Assembly verification

Torque OFF everywhere; move joints by hand only.

- [ ] All frame screws present and snug; no cracked printed parts,
      especially around the servo pockets.
- [ ] Each servo horn was fixed while its servo sat at step 2048
      (mechanical center), per #1. If a horn went on at an unknown
      position, note the joint here and let the offset calibration below
      absorb it: _______________
- [ ] Horn and center screws tight; no play between horn and printed part.
- [ ] Cable slack: swing every joint slowly through its full range. The
      bus cable must never pull taut, pinch, or rub a sharp edge; leave a
      small service loop at each joint.
- [ ] Gripper fingers mounted, parallel, free to open/close by hand.
- [ ] No interference at the extremes: at every joint's mechanical limits,
      links, cables, and the camera mount clear each other and the plane.

## Link length measurement

Measure axis to axis (rotation center to rotation center), not part edges.
Use calipers where they reach; for longer spans, mark both axis centers on
paper/tape and measure with the ruler. Measure twice, record the second
reading. The values replace the provisional `ArmParams` defaults in
software/core/ik.h; measure the assembled arm, not the CAD model. For link3
the tool point is the midpoint between the closed fingertips; for shoulder_z,
measure from the surface the calibration markers sit on.

| Parameter | Meaning | Nominal [mm] | Measured [mm] | Diff [mm] |
|---|---|---|---|---|
| link1 | shoulder axis -> elbow axis | 115 | | |
| link2 | elbow axis -> wrist axis | 115 | | |
| link3 | wrist axis -> gripper tip (tool point) | 90 | | |
| shoulder_z | work plane -> shoulder axis height | 95 | | |
| shoulder_offset | base axis -> shoulder axis (horizontal) | 0 | | |

## Joint offset calibration

Goal: for every joint, find the step count that corresponds to its logical
0 degrees. One step = 360 / 4096 = 0.0879 degrees.

Reference pose (define once, always return to it): base facing +x; upper
arm and forearm both horizontal, pointing forward (elbow fully straight);
wrist pitched so the tool points straight down; gripper half open.

1. Torque OFF. Move the arm into the reference pose by hand, checking
   horizontal/vertical with a ruler or square.
2. Read PRESENT_POSITION of all 6 servos; record raw steps in the table.
3. Per joint, center offset = read_steps - 2048. Applying it means:
   steps = 2048 + offset + angle_deg / 0.0879, with the sign flipped for
   any joint mounted opposite to the IK convention in software/core/ik.h
   (note flips in the table).
4. This sheet is the record of truth for the session. In code the
   offsets are passed to `Sts3215ArmDriver::setJointCenters()` in
   software/app/sts3215_driver.h (center_deg = offset_steps * 0.0879
   per joint), which feeds the `center_deg` of `angleToSteps()`.
5. Acceptance: apply the offsets, return to the reference pose, re-read.
   Every joint must report within +/- 2 degrees of its reference angle;
   if one is out, redo steps 1-3 for that joint only.

| Joint | ID | Ref-pose steps | Offset [steps] | Sign flip? | Re-check [deg] |
|---|---|---|---|---|---|
| base | 1 | | | | |
| shoulder | 2 | | | | |
| elbow | 3 | | | | |
| wrist_pitch | 4 | | | | |
| wrist_roll | 5 | | | | |
| gripper | 6 | | | n/a | |

## Gripper range finding

Both drivers currently map the gripper provisionally
(open_fraction x 90 degrees); that placeholder is replaced by the
measured endpoints found here.

1. Torque ON, gripper empty, current-limited supply as in #2.
2. From mid-range, step the goal in small increments (~20 steps) toward
   closed. Stop at the first increment where the fingers are fully closed
   and PRESENT_POSITION stops advancing; record it as `closed_steps`.
   Never hold the servo stalled against the hard stop; back off at once.
3. Repeat toward open; record `open_steps` at the limit, same stop rule.
4. Sanity check: command 0.0 / 0.5 / 1.0 open fractions mapped linearly
   between the endpoints; the gripper must track without stalling.

| Gripper | Steps |
|---|---|
| closed_steps | |
| open_steps | |

## Camera plane calibration

Calibrates the pixel -> work-plane mapping of `PlaneCalibration` in software/core/calibration.h.

1. Place at least 4 markers at known work-plane coordinates spanning the
   sorting area (near its corners, not in a line); 6-9 markers including
   one near the center improve the fit. Record each marker's (x, y).
2. With the ESP32-CAM in its final mounted position, capture a frame and
   record each marker's pixel coordinates (u, v).
3. Compute the calibration from the correspondences; save it in
   PlaneCalibration's save/load text format and keep it with the record.
4. Residuals: push every marker's pixels back through the calibration.
   Acceptance: reprojection error <= 2 mm at every marker. If one marker
   is far off, re-check its recorded coordinates before adding markers.

| Marker | World x [mm] | World y [mm] | Pixel u | Pixel v | Residual [mm] |
|---|---|---|---|---|---|
| 1 | | | | | |
| 2 | | | | | |
| 3 | | | | | |
| 4 | | | | | |
| 5 | | | | | |
| 6 | | | | | |

## First-motion check

Bench PSU current limit ~1 A for this whole phase. One joint at a time; hand near the supply switch. Do not run the sorting sequence yet.

1. PING all 6 servos; all must answer.
2. Read PRESENT_POSITION of all 6; values plausible (0-4095), stable across two reads.
3. Torque ON for all joints. The arm must hold position without sagging,
   buzzing, or drifting.
4. Per joint, in this order — wrist_roll, gripper, wrist_pitch, elbow,
   shoulder, base: command +10 degrees from the current position, wait
   for the move to settle, record the reached position, command back.
5. Combined move: command the reference pose at low speed, all joints
   together. Watch the whole path, not just the endpoint.
6. Torque OFF. Session done.

Stop immediately (torque OFF, then investigate) if any of: a status packet
shows any error bit set (voltage, angle, overheating, over-current,
overload); PRESENT_LOAD spikes or stays high on an unloaded joint; abnormal
noise, vibration, smell, or a hot servo case; a joint's reached position
differs from its goal by more than ~5 degrees after settling.

| Joint | Goal [deg] | Reached [deg] | Back OK? | Notes |
|---|---|---|---|---|
| wrist_roll | | | | |
| gripper | | | | |
| wrist_pitch | | | | |
| elbow | | | | |
| shoulder | | | | |
| base | | | | |
| combined: reference pose | n/a | | | |

## Sign-off

Paste into #4 when the session is done: the link length table with the
measured ArmParams values (plus the commit updating software/core/ik.h, if
they changed); the joint offset table including any sign flips; gripper
closed_steps / open_steps; the saved camera calibration file contents and
the marker residuals; the first-motion record table, plus any stop-criterion
events and how they were resolved.
