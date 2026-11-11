# Pico serial bridge: design (for #5, input to #6)

Status: design, pre-hardware. The STS3215 servos have not arrived; this
document fixes the bridge design so the firmware (#5) and the host
driver (#6) can be built and tested against it.

## Role

The Raspberry Pi Pico sits between the PC (USB serial / CDC) and the
Waveshare Bus Servo Adapter (UART, half-duplex, 1 Mbps). The PC-side
driver speaks the Feetech packet protocol end to end; the Pico is a
transparent forwarder with frame recovery, not a protocol translator.

Why transparent: the same packet definitions are already implemented
and tested on the org side (SO-101 evaluation track, Python
`so101_hardware/protocol.py`). Keeping one definition source avoids a
second, divergent implementation of register addresses and framing in
the bridge. The C++ driver in this repo (#6) transcribes the same
definitions; any change to a definition must be mirrored in both
places and noted in the PR that changes it.

## Packet format (shared definition, transcribed)

- Instruction packet: `FF FF ID LEN INST PARAM... CHK`
- Status packet: `FF FF ID LEN ERR PARAM... CHK`
- `LEN` = number of PARAM bytes + 2. Multi-byte values are
  little-endian. `CHK` = `(~(ID + LEN + INST/ERR + sum(PARAM))) & 0xFF`.
- Instructions used: PING 0x01, READ 0x02, WRITE 0x03, REG_WRITE 0x04,
  ACTION 0x05, SYNC_READ 0x82, SYNC_WRITE 0x83. Broadcast ID 0xFE.
- Registers used (STS3215): ID 5, BAUD_RATE 6, MODE 33,
  TORQUE_ENABLE 40, ACC 41, GOAL_POSITION 42 (followed by GOAL_TIME 44
  and GOAL_SPEED 46, writable as one 7-byte block starting at ACC),
  LOCK 55, PRESENT_POSITION 56 (followed by PRESENT_SPEED 58 and
  PRESENT_LOAD 60; a 6-byte read starting at 56 returns all three).
- Position is 0..4095 per revolution (4096 ticks). Position and speed
  use sign-magnitude encoding (bit 15 is the sign); load uses bit 10.
- Status error byte bits: voltage 1, angle limit 2, overheating 4,
  over-current 8, overload 32.

## Forwarding and frame recovery (Pico side)

- USB CDC <-> UART forwarding in both directions, byte-transparent.
- The Pico tracks frame boundaries only to recover from noise: it
  scans for the `FF FF` header, reads LEN, and forwards exactly
  LEN + 4 bytes as one frame. Bytes before a header are dropped.
  A frame whose checksum fails is still forwarded (the host driver
  owns validation and error counting, as in the org implementation);
  the Pico never rewrites packet contents.
- Half-duplex turnaround is owned by the bus adapter hardware; the
  Pico only moves bytes between the two ports. UART: 1 Mbps, 8N1.
- Buffering: the Pico keeps a small ring buffer per direction
  (2 KB is enough: the largest expected frame is a SYNC_WRITE for
  6 servos, well under 100 bytes; status bursts are similar). On
  overflow the partial frame is dropped and the scanner resyncs at
  the next header. No flow control is assumed on the USB side.

## Command patterns the driver will use (fixed here for #6)

- Bring-up (#1): PING per ID; ID change = WRITE LOCK=0, WRITE ID,
  WRITE LOCK=1, then PING at the new ID (same sequence as the org
  implementation's `set_id`). One servo on the bus at a time.
- Motion: SYNC_WRITE starting at ACC (41), 7 data bytes per servo:
  ACC, GOAL_POSITION (2), GOAL_TIME = 0 (2), GOAL_SPEED (2).
- Feedback: SYNC_READ starting at PRESENT_POSITION (56), 6 bytes per
  servo, giving position / speed / load in one round trip.
- Torque: SYNC_WRITE at TORQUE_ENABLE (40), 1 byte per servo.
  Torque stays off until the first valid goal is written, matching
  the org driver's safety behavior.

## Host-side test plan (no hardware, for #6)

- Packet builder/parser unit tests: golden vectors for PING, WRITE
  (goal position), SYNC_WRITE (6 servos), SYNC_READ request, and
  status parsing including the error byte. The org Python test suite
  (`so101_hardware/tests`) is the cross-check source for vectors.
- Stream parser tests: garbage bytes before a header, a frame split
  across reads, a corrupted checksum, and back-to-back frames must
  resync the same way as the org parser (skip 1 byte on a bad frame
  and rescan).

## Alternatives considered

- Smart bridge (Pico parses commands and generates packets):
  rejected. It duplicates the protocol definitions in a third place
  and makes the PC driver thinner than the org driver for no gain.
- Direct PC-to-adapter connection without the Pico: workable for
  bring-up (#1 can use it), but the target configuration keeps the
  Pico in the loop (USB isolation from the servo bus and a future
  standalone mode), so the bridge is built as designed above.

## Open points

- Confirm the bus adapter's exact UART side wiring (TX/RX swap) and
  that its direction switching needs no Pico GPIO. Checked at first
  power-up (#1 session), recorded in that issue.
- USB CDC throughput at 1 Mbps UART is not a bottleneck for the
  expected frame sizes; re-evaluate only if 50 Hz full feedback for
  6 servos is attempted (org-side open question as well).
