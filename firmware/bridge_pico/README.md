# Pico serial bridge firmware (STS3215 bus)

Transparent USB-serial <-> UART bridge between the host PC and the
STS3215 servo bus, implementing issue #5 to the design in
`docs/pico_bridge_design.md`. The bridge forwards bytes only; all
framing and packet recovery live on the host (`software/proto`,
`sts::StreamParser`). The byte ring buffers are shared with the
host-tested code in `../bridge/`.

## Wiring

| Connection | Pins |
|---|---|
| Pico UART0 -> bus adapter | GP0 = TX, GP1 = RX, 1,000,000 baud 8N1 |
| Host | USB (CDC) |
| Servo power | 7.4 V rail per docs/power_check.md; never from the Pico |

The bus adapter handles the half-duplex servo bus; the Pico side is
plain full-duplex UART. Confirm the adapter's TX/RX crossover against
the adapter documentation at first wiring (issue #5 acceptance).

## Behavior notes

- LED heartbeat: 1 s blink. If a ring buffer overflows (byte dropped),
  the LED switches to a fast blink until the next second boundary and
  the drop counter resets; sustained overflow means the host is not
  draining fast enough.
- Traffic model is request/response, so the polling pump is enough;
  this is not a sustained full-duplex 1 Mbps streaming design.

## Build and flash

Use a complete Arm GCC toolchain with a C library (the Arm GNU
Toolchain release works; a newlib-less install fails at link time).

```sh
PICO_SDK_PATH=~/pico/pico-sdk cmake -S . -B build
cmake --build build
# Hold BOOTSEL while plugging in the Pico, then copy the UF2:
cp build/bridge_pico.uf2 /Volumes/RPI-RP2/
```
