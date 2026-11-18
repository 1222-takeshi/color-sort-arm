// Tests for Sts3215ArmDriver over a fake transport that answers
// PING and SYNC_READ like a real bus. Assert-style, no dependencies.
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <vector>

#include "app/sts3215_driver.h"

namespace {
int g_checks = 0;
int g_failures = 0;
void report(bool ok, const char* expr, int line) {
  ++g_checks;
  if (!ok) {
    ++g_failures;
    std::printf("FAIL (line %d): %s\n", line, expr);
  }
}
#define CHECK(cond) report(static_cast<bool>(cond), #cond, __LINE__)
#define CHECK_NEAR(a, b) \
  report(std::fabs((a) - (b)) < 0.01, #a " ~= " #b, __LINE__)

std::vector<uint8_t> statusPacket(uint8_t id,
                                  const std::vector<uint8_t>& params) {
  std::vector<uint8_t> p = {0xFF, 0xFF, id,
                            static_cast<uint8_t>(params.size() + 2), 0};
  p.insert(p.end(), params.begin(), params.end());
  p.push_back(sts::checksum(p.data() + 2, p.size() - 2));
  return p;
}

// Splits a byte stream into packets using the LEN field.
std::vector<std::vector<uint8_t>> splitPackets(
    const std::vector<uint8_t>& stream) {
  std::vector<std::vector<uint8_t>> out;
  size_t i = 0;
  while (i + 4 <= stream.size()) {
    const size_t total = static_cast<size_t>(stream[i + 3]) + 4;
    if (i + total > stream.size()) break;
    out.emplace_back(stream.begin() + i, stream.begin() + i + total);
    i += total;
  }
  return out;
}

class FakeTransport : public arm::ByteTransport {
 public:
  std::vector<uint8_t> written;
  std::map<int, int> positions;  // id -> present position steps
  std::set<int> silent;
  std::vector<std::pair<int, int>> torque_writes;  // (id, value)

  void write(const std::vector<uint8_t>& bytes) override {
    written.insert(written.end(), bytes.begin(), bytes.end());
    for (const auto& pkt : splitPackets(bytes)) handle(pkt);
  }
  std::vector<uint8_t> readSome(int) override {
    auto out = pending_;
    pending_.clear();
    return out;
  }
  void flushInput() override { pending_.clear(); }

 private:
  std::vector<uint8_t> pending_;
  void reply(const std::vector<uint8_t>& pkt) {
    pending_.insert(pending_.end(), pkt.begin(), pkt.end());
  }
  void handle(const std::vector<uint8_t>& pkt) {
    if (pkt.size() < 5) return;
    const uint8_t id = pkt[2];
    const uint8_t inst = pkt[4];
    if (inst == sts::kInstPing && !silent.count(id)) {
      reply(statusPacket(id, {}));
    } else if (inst == sts::kInstWrite && pkt.size() >= 7 &&
               pkt[5] == sts::kRegTorqueEnable) {
      torque_writes.push_back({id, pkt[6]});
    } else if (inst == sts::kInstSyncRead) {
      const uint8_t len = pkt[6];
      for (size_t i = 7; i < pkt.size() - 1; ++i) {
        const uint8_t sid = pkt[i];
        if (silent.count(sid)) continue;
        std::vector<uint8_t> params(len, 0);
        uint8_t le[2];
        sts::encodeU16(static_cast<uint16_t>(positions[sid]), le);
        params[0] = le[0];
        params[1] = le[1];
        reply(statusPacket(sid, params));
      }
    }
  }
};

FakeTransport makeBus() {
  FakeTransport bus;
  for (int id = 1; id <= 6; ++id) bus.positions[id] = 2048;
  bus.positions[2] = 2048 + 512;  // shoulder at +45 degrees
  return bus;
}

// Extracts (id -> goal position) from the last SYNC_WRITE in the log.
std::map<int, int> lastSyncGoals(const std::vector<uint8_t>& written) {
  std::map<int, int> goals;
  for (const auto& pkt : splitPackets(written)) {
    if (pkt.size() < 5 || pkt[4] != sts::kInstSyncWrite) continue;
    goals.clear();
    size_t i = 7;  // after FF FF FE LEN 83 ADDR LEN
    while (i + 7 < pkt.size()) {
      const uint8_t id = pkt[i];
      goals[id] = sts::decodeU16(&pkt[i + 2]);  // skip ACC byte
      i += 8;
    }
  }
  return goals;
}

void testConnectAndReadState() {
  FakeTransport bus = makeBus();
  arm::Sts3215ArmDriver driver(bus);
  driver.connect();
  CHECK(bus.torque_writes.empty());  // torque stays off after connect
  const arm::ArmState st = driver.readState();
  CHECK_NEAR(st.joints.at("shoulder"), 45.0);
  CHECK_NEAR(st.joints.at("base"), 0.0);
  CHECK_NEAR(st.gripper_open, 0.0);
}

void testMoveJointsAndTorque() {
  FakeTransport bus = makeBus();
  arm::Sts3215ArmDriver driver(bus);
  driver.connect();
  arm::JointAngles a;
  a.base = 30;
  a.shoulder = 45;
  a.elbow = -30;
  a.wrist = 10;
  driver.moveJoints(a);
  CHECK(bus.torque_writes.size() == 6);
  for (const auto& tw : bus.torque_writes) CHECK(tw.second == 1);
  const auto goals = lastSyncGoals(bus.written);
  CHECK(goals.at(1) == arm::angleToSteps(30));
  CHECK(goals.at(2) == arm::angleToSteps(45));
  CHECK(goals.at(3) == arm::angleToSteps(-30));
  CHECK(goals.at(4) == arm::angleToSteps(10));
  CHECK(goals.at(5) == 2048);            // wrist roll untouched
  CHECK(goals.at(6) == 2048);            // gripper seeded from position
  driver.moveJoints(a);
  CHECK(bus.torque_writes.size() == 6);  // no re-enable on later moves
  driver.setGripper(1.0);
  CHECK(lastSyncGoals(bus.written).at(6) == arm::angleToSteps(90));
  driver.setGripper(0.0);
  CHECK(lastSyncGoals(bus.written).at(6) == arm::angleToSteps(0));
  driver.disconnect();
  CHECK(bus.torque_writes.size() == 12);
  for (size_t i = 6; i < bus.torque_writes.size(); ++i)
    CHECK(bus.torque_writes[i].second == 0);
}

void testConnectFailure() {
  FakeTransport bus = makeBus();
  bus.silent.insert(3);
  arm::Sts3215ArmDriver driver(bus);
  bool threw = false;
  try {
    driver.connect();
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CHECK(threw);
}
}  // namespace

int main() {
  testConnectAndReadState();
  testMoveJointsAndTorque();
  testConnectFailure();
  std::printf("%d checks, %d failure(s)\n", g_checks, g_failures);
  if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
