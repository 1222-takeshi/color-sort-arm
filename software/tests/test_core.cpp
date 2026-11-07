// Unit tests for the dependency-free core library (ik + calibration).
// Single assert-style binary with no external test framework; exits with
// a non-zero status if any check fails.
//
// Coverage mirrors the reference test suite: IK/FK round trips, tool
// pitch handling, base-angle quadrants, unreachable targets (too far /
// too close), custom geometry parameters, homography corner exactness,
// both conversion directions, batch conversion, and save/load.
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <random>
#include <utility>
#include <vector>

#include "core/calibration.h"
#include "core/ik.h"

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

bool near(double a, double b, double tol) { return std::abs(a - b) <= tol; }

#define CHECK(cond) report(static_cast<bool>(cond), #cond, __LINE__)
#define CHECK_NEAR(a, b, tol) \
  report(near((a), (b), (tol)), #a " ~= " #b, __LINE__)
#define CHECK_THROWS_AS(expr, type)                              \
  do {                                                           \
    bool caught = false;                                         \
    try {                                                        \
      expr;                                                      \
    } catch (const type&) {                                      \
      caught = true;                                             \
    } catch (...) {                                              \
    }                                                            \
    report(caught, #expr " throws " #type, __LINE__);            \
  } while (0)

// ---------------------------------------------------------------- IK --
void testIkFkRoundtrip() {
  const double targets[][3] = {
      {170.0, -40.0, 20.0}, {185.0, 25.0, 20.0}, {150.0, 55.0, 60.0},
      {120.0, 0.0, 120.0},  {200.0, -80.0, 40.0}, {100.0, 100.0, 80.0},
  };
  for (const auto& t : targets) {
    for (const arm::ElbowMode mode :
         {arm::ElbowMode::kUp, arm::ElbowMode::kDown}) {
      const arm::JointAngles angles =
          arm::solveIk(t[0], t[1], t[2], -90.0, mode);
      const arm::Pose pose = arm::forwardKinematics(angles);
      CHECK_NEAR(pose.x, t[0], 1e-6);
      CHECK_NEAR(pose.y, t[1], 1e-6);
      CHECK_NEAR(pose.z, t[2], 1e-6);
      CHECK_NEAR(pose.tool_pitch, -90.0, 1e-6);
    }
  }
}

void testToolPitchRespected() {
  for (const double pitch : {-90.0, -60.0, -45.0, 0.0}) {
    const arm::JointAngles angles = arm::solveIk(160.0, 30.0, 90.0, pitch);
    const arm::Pose pose = arm::forwardKinematics(angles);
    CHECK_NEAR(pose.tool_pitch, pitch, 1e-6);
    CHECK_NEAR(pose.x, 160.0, 1e-6);
    CHECK_NEAR(pose.y, 30.0, 1e-6);
    CHECK_NEAR(pose.z, 90.0, 1e-6);
  }
}

void testFkIkRoundtripFromAngles() {
  arm::JointAngles original;
  original.base = 30.0;
  original.shoulder = 40.0;
  original.elbow = -70.0;
  original.wrist = -60.0;
  const arm::Pose pose = arm::forwardKinematics(original);
  const arm::JointAngles solved = arm::solveIk(
      pose.x, pose.y, pose.z, pose.tool_pitch, arm::ElbowMode::kUp);
  const arm::Pose back = arm::forwardKinematics(solved);
  CHECK_NEAR(back.x, pose.x, 1e-6);
  CHECK_NEAR(back.y, pose.y, 1e-6);
  CHECK_NEAR(back.z, pose.z, 1e-6);
}

void testBaseAngleQuadrants() {
  const double targets[][2] = {{-150.0, 50.0}, {-120.0, -80.0}, {0.0, 180.0}};
  for (const auto& t : targets) {
    const arm::JointAngles angles = arm::solveIk(t[0], t[1], 60.0);
    const double expected =
        std::atan2(t[1], t[0]) * 180.0 / 3.14159265358979323846;
    CHECK_NEAR(angles.base, expected, 1e-9);
  }
}

void testUnreachableTooFar() {
  CHECK_THROWS_AS(arm::solveIk(1000.0, 0.0, 0.0), arm::UnreachableError);
  CHECK(!arm::isReachable(1000.0, 0.0, 0.0));
}

void testUnreachableTooClose() {
  // With |L1 - L2| = 90 mm there is an inner unreachable region; with
  // tool_pitch = -90 the wrist center sits 90 mm above the tip, so the
  // tip (30, 0, 5) puts the wrist center 30 mm from the shoulder.
  arm::ArmParams params;
  params.link1 = 150.0;
  params.link2 = 60.0;
  params.link3 = 90.0;
  params.shoulder_z = 95.0;
  params.shoulder_offset = 0.0;
  CHECK_THROWS_AS(arm::solveIk(30.0, 0.0, 5.0, -90.0, arm::ElbowMode::kUp,
                               params),
                  arm::UnreachableError);
  CHECK(!arm::isReachable(30.0, 0.0, 5.0, -90.0, params));
}

void testReachableTrue() { CHECK(arm::isReachable(170.0, -40.0, 20.0)); }

void testCustomParamsChangeReach() {
  arm::ArmParams small;
  small.link1 = 50.0;
  small.link2 = 50.0;
  small.link3 = 30.0;
  small.shoulder_z = 40.0;
  small.shoulder_offset = 0.0;
  CHECK(!arm::isReachable(170.0, 0.0, 20.0, -90.0, small));
  const arm::JointAngles angles =
      arm::solveIk(70.0, 0.0, 20.0, -90.0, arm::ElbowMode::kUp, small);
  const arm::Pose pose = arm::forwardKinematics(angles, small);
  CHECK_NEAR(pose.x, 70.0, 1e-6);
  CHECK_NEAR(pose.y, 0.0, 1e-6);
  CHECK_NEAR(pose.z, 20.0, 1e-6);
}

// ------------------------------------------------------- calibration --
const std::vector<std::pair<double, double>> kWorldCorners = {
    {80.0, -100.0}, {280.0, -100.0}, {280.0, 100.0}, {80.0, 100.0}};
// Trapezoid-distorted pixel positions, to look perspective-like.
const std::vector<std::pair<double, double>> kPixelCorners = {
    {60.0, 430.0}, {590.0, 450.0}, {560.0, 60.0}, {90.0, 40.0}};

void testCornersMapExactly() {
  const arm::PlaneCalibration calib =
      arm::PlaneCalibration::compute(kPixelCorners, kWorldCorners);
  for (std::size_t i = 0; i < 4; ++i) {
    const auto w = calib.pixelToWorld(kPixelCorners[i].first,
                                      kPixelCorners[i].second);
    CHECK_NEAR(w.first, kWorldCorners[i].first, 1e-4);
    CHECK_NEAR(w.second, kWorldCorners[i].second, 1e-4);
    const auto p = calib.worldToPixel(kWorldCorners[i].first,
                                      kWorldCorners[i].second);
    CHECK_NEAR(p.first, kPixelCorners[i].first, 1e-4);
    CHECK_NEAR(p.second, kPixelCorners[i].second, 1e-4);
  }
}

void testWorldPixelRoundtrip() {
  const arm::PlaneCalibration calib =
      arm::PlaneCalibration::compute(kPixelCorners, kWorldCorners);
  std::mt19937 rng(42);
  std::uniform_real_distribution<double> dx(90.0, 270.0);
  std::uniform_real_distribution<double> dy(-90.0, 90.0);
  for (int i = 0; i < 50; ++i) {
    const double x = dx(rng);
    const double y = dy(rng);
    const auto px = calib.worldToPixel(x, y);
    const auto back = calib.pixelToWorld(px.first, px.second);
    CHECK_NEAR(back.first, x, 1e-6);
    CHECK_NEAR(back.second, y, 1e-6);
  }
}

void testPixelWorldRoundtrip() {
  const arm::PlaneCalibration calib =
      arm::PlaneCalibration::compute(kPixelCorners, kWorldCorners);
  std::mt19937 rng(7);
  std::uniform_real_distribution<double> du(100.0, 550.0);
  std::uniform_real_distribution<double> dv(80.0, 420.0);
  for (int i = 0; i < 50; ++i) {
    const double u = du(rng);
    const double v = dv(rng);
    const auto w = calib.pixelToWorld(u, v);
    const auto back = calib.worldToPixel(w.first, w.second);
    CHECK_NEAR(back.first, u, 1e-6);
    CHECK_NEAR(back.second, v, 1e-6);
  }
}

void testSaveLoadRoundtrip() {
  const arm::PlaneCalibration calib =
      arm::PlaneCalibration::compute(kPixelCorners, kWorldCorners);
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "color_sort_test_calib.txt";
  calib.save(path.string());
  const arm::PlaneCalibration loaded =
      arm::PlaneCalibration::load(path.string());
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      CHECK_NEAR(loaded.homography[r][c], calib.homography[r][c], 1e-9);
  const auto a = calib.pixelToWorld(320.0, 240.0);
  const auto b = loaded.pixelToWorld(320.0, 240.0);
  CHECK_NEAR(b.first, a.first, 1e-9);
  CHECK_NEAR(b.second, a.second, 1e-9);
  CHECK(loaded.pixel_points.size() == 4);
  CHECK(loaded.world_points.size() == 4);
  std::filesystem::remove(path);
}

void testRequiresFourPoints() {
  const std::vector<std::pair<double, double>> three_px(kPixelCorners.begin(),
                                                        kPixelCorners.begin() + 3);
  const std::vector<std::pair<double, double>> three_w(kWorldCorners.begin(),
                                                       kWorldCorners.begin() + 3);
  CHECK_THROWS_AS(arm::PlaneCalibration::compute(three_px, three_w),
                  std::invalid_argument);
}

void testBatchConversion() {
  const arm::PlaneCalibration calib =
      arm::PlaneCalibration::compute(kPixelCorners, kWorldCorners);
  const std::vector<std::pair<double, double>> pts = {
      {100.0, 100.0}, {320.0, 240.0}, {500.0, 400.0}};
  const auto out = calib.pixelsToWorld(pts);
  CHECK(out.size() == 3);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    const auto single = calib.pixelToWorld(pts[i].first, pts[i].second);
    CHECK(out[i] == single);
  }
}

}  // namespace

int main() {
  testIkFkRoundtrip();
  testToolPitchRespected();
  testFkIkRoundtripFromAngles();
  testBaseAngleQuadrants();
  testUnreachableTooFar();
  testUnreachableTooClose();
  testReachableTrue();
  testCustomParamsChangeReach();

  testCornersMapExactly();
  testWorldPixelRoundtrip();
  testPixelWorldRoundtrip();
  testSaveLoadRoundtrip();
  testRequiresFourPoints();
  testBatchConversion();

  std::printf("%d checks, %d failure(s)\n", g_checks, g_failures);
  if (g_failures == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  return 1;
}
