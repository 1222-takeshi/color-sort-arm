#include "app/arm_driver.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace arm {

int angleToSteps(double angle_deg, double center_deg) {
  const long steps = std::lround(
      2048.0 + (angle_deg - center_deg) * kStepsPerRevolution / 360.0);
  return static_cast<int>(
      std::max(0L, std::min(static_cast<long>(kStepsPerRevolution - 1), steps)));
}

ArmState::ArmState() {
  for (const auto& entry : jointIds()) joints[entry.first] = 0.0;
}

void StubArmDriver::connect() {
  connected = true;
  command_log.push_back("connect");
  std::cout << "[stub] connect\n";
}

void StubArmDriver::disconnect() {
  connected = false;
  command_log.push_back("disconnect");
  std::cout << "[stub] disconnect\n";
}

void StubArmDriver::moveJoints(const JointAngles& angles) {
  state_.joints["base"] = angles.base;
  state_.joints["shoulder"] = angles.shoulder;
  state_.joints["elbow"] = angles.elbow;
  state_.joints["wrist_pitch"] = angles.wrist;
  char buf[160];
  std::snprintf(buf, sizeof(buf),
                "move base=%.1f shoulder=%.1f elbow=%.1f wrist=%.1f",
                angles.base, angles.shoulder, angles.elbow, angles.wrist);
  command_log.emplace_back(buf);
  std::cout << "[stub] " << buf << "\n";
}

void StubArmDriver::setWristRoll(double angle_deg) {
  state_.joints["wrist_roll"] = angle_deg;
  char buf[64];
  std::snprintf(buf, sizeof(buf), "wrist_roll=%.1f", angle_deg);
  command_log.emplace_back(buf);
  std::cout << "[stub] " << buf << "\n";
}

void StubArmDriver::setGripper(double open_fraction) {
  open_fraction = std::max(0.0, std::min(1.0, open_fraction));
  state_.gripper_open = open_fraction;
  state_.joints["gripper"] = open_fraction * 90.0;  // virtual opening angle
  char buf[64];
  std::snprintf(buf, sizeof(buf), "gripper=%.2f", open_fraction);
  command_log.emplace_back(buf);
  std::cout << "[stub] " << buf << "\n";
}

ArmState StubArmDriver::readState() { return state_; }

}  // namespace arm
