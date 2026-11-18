# SO-101 frame printing: checklist (for #3)

Status: in progress. Step 1 (download) and step 2 (fit check) were
done on 2026-11-18; step 3 and printing remain. Nothing is printed
until the owner approves printing (parts, time, and filament are
reported first).

## Step 1: obtain the official data

- Done 2026-11-18: downloaded from the official source,
  https://github.com/TheRobotStudio/SO-ARM100 (STL/SO101/Individual/),
  upstream main at commit a758567c3978 (2026-10-06). Files are kept
  outside the repo at ~/workspace/so101-stl/ on the Mac (15 STLs).
  The combined print plates under STL/SO101/Follower/ target other
  printers' beds and are not used; the individual parts below are
  the print set.
- The frame must be the follower configuration (this arm has no
  leader). Camera pan-tilt parts are custom and are NOT part of this
  checklist (see docs/pan_tilt_mount.md).
- Role column below: "leader only" parts are not printed. Two pairs
  are variants to resolve against the official assembly before
  printing: Base_motor_holder vs Motor_holder_SO101_Base, and the
  mounting plates (this build uses the WaveShare adapter, so the
  WaveShare plate is the default choice).

## Step 2: per-part fit check (Adventurer 3, 150 x 150 x 150 mm)

For every STL: measure the bounding box (slicer or a script), choose
a print orientation, and fill in one row. A part passes if every
dimension is <= 150 mm in the chosen orientation; plan with a 140 mm
working limit so brims and skirts fit. If a part fails, record the
split plan (cut plane + joining method: screws / inserts) instead of
scaling the part - never scale frame parts, the servo pockets and
shaft spacing are dimensioned for the official size.

| # | File | BBox X x Y x Z [mm] | Role | Fits | Notes |
|---|---|---|---|---|---|
| 1 | Base_SO101.stl | 110.9 x 72.0 x 87.0 | follower | yes | |
| 2 | Base_motor_holder_SO101.stl | 47.6 x 31.4 x 80.2 | variant pair | yes | choose vs #4 |
| 3 | Handle_SO101.stl | 88.1 x 25.0 x 60.1 | leader only | - | do not print |
| 4 | Motor_holder_SO101_Base.stl | 28.0 x 38.1 x 55.1 | variant pair | yes | choose vs #2 |
| 5 | Motor_holder_SO101_Wrist.stl | 28.0 x 36.9 x 56.4 | follower | yes | |
| 6 | Moving_Jaw_SO101.stl | 22.3 x 92.0 x 48.0 | follower | yes | gripper jaw |
| 7 | Rotation_Pitch_SO101.stl | 59.8 x 84.2 x 46.0 | follower | yes | |
| 8 | Seeedstudio_Mounting_Plate_SO101.stl | 51.0 x 42.0 x 7.6 | alt. plate | yes | not used (WaveShare) |
| 9 | Trigger_SO101.stl | 42.0 x 61.0 x 32.0 | leader only | - | do not print |
| 10 | Under_arm_SO101.stl | 130.7 x 24.0 x 64.4 | follower | yes | |
| 11 | Upper_arm_SO101.stl | 142.1 x 24.5 x 67.3 | follower | volume only | see below |
| 12 | WaveShare_Mounting_Plate_SO101.stl | 51.0 x 42.0 x 7.6 | follower | yes | this build's adapter |
| 13 | Wrist_Roll_Follower_SO101.stl | 65.2 x 52.0 x 105.4 | follower | yes | |
| 14 | Wrist_Roll_Pitch_SO101.stl | 62.3 x 35.7 x 77.8 | follower | yes | |
| 15 | Wrist_Roll_SO101.stl | 65.2 x 52.0 x 42.8 | leader/generic | - | follower uses #13 |

Bounding boxes are as stored in the STL (measured by script,
2026-11-18), before any slicer orientation. Verdict summary: every
follower part fits the 150 mm build volume in its stored
orientation, so no part needs splitting and none may be scaled.
One caution: Upper_arm's longest dimension (142.1 mm) exceeds the
140 mm working limit, leaving no room for a brim along that axis -
print it diagonally on the bed (the 150 x 150 diagonal is ~212 mm)
or without a brim. Final orientation, time, and filament per part
are filled in at slicing time (step 3).

## Step 3: pre-print readiness

- [ ] Filament on hand covers the total estimate from step 2, plus
      one spare part's worth. (The org-side estimate for a full
      SO-101 set was roughly 51 hours of printing; treat as an
      order-of-magnitude reference, not a quote.)
- [ ] Fasteners ready per the official BOM: M3 screws in the lengths
      the BOM lists, plus the brass heat-set inserts used elsewhere
      in this project (HW-0027). Record any missing length here
      before printing starts.
- [ ] Printer check: bed adhesion test / calibration cube within
      tolerance; servo pocket dimensions depend on it.
- [ ] One test print first: the part with a servo pocket, checked
      against a real STS3215 (body 40.7 x 20 mm class pocket fit)
      before printing the whole set.

## Acceptance (from #3, restated)

All frame parts printed and listed in the table, each verified
against the build volume. The filled-in table is pasted into #3.
