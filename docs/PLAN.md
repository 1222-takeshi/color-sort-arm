# Plan

## Concept

- The ESP32-CAM overlooks the workspace from an independent pan-tilt mount
  (stable viewpoint, not mounted on the arm tip).
- Objects are detected by color (HSV); centroids are mapped to workspace
  plane coordinates through a 4-point planar calibration.
- The 6-axis arm picks each object and places it into the bin for its color.
- Servo position feedback makes the origin encoder-based (no manual homing
  at every power-up) and lets grasp success be estimated from servo
  position/current in addition to the camera.

## Axes

All six joints (base rotation, shoulder pitch, elbow pitch, wrist pitch,
wrist roll, gripper) use STS3215 servos (7.4 V, C001 series; gear ratios
follow the official SO-101 follower configuration). The camera mount uses
two 9 g PWM servos driven through a PCA9685.

## Power

12 V 5 A adapter -> buck converter (12 A class, CC/CV) set to 7.4 V ->
bus adapter -> servos. Logic power (Pico, PCA9685, ESP32-CAM) is separate;
grounds are common. A 2S Li-ion pack is NOT used: at full charge (8.4 V) it
may exceed the 7.4 V servo rating.

## Software

- Controller: Raspberry Pi Pico bridges USB serial (PC) and the bus
  adapter UART.
- PC side (C++): inverse kinematics (analytic 2-link + wrist), trajectory
  via waypoints, OpenCV HSV detection, planar calibration.
- Alternative path: use the LeRobot SO-101 driver and calibration flow
  directly.

## Constraints

- Do not mix up the 7.4 V and 12 V STS3215 variants (same housing, not
  interchangeable). Orders must state 7.4 V / C001.
- Targets are light small objects (color blocks, bottle caps, ...). The
  SO-101 frame strength, not servo torque, is the practical limit.
