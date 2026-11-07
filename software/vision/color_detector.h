// Color detection on BGR frames using OpenCV.
//
// This is the only module that depends on OpenCV. The input is an
// already-acquired frame (from a file or a stream); all I/O stays with
// the caller except for the small loadImage helper.
#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "vision/detection.h"

namespace arm {

// Detect blobs of the target colors in a BGR frame (CV_8UC3).
//   colors:      color definitions to look for.
//   min_area_px: minimum blob area, for noise rejection.
// Returns detections sorted by area, largest first.
// Throws std::invalid_argument if the frame is empty.
std::vector<Detection> detectColors(
    const cv::Mat& frame_bgr,
    const std::vector<ColorRange>& colors = defaultColors(),
    double min_area_px = 300.0);

// Load an image file as a BGR frame. Throws std::runtime_error if the
// file cannot be read.
cv::Mat loadImage(const std::string& path);

// Convenience wrapper: detect directly from an image file.
std::vector<Detection> detectInImage(
    const std::string& path,
    const std::vector<ColorRange>& colors = defaultColors(),
    double min_area_px = 300.0);

}  // namespace arm
