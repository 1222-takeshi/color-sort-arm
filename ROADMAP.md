# Roadmap

Development follows the process in CONTRIBUTING.md: roadmap -> issues ->
TDD (tests first) -> small PRs (roughly 200 lines or less) -> AI review ->
merge.

## M0 - Foundations (done)

- [x] PC-side software skeleton in C++: color detection, planar
  calibration, inverse kinematics, pick-and-place sequence (dry-run)
- [x] Unit tests for IK and calibration passing

## M1 - Hardware bring-up

- [ ] STS3215 ID assignment and single-servo check over the bus adapter
- [ ] 7.4 V power rail check under load (voltage sag with all joints)
- [ ] Frame printing from the official SO-101 data (verify each part fits
  the Adventurer 3 build volume; camera pan-tilt mount is custom)

## M2 - Assembly and calibration

- [ ] Assemble the arm; LeRobot-style calibration
- [ ] Measure actual link lengths and update `ArmParams`

## M3 - Real servo driver

- [ ] Pico serial bridge firmware (USB serial <-> bus adapter UART)
- [ ] Real `ArmDriver` implementation replacing the stub

## M4 - Vision integration

- [ ] ESP32-CAM stream into the main loop
- [ ] HSV range tuning and workspace calibration on the real setup

## M5 - End-to-end sorting

- [ ] Detect -> pick -> place loop running continuously
- [ ] Grasp verification using servo position/current feedback plus camera
