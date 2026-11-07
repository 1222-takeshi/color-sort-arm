// Shared vision data types. This header intentionally has no OpenCV
// dependency so that non-vision code (e.g. the dry-run path in the app)
// can work with detections without linking OpenCV.
#pragma once

#include <array>
#include <string>
#include <vector>

namespace arm {

struct Vec2 {
  double x = 0.0;
  double y = 0.0;
};

struct BoundingBox {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

// One detected blob.
struct Detection {
  std::string label;     // color label (ColorRange::name)
  Vec2 centroid_px;      // centroid (u, v) [px]
  double area_px = 0.0;  // blob area [px^2]
  BoundingBox bbox;      // bounding rectangle (x, y, w, h) [px]
};

// One HSV interval. Colors whose hue wraps around 0/179 (such as red)
// use two intervals.
// Values follow the OpenCV HSV convention (H: 0-179, S: 0-255, V: 0-255).
struct HsvRange {
  std::array<int, 3> lower;
  std::array<int, 3> upper;
};

struct ColorRange {
  std::string name;
  std::vector<HsvRange> ranges;
};

// Default target colors; tune to the actual lighting conditions.
inline const std::vector<ColorRange>& defaultColors() {
  static const std::vector<ColorRange> kColors = {
      {"red",
       {{{0, 90, 70}, {10, 255, 255}}, {{170, 90, 70}, {179, 255, 255}}}},
      {"green", {{{40, 70, 60}, {85, 255, 255}}}},
      {"blue", {{{95, 90, 60}, {130, 255, 255}}}},
      {"yellow", {{{20, 90, 80}, {35, 255, 255}}}},
  };
  return kColors;
}

}  // namespace arm
