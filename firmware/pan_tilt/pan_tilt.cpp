#include "firmware/pan_tilt/pan_tilt.h"

#include <algorithm>
#include <cmath>

namespace pantilt {

double clampAngle(double angle_deg, const AxisConfig& axis) {
  return std::max(axis.min_angle_deg, std::min(axis.max_angle_deg, angle_deg));
}

int angleToPulseUs(double angle_deg, const AxisConfig& axis) {
  const double clamped = clampAngle(angle_deg, axis);
  const double pulse = kCenterPulseUs + clamped * kUsPerDeg;
  const double safe =
      std::max(kMinPulseUs, std::min(kMaxPulseUs, pulse));
  return static_cast<int>(std::lround(safe));
}

int pulseUsToCounts(int pulse_us, double freq_hz) {
  const double counts = pulse_us * freq_hz * 4096.0 / 1.0e6;
  return std::max(0, std::min(4095, static_cast<int>(std::lround(counts))));
}

Sweeper::Sweeper(const AxisConfig& axis, double speed_deg_per_s,
                 double start_deg)
    : min_(axis.min_angle_deg),
      max_(axis.max_angle_deg),
      speed_(std::abs(speed_deg_per_s)),
      angle_(clampAngle(start_deg, axis)) {}

double Sweeper::update(double dt_s) {
  if (dt_s <= 0.0 || speed_ == 0.0) return angle_;
  double next = angle_ + direction_ * speed_ * dt_s;
  if (next >= max_) {
    next = max_;
    direction_ = -1;
  } else if (next <= min_) {
    next = min_;
    direction_ = 1;
  }
  angle_ = next;
  return angle_;
}

}  // namespace pantilt
