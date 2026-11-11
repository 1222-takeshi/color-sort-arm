# Color-Sort Arm — PC-side Software (C++)

PC-side software for the 6-axis color-sorting pick-and-place arm
(STS3215 bus servos + ESP32-CAM vision). The detection -> coordinate
conversion -> inverse kinematics -> drive pipeline runs end to end as a
dry run with a stub driver; the real hardware has not arrived yet.

## Layout

| Path | Contents |
|---|---|
| `core/` | Dependency-free pure C++ library. `ik` (planar 2-link + wrist IK/FK, parameterized geometry, reachability check) and `calibration` (4-point homography, computed with a hand-written Gaussian-elimination solver; plain-text save/load). |
| `vision/` | HSV color detection. `detection.h` holds the shared data types and has no OpenCV dependency; `color_detector.*` is the only code that uses OpenCV (BGR -> HSV, per-color masks with red hue wrap-around, morphology, contours). |
| `app/` | `arm_driver` (driver interface + stub implementation; includes the STS3215 joint-ID map and degrees-to-steps conversion) and `main.cpp` (the pick-and-place sequence). |
| `tests/` | Tests for `ik` and `calibration` in a single assert-style binary with no external dependencies. |

## Build

```sh
cmake -S . -B build
cmake --build build
./build/arm_tests            # unit tests (core only, no OpenCV needed)
./build/color_sort --synthetic   # dry run with synthetic detections
```

Requirements: a C++17 compiler and CMake. OpenCV is optional: if CMake
finds it, the vision module is built and the `--image` mode is enabled;
without OpenCV only the `--synthetic` dry run is available. The
`--synthetic` path constructs its detections directly and never touches
an image, so it works in the no-OpenCV build.

For real images (OpenCV build only):

```sh
./build/color_sort --image photo.jpg --calibration calib.txt
```

Note: the calibration file format is the plain-text format written by
`PlaneCalibration::save` in `core/calibration.cpp`, not JSON.

## Not yet verified

- `vision/` and the `--image` path were compile-checked and run
  against a synthetic image on macOS with OpenCV 5.1 (2026-11-11).
  OpenCV 5 moved some imgproc functions into the geometry module;
  `vision/color_detector.cpp` includes that header under a version
  guard, so OpenCV 4 builds keep working. Real-camera HSV ranges are
  still untuned (see issue #7).
- Everything hardware-facing is a stub: no serial communication takes
  place anywhere yet.
- The IK link lengths in `ik.h` (`ArmParams`) are provisional estimates
  for an SO-101-class arm, not measured values.

## To replace when the hardware arrives

1. `core/ik.h`: set `ArmParams` to the measured link lengths.
2. `app/arm_driver.*`: add a real driver implementing `ArmDriver`
   (Pico USB-serial relay or a bus servo adapter; the degrees-to-steps
   conversion can be reused), and switch `main.cpp` to use it.
3. `app/main.cpp`: replace the frame source with the ESP32-CAM stream,
   and tune the HSV ranges, placement spots, and grasp heights to the
   real setup.
