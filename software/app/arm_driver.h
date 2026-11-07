// Arm driver: the boundary for STS3215 bus commands.
//
// This module is the hardware I/O boundary. Upper-level logic (the
// sorting sequence in main.cpp) depends only on the ArmDriver interface
// and does not know whether real hardware is attached.
//
// Currently only StubArmDriver exists; it performs no serial I/O.
// When the hardware arrives, add a real driver in this module:
//   - Connect via Raspberry Pi Pico (USB serial relay) or directly to a
//     Waveshare Bus Servo Adapter.
//   - Write Goal Position etc. using the Feetech STS3215 protocol
//     (half-duplex UART, 1 Mbps).
//   - angleToSteps() (degrees -> 0..4095) can be reused as is.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/ik.h"

namespace arm {

// Joint name -> STS3215 ID (assumed to match the SO-101 follower layout).
inline const std::map<std::string, int>& jointIds() {
  static const std::map<std::string, int> kIds = {
      {"base", 1},        {"shoulder", 2},   {"elbow", 3},
      {"wrist_pitch", 4}, {"wrist_roll", 5}, {"gripper", 6},
  };
  return kIds;
}

// STS3215 position resolution: 360 degrees in 4096 steps.
inline constexpr int kStepsPerRevolution = 4096;

// Convert a joint angle [degrees] to an STS3215 position step (0..4095).
// center_deg is the offset under the assumption that the joint's 0
// degrees corresponds to the servo center (2048); fix it after the real
// calibration. Out-of-range results are clamped.
int angleToSteps(double angle_deg, double center_deg = 0.0);

// Joint state held by the driver (virtual or real), all in degrees.
struct ArmState {
  std::map<std::string, double> joints;  // one entry per jointIds() key
  double gripper_open = 1.0;             // 0.0 = fully closed, 1.0 = open

  ArmState();
};

// Arm driver interface.
class ArmDriver {
 public:
  virtual ~ArmDriver() = default;

  virtual void connect() = 0;
  virtual void disconnect() = 0;
  // Move to an IK solution (base/shoulder/elbow/wrist, degrees).
  virtual void moveJoints(const JointAngles& angles) = 0;
  // Move the wrist roll axis to the given angle [degrees].
  virtual void setWristRoll(double angle_deg) = 0;
  // Open/close the gripper (1.0 = fully open, 0.0 = fully closed).
  virtual void setGripper(double open_fraction) = 0;
  // Current (or last commanded) joint state.
  virtual ArmState readState() = 0;
};

// Stub for use without hardware. Records commands and updates a virtual
// state only; never touches a serial port. For dry runs.
class StubArmDriver : public ArmDriver {
 public:
  std::vector<std::string> command_log;
  bool connected = false;

  void connect() override;
  void disconnect() override;
  void moveJoints(const JointAngles& angles) override;
  void setWristRoll(double angle_deg) override;
  void setGripper(double open_fraction) override;
  ArmState readState() override;

 private:
  ArmState state_;
};

}  // namespace arm
