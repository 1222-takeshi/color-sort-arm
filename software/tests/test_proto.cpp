// Tests for the STS3215 packet protocol (proto/sts3215). Golden
// vectors are hand-computed from the packet definition and cross-
// checked against the org-side implementation (see
// docs/pico_bridge_design.md). Assert-style, no dependencies.
#include <cstdio>
#include <vector>

#include "proto/sts3215.h"

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

void testGoldenVectors() {
  // PING id=1: FF FF 01 02 01, sum=0x04 -> CHK 0xFB.
  CHECK(sts::pingPacket(1) ==
        (std::vector<uint8_t>{0xFF, 0xFF, 0x01, 0x02, 0x01, 0xFB}));
  // WRITE goal position 2048 (LE 00 08) at reg 42, id=1:
  // sum = 01+05+03+2A+00+08 = 0x3B -> CHK 0xC4.
  uint8_t le[2];
  sts::encodeU16(2048, le);
  CHECK(sts::writePacket(1, sts::kRegGoalPosition, {le[0], le[1]}) ==
        (std::vector<uint8_t>{0xFF, 0xFF, 0x01, 0x05, 0x03, 0x2A, 0x00,
                              0x08, 0xC4}));
}

void testParseStatus() {
  // Status: id=1, err=0, position 2048 -> FF FF 01 04 00 00 08 F2.
  const auto st = sts::parseStatus(
      std::vector<uint8_t>{0xFF, 0xFF, 0x01, 0x04, 0x00, 0x00, 0x08, 0xF2});
  CHECK(st.has_value());
  CHECK(st->id == 1 && st->error == 0);
  CHECK(st->params.size() == 2 && sts::decodeU16(st->params.data()) == 2048);
  // Corrupted checksum and bad header are rejected.
  CHECK(!sts::parseStatus(
      std::vector<uint8_t>{0xFF, 0xFF, 0x01, 0x04, 0x00, 0x00, 0x08, 0x00}));
  CHECK(!sts::parseStatus(
      std::vector<uint8_t>{0xFF, 0x00, 0x01, 0x04, 0x00, 0x00, 0x08, 0xF2}));
  // Truncated packet is rejected.
  CHECK(!sts::parseStatus(std::vector<uint8_t>{0xFF, 0xFF, 0x01, 0x04}));
}

void testSignedMagnitude() {
  CHECK(sts::decodeSignedMagnitude(0x0001) == 1);
  CHECK(sts::decodeSignedMagnitude(0x8001) == -1);
  CHECK(sts::decodeSignedMagnitude(0x83FF) == -1023);
  CHECK(sts::decodeSignedMagnitude(0x0401, 10) == -1);
  CHECK(sts::decodeSignedMagnitude(0x03FF, 10) == 1023);
}
void testSyncPackets() {
  // SYNC_WRITE from ACC (41), 7 bytes for servo 1:
  // ACC=10, goal pos 2048 (LE 00 08), time 0, speed 0.
  // sum = FE+0C+83+29+07+01+0A+00+08 = 0xD0 -> CHK 0x2F.
  const auto sw = sts::syncWritePacket(
      sts::kRegAcc, 7, {{1, {10, 0x00, 0x08, 0, 0, 0, 0}}});
  CHECK(sw == (std::vector<uint8_t>{0xFF, 0xFF, 0xFE, 0x0C, 0x83, 0x29,
                                    0x07, 0x01, 0x0A, 0x00, 0x08, 0x00,
                                    0x00, 0x00, 0x00, 0x2F}));
  // SYNC_READ 6 bytes from PRESENT_POSITION (56) for servos 1 and 2:
  // sum = FE+06+82+38+06+01+02 = 0xC7 -> CHK 0x38.
  const auto sr = sts::syncReadPacket(sts::kRegPresentPosition, 6, {1, 2});
  CHECK(sr == (std::vector<uint8_t>{0xFF, 0xFF, 0xFE, 0x06, 0x82, 0x38,
                                    0x06, 0x01, 0x02, 0x38}));
}

void testStreamParser() {
  const std::vector<uint8_t> good = {0xFF, 0xFF, 0x01, 0x04,
                                     0x00, 0x00, 0x08, 0xF2};  // id 1
  const std::vector<uint8_t> good2 = {0xFF, 0xFF, 0x02, 0x04,
                                      0x00, 0x00, 0x08, 0xF1};  // id 2
  {  // garbage before the header is dropped
    sts::StreamParser p;
    std::vector<uint8_t> stream = {0x00, 0x13, 0xFF};
    stream.insert(stream.end(), good.begin(), good.end());
    const auto out = p.feed(stream);
    CHECK(out.size() == 1 && out[0].id == 1);
  }
  {  // a corrupted packet does not block the next one
    sts::StreamParser p;
    std::vector<uint8_t> stream = {0xFF, 0xFF, 0x01, 0x04,
                                   0x00, 0x00, 0x08, 0x00};  // bad CHK
    stream.insert(stream.end(), good.begin(), good.end());
    const auto out = p.feed(stream);
    CHECK(out.size() == 1 && out[0].id == 1);
  }
  {  // split feeds complete only when the last byte arrives
    sts::StreamParser p;
    CHECK(p.feed(good.data(), 3).empty());
    const auto out = p.feed(good.data() + 3, good.size() - 3);
    CHECK(out.size() == 1 && out[0].id == 1);
  }
  {  // two packets in one feed
    sts::StreamParser p;
    std::vector<uint8_t> stream = good;
    stream.insert(stream.end(), good2.begin(), good2.end());
    const auto out = p.feed(stream);
    CHECK(out.size() == 2 && out[0].id == 1 && out[1].id == 2);
  }
}

}  // namespace

int main() {
  testGoldenVectors();
  testParseStatus();
  testSignedMagnitude();
  testSyncPackets();
  testStreamParser();
  std::printf("%d checks, %d failure(s)\n", g_checks, g_failures);
  if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
