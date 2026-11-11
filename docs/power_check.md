# 7.4 V power rail check: procedure and pre-registered criteria (for #2)

Status: procedure, pre-hardware. The pass criteria below are fixed
before any measurement, per CONTRIBUTING.md (acceptance criteria are
decided before seeing results and are not loosened afterwards).
If a criterion turns out to be unmeasurable as written, record that
in #2 and propose the change there; do not silently adjust it.

## Setup

- Source: in-stock 12 V 5 A adapter -> buck converter (12 A class,
  CC/CV) set to 7.40 V -> bus adapter -> servo bus.
- Set the buck output to 7.40 V (+/- 0.05 V) with no load first,
  measured at the bus adapter input terminals. Only then connect
  servos. The bus adapter does not regulate: its output equals its
  input.
- Measurement points: (A) buck output terminals, (B) far end of the
  servo bus (last servo's connector), with a multimeter; log both.
- Servo logic supplies (Pico, PCA9685, ESP32-CAM) stay on their own
  supply; only GND is common, per docs/PLAN.md.

## Pre-registered pass criteria

1. No-load rail at point B: 7.35-7.45 V.
2. Load test: all 6 servos connected, torque on, simultaneous goal
   moves of shoulder + elbow + base for 30 s (the heaviest expected
   combination for this arm). Rail at point B stays at or above
   6.8 V at all times (STS3215 7.4 V version operating range starts
   at 6.0 V; 6.8 V keeps margin for wiring drops and transients a
   multimeter cannot catch).
3. No brown-out symptoms during the load test: no servo resets, no
   lost PING responses (PING all 6 before and after), no status
   error byte with the voltage bit (1) set in the post-test read.
4. Buck converter and wiring: no connector or cable section too hot
   to touch after the load test; buck output does not drift by more
   than 0.1 V between the start and end of the test.

Fail handling: if criterion 2 or 3 fails, the next step is a thicker
or shorter bus power run and a re-test, before considering a larger
supply. Record the failing values as measured.

## Record sheet (copy into #2)

| Item | Value |
|---|---|
| Buck setpoint (no load, point A) | |
| No-load rail (point B) | |
| Min rail during load test (point B) | |
| PING 1-6 before / after | / |
| Voltage error bit seen | |
| Buck drift over test | |
| Heat check | |
| Verdict vs. criteria 1-4 | |
