// Planar calibration.
//
// Computes a homography from 4 point correspondences (known points on the
// work plane and their pixel positions in the image) and converts between
// pixel coordinates [px] and work-plane coordinates [mm] in both
// directions. The homography and the source correspondences can be saved
// to and loaded from a plain-text file.
#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace arm {

class PlaneCalibration {
 public:
  // 3x3 homography, pixel -> world (mm), row-major.
  std::array<std::array<double, 3>, 3> homography{};
  std::vector<std::pair<double, double>> pixel_points;
  std::vector<std::pair<double, double>> world_points;

  // Compute the homography from exactly 4 correspondences.
  // pixel_points and world_points must be in matching order.
  // Throws std::invalid_argument unless both lists have exactly 4 points.
  static PlaneCalibration compute(
      const std::vector<std::pair<double, double>>& pixel_points,
      const std::vector<std::pair<double, double>>& world_points);

  // Convert a pixel coordinate (u, v) to a work-plane coordinate (x, y).
  std::pair<double, double> pixelToWorld(double u, double v) const;
  // Convert a work-plane coordinate (x, y) [mm] to a pixel coordinate.
  std::pair<double, double> worldToPixel(double x, double y) const;
  // Batch version of pixelToWorld.
  std::vector<std::pair<double, double>> pixelsToWorld(
      const std::vector<std::pair<double, double>>& points) const;

  // Save/load in the plain-text format described in calibration.cpp.
  void save(const std::string& path) const;
  static PlaneCalibration load(const std::string& path);
};

}  // namespace arm
