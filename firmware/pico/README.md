# Pan-tilt firmware (Raspberry Pi Pico + PCA9685)

Minimal firmware that drives the ESP32-CAM pan-tilt mount: pan sweeps
between its angle limits, tilt holds the look-down home angle. All angle,
limit and pulse logic lives in `../pan_tilt/` and is unit-tested on the
host (`cmake -S ../pan_tilt -B build-host && cmake --build build-host`).

## Wiring

| Connection | Pins |
|---|---|
| Pico I2C0 -> PCA9685 | GP4 = SDA, GP5 = SCL, address 0x40 |
| Pan servo | PCA9685 channel 0 |
| Tilt servo | PCA9685 channel 1 |
| Servo power | 5-6 V supply into PCA9685 V+ (not the Pico 5 V pin) |
| Grounds | Pico GND and servo supply GND common |

## Build and flash

Use a complete Arm GCC toolchain with a C library (the Arm GNU
Toolchain release works; a binutils-only or newlib-less install fails
at link time with missing -lc/-lg).

```sh
PICO_SDK_PATH=~/pico/pico-sdk cmake -S . -B build
cmake --build build
# Hold BOOTSEL while plugging in the Pico, then copy the UF2:
cp build/pan_tilt_pico.uf2 /Volumes/RPI-RP2/
```

## Behavior and safety

- Angles are clamped to the per-axis limits in `../pan_tilt/pan_tilt.h`
  (pan +/-85 deg, tilt -75..+35 deg) before any pulse is emitted, and
  pulses are clamped to 600..2400 us.
- If the PCA9685 does not answer on I2C, the onboard LED blinks and init
  is retried once per second; no pulses are emitted in that state.
- First power-up: run the servos unloaded (horns off) and confirm the
  sweep direction and center before attaching the camera holder.
