#include "core/ik.h"

#include <algorithm>
#include <cmath>

namespace arm {
namespace {

constexpr double kPi = 3.14159265358979323846;

double degToRad(double deg) { return deg * kPi / 180.0; }
double radToDeg(double rad) { return rad * 180.0 / kPi; }

}  // namespace

JointAngles solveIk(double x, double y, double z, double tool_pitch,
                    ElbowMode elbow_mode, const ArmParams& params) {
  const double base = radToDeg(std::atan2(y, x));
  const double r_tip = std::hypot(x, y);

  // Place the wrist center in planar coordinates (a, b) with the shoulder
  // axis at the origin.
  const double phi = degToRad(tool_pitch);
  const double wrist_r = r_tip - params.link3 * std::cos(phi);
  const double wrist_z = z - params.link3 * std::sin(phi);
  const double a = wrist_r - params.shoulder_offset;
  const double b = wrist_z - params.shoulder_z;

  const double dist = std::hypot(a, b);
  const double l1 = params.link1;
  const double l2 = params.link2;
  if (dist > l1 + l2) {
    throw UnreachableError("target too far: shoulder->wrist distance exceeds "
                           "the sum of the link lengths");
  }
  if (dist < std::abs(l1 - l2)) {
    throw UnreachableError("target too close: shoulder->wrist distance is "
                           "below the difference of the link lengths");
  }
  if (dist == 0.0) {
    throw UnreachableError("wrist center coincides with the shoulder axis "
                           "(singularity)");
  }

  const double alpha = std::atan2(b, a);
  const double cos_beta =
      (l1 * l1 + dist * dist - l2 * l2) / (2.0 * l1 * dist);
  const double beta = std::acos(std::max(-1.0, std::min(1.0, cos_beta)));

  const double theta1 =
      (elbow_mode == ElbowMode::kUp) ? alpha + beta : alpha - beta;

  // Absolute angle of link 2 -> relative elbow angle.
  const double elbow_pos_r = l1 * std::cos(theta1);
  const double elbow_pos_z = l1 * std::sin(theta1);
  const double theta2_abs = std::atan2(b - elbow_pos_z, a - elbow_pos_r);
  const double theta2 = theta2_abs - theta1;

  // Wrist: pick the relative angle so the absolute tool angle matches
  // the requested tool pitch.
  const double theta3 = phi - theta1 - theta2;

  JointAngles result;
  result.base = base;
  result.shoulder = radToDeg(theta1);
  result.elbow = radToDeg(theta2);
  result.wrist = radToDeg(theta3);
  return result;
}

Pose forwardKinematics(const JointAngles& angles, const ArmParams& params) {
  const double base = degToRad(angles.base);
  const double t1 = degToRad(angles.shoulder);
  const double t2 = t1 + degToRad(angles.elbow);
  const double t3 = t2 + degToRad(angles.wrist);

  const double r = params.shoulder_offset + params.link1 * std::cos(t1) +
                   params.link2 * std::cos(t2) + params.link3 * std::cos(t3);
  const double z = params.shoulder_z + params.link1 * std::sin(t1) +
                   params.link2 * std::sin(t2) + params.link3 * std::sin(t3);
  Pose pose;
  pose.x = r * std::cos(base);
  pose.y = r * std::sin(base);
  pose.z = z;
  pose.tool_pitch = radToDeg(t3);
  return pose;
}

bool isReachable(double x, double y, double z, double tool_pitch,
                 const ArmParams& params) {
  try {
    solveIk(x, y, z, tool_pitch, ElbowMode::kUp, params);
    return true;
  } catch (const UnreachableError&) {
    return false;
  }
}

}  // namespace arm
