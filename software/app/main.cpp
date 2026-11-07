// Color-sorting pick-and-place sequence.
//
// Connects detection (vision) -> coordinate conversion (calibration) ->
// IK (ik) -> driving (arm_driver). The hardware has not arrived yet, so
// the program runs end to end as a dry run without real hardware:
//
//   color_sort --synthetic        synthetic detections + stub driver
//   color_sort --image photo.jpg --calibration calib.txt
//                                 (only when built with OpenCV)
//
// For real operation, replace:
//   - Frame acquisition: use the ESP32-CAM stream.
//   - Driver: a real ArmDriver implementation (to be added when the
//     hardware arrives).
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "app/arm_driver.h"
#include "core/calibration.h"
#include "core/ik.h"
#include "vision/detection.h"

#ifdef COLOR_SORT_WITH_OPENCV
#include "vision/color_detector.h"
#endif

namespace {

// Sequence parameters (provisional; tune on the real hardware/layout).
constexpr double kGraspZ = 20.0;        // gripper tip height when grasping [mm]
constexpr double kApproachLift = 60.0;  // extra height above a target [mm]
constexpr double kPlaceZ = 45.0;        // tip height when placing [mm]
constexpr double kToolDown = -90.0;     // tool pitch for pick/place [degrees]

// Placement spots per color, work-plane coordinates [mm].
const std::map<std::string, std::array<double, 3>>& placePoses() {
  static const std::map<std::string, std::array<double, 3>> kPoses = {
      {"red", {140.0, -90.0, kPlaceZ}},
      {"blue", {140.0, 90.0, kPlaceZ}},
      {"green", {110.0, -110.0, kPlaceZ}},
      {"yellow", {110.0, 110.0, kPlaceZ}},
  };
  return kPoses;
}

// Move the tip to a position (tool pointing straight down).
// Throws arm::UnreachableError if the target cannot be reached.
void moveTo(arm::ArmDriver& driver, double x, double y, double z) {
  const arm::JointAngles angles = arm::solveIk(x, y, z, kToolDown);
  driver.moveJoints(angles);
}

// Pick one object and carry it to the placement spot for its color.
// Returns true on success.
bool pickAndPlace(arm::ArmDriver& driver, const std::string& label,
                  const std::pair<double, double>& world_xy) {
  const auto it = placePoses().find(label);
  if (it == placePoses().end()) {
    std::cerr << "warning: no placement spot defined for color: " << label
              << "\n";
    return false;
  }
  const double x = world_xy.first;
  const double y = world_xy.second;
  const auto& place = it->second;
  try {
    // Pick: above -> descend -> grasp -> lift.
    moveTo(driver, x, y, kGraspZ + kApproachLift);
    moveTo(driver, x, y, kGraspZ);
    driver.setGripper(0.0);
    moveTo(driver, x, y, kGraspZ + kApproachLift);
    // Place: above the spot -> descend -> release -> retreat.
    moveTo(driver, place[0], place[1], place[2] + kApproachLift);
    moveTo(driver, place[0], place[1], place[2]);
    driver.setGripper(1.0);
    moveTo(driver, place[0], place[1], place[2] + kApproachLift);
  } catch (const arm::UnreachableError& e) {
    std::cerr << "warning: unreachable, skipping (" << label << " @ " << x
              << ", " << y << "): " << e.what() << "\n";
    return false;
  }
  std::cout << "done: " << label << " from (" << x << ", " << y
            << ") to its placement spot\n";
  return true;
}

// Run the sorting for one set of detections; returns the number of
// objects processed.
int runSorting(const std::vector<arm::Detection>& detections,
               const arm::PlaneCalibration& calib, arm::ArmDriver& driver) {
  std::cout << detections.size() << " object(s) detected\n";
  int done = 0;
  for (const arm::Detection& det : detections) {
    const std::pair<double, double> world_xy =
        calib.pixelToWorld(det.centroid_px.x, det.centroid_px.y);
    std::cout << "detected: " << det.label << " @ px(" << det.centroid_px.x
              << ", " << det.centroid_px.y << ") -> mm(" << world_xy.first
              << ", " << world_xy.second << ")\n";
    if (pickAndPlace(driver, det.label, world_xy)) ++done;
  }
  return done;
}

// --- Synthetic scene for dry runs -------------------------------------
// The camera is assumed to view the work area x: 80..280, y: -100..100
// [mm]; the 4 correspondences below define that geometry.
const std::vector<std::pair<double, double>> kSynthWorldCorners = {
    {80.0, -100.0}, {280.0, -100.0}, {280.0, 100.0}, {80.0, 100.0}};
const std::vector<std::pair<double, double>> kSynthPixelCorners = {
    {40.0, 440.0}, {600.0, 440.0}, {600.0, 40.0}, {40.0, 40.0}};
// Objects in the synthetic scene: (color label, work-plane position [mm]).
const std::vector<std::pair<std::string, std::pair<double, double>>>
    kSynthObjects = {
        {"red", {170.0, -40.0}},
        {"blue", {185.0, 25.0}},
        {"green", {150.0, 55.0}},
        {"yellow", {205.0, -60.0}},
};

// Build the synthetic calibration and detection list directly, without
// rendering or processing any image (so this path needs no OpenCV).
std::pair<std::vector<arm::Detection>, arm::PlaneCalibration>
buildSyntheticScene() {
  arm::PlaneCalibration calib =
      arm::PlaneCalibration::compute(kSynthPixelCorners, kSynthWorldCorners);
  std::vector<arm::Detection> detections;
  for (const auto& obj : kSynthObjects) {
    const std::pair<double, double> px =
        calib.worldToPixel(obj.second.first, obj.second.second);
    arm::Detection det;
    det.label = obj.first;
    det.centroid_px = {px.first, px.second};
    det.area_px = 1500.0;  // nominal value; unused in the dry run
    det.bbox = {static_cast<int>(px.first) - 22,
                static_cast<int>(px.second) - 22, 44, 44};
    detections.push_back(det);
  }
  return {detections, calib};
}

void printUsage(const char* argv0) {
  std::cerr << "usage: " << argv0 << " --synthetic\n"
            << "       " << argv0
            << " --image <file> --calibration <file>\n";
}

}  // namespace

int main(int argc, char** argv) {
  bool synthetic = false;
  std::string image_path;
  std::string calibration_path;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--synthetic") == 0) {
      synthetic = true;
    } else if (std::strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
      image_path = argv[++i];
    } else if (std::strcmp(argv[i], "--calibration") == 0 && i + 1 < argc) {
      calibration_path = argv[++i];
    } else {
      printUsage(argv[0]);
      return 2;
    }
  }
  // Exactly one of --synthetic / --image is required.
  const bool valid = (synthetic && image_path.empty()) ||
                     (!synthetic && !image_path.empty());
  if (!valid) {
    printUsage(argv[0]);
    return 2;
  }

  std::vector<arm::Detection> detections;
  arm::PlaneCalibration calib;
  try {
    if (synthetic) {
      auto scene = buildSyntheticScene();
      detections = scene.first;
      calib = scene.second;
    } else {
      if (calibration_path.empty()) {
        std::cerr << "--image requires --calibration\n";
        return 2;
      }
#ifdef COLOR_SORT_WITH_OPENCV
      const cv::Mat frame = arm::loadImage(image_path);
      detections = arm::detectColors(frame);
      calib = arm::PlaneCalibration::load(calibration_path);
#else
      std::cerr << "this build has no OpenCV support; --image is unavailable "
                   "(use --synthetic)\n";
      return 1;
#endif
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }

  // Stub until the hardware arrives; switch to the real driver here.
  arm::StubArmDriver driver;
  driver.connect();
  const int done = runSorting(detections, calib, driver);
  driver.disconnect();
  std::cout << "sorting complete: " << done << " object(s)\n";
  return 0;
}
