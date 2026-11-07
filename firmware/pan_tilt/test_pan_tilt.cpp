// Host-side unit tests for the pan-tilt control logic. Assert-style
// binary, no external dependencies (same pattern as software/tests).
#include <cmath>
#include <cstdio>

#include "firmware/pan_tilt/pan_tilt.h"

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

void testClamp() {
  CHECK(pantilt::clampAngle(0.0, pantilt::kPanConfig) == 0.0);
  CHECK(pantilt::clampAngle(120.0, pantilt::kPanConfig) == 85.0);
  CHECK(pantilt::clampAngle(-120.0, pantilt::kPanConfig) == -85.0);
  CHECK(pantilt::clampAngle(-90.0, pantilt::kTiltConfig) == -75.0);
  CHECK(pantilt::clampAngle(90.0, pantilt::kTiltConfig) == 35.0);
}

void testPulseMapping() {
  CHECK(pantilt::angleToPulseUs(0.0, pantilt::kPanConfig) == 1500);
  CHECK(pantilt::angleToPulseUs(85.0, pantilt::kPanConfig) == 2350);
  CHECK(pantilt::angleToPulseUs(-85.0, pantilt::kPanConfig) == 650);
  // Beyond-limit angles clamp to the limit pulse, not beyond it.
  CHECK(pantilt::angleToPulseUs(180.0, pantilt::kPanConfig) == 2350);
  CHECK(pantilt::angleToPulseUs(-75.0, pantilt::kTiltConfig) == 750);
}

void testCounts() {
  CHECK(pantilt::pulseUsToCounts(1500) == 307);
  CHECK(pantilt::pulseUsToCounts(1000) == 205);
  CHECK(pantilt::pulseUsToCounts(2000) == 410);
  CHECK(pantilt::pulseUsToCounts(0) == 0);
}

void testSweeper() {
  pantilt::Sweeper sweep(pantilt::kPanConfig, 85.0, -85.0);
  CHECK(sweep.update(1.0) == 0.0);          // -85 + 85/s * 1 s
  CHECK(sweep.update(1.0) == 85.0);         // reaches max
  CHECK(sweep.update(0.5) == 42.5);         // reversed at max
  CHECK(sweep.update(2.0) == -85.0);        // clamps at min, no overshoot
  CHECK(sweep.update(0.0) == -85.0);        // zero dt is a no-op
  pantilt::Sweeper still(pantilt::kTiltConfig, 0.0, -45.0);
  CHECK(still.update(1.0) == -45.0);        // zero speed holds position
}
}  // namespace

int main() {
  testClamp();
  testPulseMapping();
  testCounts();
  testSweeper();
  std::printf("%d checks, %d failure(s)\n", g_checks, g_failures);
  if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
