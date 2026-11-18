// Real ArmDriver for the STS3215 bus, over an abstract byte transport.
//
// The transport is whatever connects the host to the servo bus: the
// Pico bridge (USB serial, firmware side is issue #5) or a Waveshare
// bus adapter directly. This class owns the protocol-level behavior
// fixed in docs/pico_bridge_design.md:
//   - torque stays OFF after connect; it is enabled together with the
//     first goal write, and disabled again on disconnect.
//   - goals go out as one SYNC_WRITE starting at ACC (7 bytes per
//     servo: ACC, goal position, goal time 0, goal speed 0).
//   - feedback is one SYNC_READ of 6 bytes from PRESENT_POSITION
//     (position, speed, load), decoded with the sign-magnitude rules.
// Joint angle <-> step conversion reuses angleToSteps(); per-joint
// center offsets default to 0 and are set from the #4 calibration.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "app/arm_driver.h"
#include "proto/sts3215.h"

namespace arm {

// Byte stream to the servo bus. readSome returns whatever arrived
// within the timeout (possibly empty); framing is the caller's job
// (see sts::StreamParser).
class ByteTransport {
 public:
  virtual ~ByteTransport() = default;
  virtual void write(const std::vector<uint8_t>& bytes) = 0;
  virtual std::vector<uint8_t> readSome(int timeout_ms) = 0;
  virtual void flushInput() = 0;
};

class Sts3215ArmDriver : public ArmDriver {
 public:
  explicit Sts3215ArmDriver(ByteTransport& transport)
      : transport_(transport) {}

  // Per-joint center offsets [degrees], indexed by servo ID - 1.
  // Filled in from the #4 calibration; defaults are all zero.
  void setJointCenters(const std::array<double, 6>& centers_deg) {
    centers_ = centers_deg;
  }

  void connect() override;
  void disconnect() override;
  void moveJoints(const JointAngles& angles) override;
  void setWristRoll(double angle_deg) override;
  void setGripper(double open_fraction) override;
  ArmState readState() override;

 private:
  // Send the request, then collect status packets until `expect`
  // arrive or the deadline passes. Returns what arrived.
  std::vector<sts::StatusPacket> transact(
      const std::vector<uint8_t>& request, size_t expect, int timeout_ms);
  void sendGoals();  // enables torque on the first call
  int goalSteps(int id) const { return goals_[id - 1]; }
  void setGoal(int id, double angle_deg) {
    goals_[id - 1] = angleToSteps(angle_deg, centers_[id - 1]);
  }

  ByteTransport& transport_;
  std::array<double, 6> centers_{};
  std::array<int, 6> goals_{2048, 2048, 2048, 2048, 2048, 2048};
  bool connected_ = false;
  bool torque_on_ = false;
};

}  // namespace arm
