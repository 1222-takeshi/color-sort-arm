#include "vision/color_detector.h"

#include <algorithm>
#include <stdexcept>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace arm {

std::vector<Detection> detectColors(const cv::Mat& frame_bgr,
                                    const std::vector<ColorRange>& colors,
                                    double min_area_px) {
  if (frame_bgr.empty()) {
    throw std::invalid_argument("empty frame passed to detectColors");
  }

  cv::Mat hsv;
  cv::cvtColor(frame_bgr, hsv, cv::COLOR_BGR2HSV);
  const cv::Mat kernel =
      cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
  std::vector<Detection> detections;

  for (const ColorRange& color : colors) {
    cv::Mat mask = cv::Mat::zeros(hsv.rows, hsv.cols, CV_8UC1);
    for (const HsvRange& range : color.ranges) {
      cv::Mat part;
      cv::inRange(hsv, cv::Scalar(range.lower[0], range.lower[1], range.lower[2]),
                  cv::Scalar(range.upper[0], range.upper[1], range.upper[2]),
                  part);
      cv::bitwise_or(mask, part, mask);
    }
    // Drop small noise and fill holes.
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);
    for (const auto& contour : contours) {
      const double area = cv::contourArea(contour);
      if (area < min_area_px) continue;
      const cv::Moments moments = cv::moments(contour);
      if (moments.m00 == 0.0) continue;
      const cv::Rect rect = cv::boundingRect(contour);
      Detection det;
      det.label = color.name;
      det.centroid_px = {moments.m10 / moments.m00, moments.m01 / moments.m00};
      det.area_px = area;
      det.bbox = {rect.x, rect.y, rect.width, rect.height};
      detections.push_back(det);
    }
  }

  std::sort(detections.begin(), detections.end(),
            [](const Detection& a, const Detection& b) {
              return a.area_px > b.area_px;
            });
  return detections;
}

cv::Mat loadImage(const std::string& path) {
  cv::Mat img = cv::imread(path);
  if (img.empty()) {
    throw std::runtime_error("could not read image: " + path);
  }
  return img;
}

std::vector<Detection> detectInImage(const std::string& path,
                                     const std::vector<ColorRange>& colors,
                                     double min_area_px) {
  return detectColors(loadImage(path), colors, min_area_px);
}

}  // namespace arm
