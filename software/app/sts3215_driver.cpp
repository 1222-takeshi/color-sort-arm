#include "app/sts3215_driver.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace arm {
namespace {

constexpr int kTimeoutMs = 100;
constexpr uint8_t kGoalAcc = 20;  // provisional acceleration (register units)

std::vector<uint8_t> idList() { return {1, 2, 3, 4, 5, 6}; }

// Inverse of angleToSteps(): step value -> joint angle [degrees].
double stepsToAngle(int steps, double center_deg) {
  return center_deg +
         (steps - 2048) * 360.0 / kStepsPerRevolution;
}

}  // namespace

std::vector<sts::StatusPacket> Sts3215ArmDriver::transact(
    const std::vector<uint8_t>& request, size_t expect, int timeout_ms) {
  transport_.write(request);
  sts::StreamParser parser;
  std::vector<sts::StatusPacket> out;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (out.size() < expect) {
    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) break;
    const int remain = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now)
            .count());
    const auto chunk = transport_.readSome(std::max(1, remain));
    if (chunk.empty()) continue;
    auto packets = parser.feed(chunk);
    out.insert(out.end(), packets.begin(), packets.end());
  }
  return out;
}

void Sts3215ArmDriver::connect() {
  transport_.flushInput();
  std::vector<int> missing;
  for (const auto& entry : jointIds()) {
    const int id = entry.second;
    const auto replies = transact(sts::pingPacket(id), 1, kTimeoutMs);
    if (replies.empty() || replies[0].id != id) missing.push_back(id);
  }
  if (!missing.empty()) {
    throw std::runtime_error("STS3215 connect: no PING reply from servo id " +
                             std::to_string(missing.front()));
  }
  // Seed the goal cache from the real positions so the first goal
  // write cannot jump. Torque stays off until the first goal.
  const ArmState now = readState();
  for (const auto& entry : jointIds()) {
    goals_[entry.second - 1] =
        angleToSteps(now.joints.at(entry.first), centers_[entry.second - 1]);
  }
  connected_ = true;
  torque_on_ = false;
}

void Sts3215ArmDriver::disconnect() {
  if (!connected_) return;
  for (const auto& entry : jointIds()) {
    transport_.write(
        sts::writePacket(entry.second, sts::kRegTorqueEnable, {0}));
  }
  torque_on_ = false;
  connected_ = false;
}

void Sts3215ArmDriver::sendGoals() {
  if (!connected_) throw std::runtime_error("STS3215 driver not connected");
  if (!torque_on_) {
    for (const auto& entry : jointIds()) {
      transport_.write(
          sts::writePacket(entry.second, sts::kRegTorqueEnable, {1}));
    }
    torque_on_ = true;
  }
  std::vector<std::pair<uint8_t, std::vector<uint8_t>>> items;
  for (const uint8_t id : idList()) {
    uint8_t le[2];
    sts::encodeU16(static_cast<uint16_t>(goals_[id - 1]), le);
    items.push_back({id, {kGoalAcc, le[0], le[1], 0, 0, 0, 0}});
  }
  transport_.write(sts::syncWritePacket(sts::kRegAcc, 7, items));
}

void Sts3215ArmDriver::moveJoints(const JointAngles& angles) {
  setGoal(1, angles.base);
  setGoal(2, angles.shoulder);
  setGoal(3, angles.elbow);
  setGoal(4, angles.wrist);
  sendGoals();
}

void Sts3215ArmDriver::setWristRoll(double angle_deg) {
  setGoal(5, angle_deg);
  sendGoals();
}

void Sts3215ArmDriver::setGripper(double open_fraction) {
  open_fraction = std::max(0.0, std::min(1.0, open_fraction));
  // Provisional mapping, same convention as the stub (0..90 degrees);
  // replaced by the measured endpoints from the #4 calibration.
  setGoal(6, open_fraction * 90.0);
  sendGoals();
}

ArmState Sts3215ArmDriver::readState() {
  const auto replies = transact(
      sts::syncReadPacket(sts::kRegPresentPosition, 6, idList()), 6,
      kTimeoutMs * 2);
  if (replies.size() < 6) {
    throw std::runtime_error("STS3215 readState: expected 6 replies, got " +
                             std::to_string(replies.size()));
  }
  std::array<int, 6> steps{};
  for (const auto& st : replies) {
    if (st.id < 1 || st.id > 6 || st.params.size() < 6) continue;
    steps[st.id - 1] =
        sts::decodeSignedMagnitude(sts::decodeU16(st.params.data()), 15);
  }
  ArmState state;
  for (const auto& entry : jointIds()) {
    const int id = entry.second;
    state.joints[entry.first] = stepsToAngle(steps[id - 1], centers_[id - 1]);
  }
  state.gripper_open =
      std::max(0.0, std::min(1.0, state.joints["gripper"] / 90.0));
  return state;
}

}  // namespace arm
