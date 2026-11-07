# color-sort-arm

A 6-axis robotic arm that sorts small objects by color: an ESP32-CAM on an
independent pan-tilt mount watches the workspace, detects objects by color,
and the arm picks each one up and places it into the bin for its color.

The arm is built with Feetech STS3215 serial bus servos (7.4 V, C001) and
follows the SO-101 follower configuration, so LeRobot calibration and
teleoperation tooling applies. The frame uses the official SO-101 print data
(printed on a FLASHFORGE Adventurer 3, 150 x 150 x 150 mm).

Japanese README: [README.ja.md](README.ja.md)

## Hardware

| Role | Part |
|---|---|
| Joints 1-6 (base, shoulder, elbow, wrist pitch/roll, gripper) | STS3215 (7.4 V) x6 |
| Bus adapter | Waveshare Bus Servo Adapter (A) or equivalent |
| Camera | ESP32-CAM (OV2640) on a 9 g servo pan-tilt mount |
| Controller | Raspberry Pi Pico (UART to the bus adapter, USB serial to PC) |
| Power | 12 V 5 A adapter -> buck converter set to 7.4 V for the servo bus |

## Repository layout

- `software/` - PC-side C++17 software: inverse kinematics and planar
  calibration (dependency-free core), HSV color detection (OpenCV, isolated
  in one module), servo driver interface with a stub implementation, and the
  pick-and-place sequence. Unit tests included.
- `cad/` - parametric part generators kept for reference (the arm frame
  itself uses the official SO-101 print data).
- `docs/PLAN.md` - design plan. `ROADMAP.md` - development roadmap.
- `CONTRIBUTING.md` - development process (roadmap -> issues -> TDD ->
  small PRs -> review -> merge).

## Status

PC-side software foundation is in place and tested off-hardware (dry-run
with synthetic detections). Waiting for the servos to arrive; see ROADMAP.md
for the bring-up sequence.
