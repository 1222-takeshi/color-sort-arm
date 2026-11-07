#pragma once
// Pan-tilt control logic for the ESP32-CAM mount: two 9 g servos on a
// PCA9685 (channels 0 = pan, 1 = tilt), driven by a Raspberry Pi Pico.
// This module is dependency-free so the logic is unit-tested on the host;
// the Pico firmware only adds the I2C transport.
//
// Angles are in degrees. 0 is the servo center (1500 us). The pulse
// mapping is linear: 10 us per degree, clamped to the safe pulse range
// for typical 9 g servos. Per-axis angle limits keep the camera holder
// from hitting the mount; tune them after the physical fit check.
namespace pantilt {

inline constexpr int kPanChannel = 0;
inline constexpr int kTiltChannel = 1;

struct AxisConfig {
  double min_angle_deg;
  double max_angle_deg;
};

// Pan: full swing. Tilt: positive looks up, negative looks down; the
// workspace is below the camera, so the useful range is mostly negative.
inline constexpr AxisConfig kPanConfig{-85.0, 85.0};
inline constexpr AxisConfig kTiltConfig{-75.0, 35.0};

inline constexpr double kCenterPulseUs = 1500.0;
inline constexpr double kUsPerDeg = 10.0;
inline constexpr double kMinPulseUs = 600.0;
inline constexpr double kMaxPulseUs = 2400.0;

// Clamp an angle to the axis limits.
double clampAngle(double angle_deg, const AxisConfig& axis);

// Convert an angle to a servo pulse width [us], clamped to the axis
// limits first and to the safe pulse range second.
int angleToPulseUs(double angle_deg, const AxisConfig& axis);

// Convert a pulse width [us] to PCA9685 off-counts (12-bit, 4096 counts
// per period) at the given PWM frequency.
int pulseUsToCounts(int pulse_us, double freq_hz = 50.0);

// Triangle-wave sweep between the axis limits at a constant speed.
// update() advances the sweep and returns the current angle.
class Sweeper {
 public:
  Sweeper(const AxisConfig& axis, double speed_deg_per_s, double start_deg);
  double update(double dt_s);
  double angle() const { return angle_; }

 private:
  double min_;
  double max_;
  double speed_;
  double angle_;
  int direction_ = 1;
};

}  // namespace pantilt
