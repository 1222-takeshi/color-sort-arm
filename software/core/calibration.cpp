#include "core/calibration.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace arm {
namespace {

// Solve an 8x8 linear system A * x = b by Gaussian elimination with
// partial pivoting. Throws std::runtime_error if the system is singular.
std::array<double, 8> solveLinear8(std::array<std::array<double, 8>, 8> a,
                                   std::array<double, 8> b) {
  for (int col = 0; col < 8; ++col) {
    int pivot = col;
    for (int row = col + 1; row < 8; ++row) {
      if (std::abs(a[row][col]) > std::abs(a[pivot][col])) pivot = row;
    }
    if (std::abs(a[pivot][col]) < 1e-12) {
      throw std::runtime_error("singular system while computing homography");
    }
    std::swap(a[pivot], a[col]);
    std::swap(b[pivot], b[col]);
    for (int row = col + 1; row < 8; ++row) {
      const double factor = a[row][col] / a[col][col];
      if (factor == 0.0) continue;
      for (int k = col; k < 8; ++k) a[row][k] -= factor * a[col][k];
      b[row] -= factor * b[col];
    }
  }
  std::array<double, 8> x{};
  for (int row = 7; row >= 0; --row) {
    double sum = b[row];
    for (int k = row + 1; k < 8; ++k) sum -= a[row][k] * x[k];
    x[row] = sum / a[row][row];
  }
  return x;
}

// Invert a 3x3 matrix via the adjugate. Throws std::runtime_error if the
// matrix is singular.
std::array<std::array<double, 3>, 3> invert3x3(
    const std::array<std::array<double, 3>, 3>& m) {
  const double a = m[0][0], b = m[0][1], c = m[0][2];
  const double d = m[1][0], e = m[1][1], f = m[1][2];
  const double g = m[2][0], h = m[2][1], i = m[2][2];
  const double det = a * (e * i - f * h) - b * (d * i - f * g) +
                     c * (d * h - e * g);
  if (std::abs(det) < 1e-15) {
    throw std::runtime_error("homography matrix is singular");
  }
  const double inv_det = 1.0 / det;
  std::array<std::array<double, 3>, 3> inv;
  inv[0] = {(e * i - f * h) * inv_det, (c * h - b * i) * inv_det,
            (b * f - c * e) * inv_det};
  inv[1] = {(f * g - d * i) * inv_det, (a * i - c * g) * inv_det,
            (c * d - a * f) * inv_det};
  inv[2] = {(d * h - e * g) * inv_det, (b * g - a * h) * inv_det,
            (a * e - b * d) * inv_det};
  return inv;
}

std::pair<double, double> applyHomography(
    const std::array<std::array<double, 3>, 3>& h, double px, double py) {
  const double w = h[2][0] * px + h[2][1] * py + h[2][2];
  return {(h[0][0] * px + h[0][1] * py + h[0][2]) / w,
          (h[1][0] * px + h[1][1] * py + h[1][2]) / w};
}

}  // namespace

PlaneCalibration PlaneCalibration::compute(
    const std::vector<std::pair<double, double>>& pixel_points,
    const std::vector<std::pair<double, double>>& world_points) {
  if (pixel_points.size() != 4 || world_points.size() != 4) {
    throw std::invalid_argument(
        "homography computation requires exactly 4 point correspondences");
  }

  // Direct linear transform with h[2][2] fixed to 1. For each
  // correspondence (u, v) -> (x, y):
  //   h11*u + h12*v + h13 - h31*u*x - h32*v*x = x
  //   h21*u + h22*v + h23 - h31*u*y - h32*v*y = y
  std::array<std::array<double, 8>, 8> a{};
  std::array<double, 8> b{};
  for (int i = 0; i < 4; ++i) {
    const double u = pixel_points[i].first;
    const double v = pixel_points[i].second;
    const double x = world_points[i].first;
    const double y = world_points[i].second;
    a[2 * i] = {u, v, 1.0, 0.0, 0.0, 0.0, -u * x, -v * x};
    b[2 * i] = x;
    a[2 * i + 1] = {0.0, 0.0, 0.0, u, v, 1.0, -u * y, -v * y};
    b[2 * i + 1] = y;
  }
  const std::array<double, 8> h = solveLinear8(a, b);

  PlaneCalibration calib;
  calib.homography = {{{h[0], h[1], h[2]},
                       {h[3], h[4], h[5]},
                       {h[6], h[7], 1.0}}};
  calib.pixel_points = pixel_points;
  calib.world_points = world_points;
  return calib;
}

std::pair<double, double> PlaneCalibration::pixelToWorld(double u,
                                                         double v) const {
  return applyHomography(homography, u, v);
}

std::pair<double, double> PlaneCalibration::worldToPixel(double x,
                                                         double y) const {
  return applyHomography(invert3x3(homography), x, y);
}

std::vector<std::pair<double, double>> PlaneCalibration::pixelsToWorld(
    const std::vector<std::pair<double, double>>& points) const {
  std::vector<std::pair<double, double>> out;
  out.reserve(points.size());
  for (const auto& p : points) out.push_back(pixelToWorld(p.first, p.second));
  return out;
}

// Plain-text calibration format:
//
//   # color-sort-arm plane calibration
//   homography
//   <h00> <h01> <h02> <h10> <h11> <h12> <h20> <h21> <h22>
//   pixel_points <n>
//   <u> <v>            (n lines)
//   world_points <n>
//   <x> <y>            (n lines)
void PlaneCalibration::save(const std::string& path) const {
  std::ofstream out(path);
  if (!out) throw std::runtime_error("cannot open for writing: " + path);
  out << "# color-sort-arm plane calibration\n";
  out << std::setprecision(17);
  out << "homography\n";
  for (int r = 0; r < 3; ++r) {
    out << homography[r][0] << ' ' << homography[r][1] << ' '
        << homography[r][2] << (r == 2 ? '\n' : ' ');
  }
  out << "pixel_points " << pixel_points.size() << "\n";
  for (const auto& p : pixel_points) out << p.first << ' ' << p.second << "\n";
  out << "world_points " << world_points.size() << "\n";
  for (const auto& p : world_points) out << p.first << ' ' << p.second << "\n";
  if (!out) throw std::runtime_error("failed while writing: " + path);
}

PlaneCalibration PlaneCalibration::load(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open calibration file: " + path);
  PlaneCalibration calib;
  std::string token;
  while (in >> token) {
    if (!token.empty() && token[0] == '#') {
      std::string rest;
      std::getline(in, rest);
    } else if (token == "homography") {
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) in >> calib.homography[r][c];
    } else if (token == "pixel_points" || token == "world_points") {
      std::size_t n = 0;
      in >> n;
      std::vector<std::pair<double, double>> points(n);
      for (std::size_t i = 0; i < n; ++i) in >> points[i].first >> points[i].second;
      if (token == "pixel_points")
        calib.pixel_points = points;
      else
        calib.world_points = points;
    } else {
      throw std::runtime_error("unexpected token in calibration file: " +
                               token);
    }
  }
  if (in.bad()) throw std::runtime_error("failed while reading: " + path);
  return calib;
}

}  // namespace arm
