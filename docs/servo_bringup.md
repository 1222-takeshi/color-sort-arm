# STS3215 bring-up procedure (for #1)

Status: procedure, pre-hardware. Run this when the servos arrive.
One servo on the bus at a time until IDs are assigned. Record results
in the table at the bottom and paste it into #1.

## Preconditions

- 7.4 V rail verified first (see docs/power_check.md). Never power a
  7.4 V (C001) servo from 12 V: the bus adapter passes its input
  voltage straight through to the servos.
- Bus adapter + Pico bridge (#5) ready, or a direct PC-to-adapter
  USB connection for this session.
- Servo horns NOT attached during ID assignment.

## Steps

1. Connect exactly one servo (factory default ID is 1). PING ID 1.
   No response: check rail voltage, wiring, and adapter direction
   before trying another servo.
2. Single-servo check at the default ID:
   - Read PRESENT_POSITION (reg 56); turn the shaft by hand and
     confirm the value changes smoothly over the range.
   - Write GOAL_POSITION +1024 ticks from the current position with
     torque on; confirm motion and read-back within a few ticks.
   - Note the rotation direction vs. increasing position for the
     record sheet (needed for the joint direction map in #4).
3. Assign the target ID (joint map from software/app/arm_driver.h):
   base=1, shoulder=2, elbow=3, wrist_pitch=4, wrist_roll=5,
   gripper=6. Sequence: WRITE LOCK (55) = 0, WRITE ID (5) = target,
   WRITE LOCK (55) = 1, then PING at the target ID and confirm the
   old ID no longer answers.
4. Label the servo with its ID (tape on the case) before disconnecting.
5. Repeat steps 1-4 for the remaining five servos.
6. Full-bus check: connect all six, PING each ID 1-6, then SYNC_READ
   present position (56, 6 bytes) for all and confirm six sane
   responses with no checksum errors.

## Acceptance (from #1, restated)

All 6 servos respond at their assigned IDs, move on command, and read
back position correctly. Any servo failing step 2 is set aside as a
suspect unit and noted in the record; do not assign it an ID.

## Record sheet (copy into #1)

| Servo (joint) | Target ID | PING at 1 | Hand-turn readback | Goal move + readback | Direction vs. ticks | New ID PING | Notes |
|---|---|---|---|---|---|---|---|
| base | 1 | | | | | | |
| shoulder | 2 | | | | | | |
| elbow | 3 | | | | | | |
| wrist_pitch | 4 | | | | | | |
| wrist_roll | 5 | | | | | | |
| gripper | 6 | | | | | | |

Full-bus check: PING 1-6 OK? ___  SYNC_READ errors: ___
