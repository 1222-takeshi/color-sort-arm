// Inverse kinematics for a planar 2-link arm plus a wrist pitch joint.
//
// Solves base rotation + shoulder/elbow/wrist pitch analytically.
// The wrist roll axis is not determined by the planar IK and is left to
// the caller.
//
// Angle conventions (all in degrees):
//   base:     rotation about the vertical axis, counter-clockwise from
//             the x axis in the work-plane.
//   shoulder: absolute angle of link 1 (upper arm) against the horizontal
//             plane; positive is up.
//   elbow:    angle of link 2 (forearm) relative to link 1.
//   wrist:    angle of the tool link relative to link 2.
//             Absolute tool pitch = shoulder + elbow + wrist.
#pragma once

#include <stdexcept>

namespace arm {

// Thrown when a target pose cannot be reached.
class UnreachableError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Link geometry [mm].
//
// NOTE: the default values below are provisional estimates for an
// SO-101-class arm; they are not official nominal dimensions. Measure
// the assembled hardware and replace them before real operation.
struct ArmParams {
  double link1 = 115.0;           // shoulder -> elbow [mm]
  double link2 = 115.0;           // elbow -> wrist [mm]
  double link3 = 90.0;            // wrist -> gripper tip [mm]
  double shoulder_z = 95.0;       // work plane -> shoulder axis height [mm]
  double shoulder_offset = 0.0;   // base axis -> shoulder axis offset [mm]
};

inline constexpr ArmParams kDefaultParams{};

// Joint angles [degrees]. Output of the IK, input of the FK.
struct JointAngles {
  double base = 0.0;
  double shoulder = 0.0;
  double elbow = 0.0;
  double wrist = 0.0;
  double wrist_roll = 0.0;  // not solved by the planar IK (caller-specified)
};

// Result of the forward kinematics: gripper tip position [mm] and the
// absolute tool pitch [degrees].
struct Pose {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double tool_pitch = 0.0;
};

enum class ElbowMode { kUp, kDown };

// Solve the joint angles for a gripper tip target position and tool pitch.
//   x, y, z:    target position [mm]; origin is the base axis on the work
//               plane.
//   tool_pitch: absolute tool angle [degrees]; -90 points straight down.
// Throws UnreachableError if the target cannot be reached.
JointAngles solveIk(double x, double y, double z, double tool_pitch = -90.0,
                    ElbowMode elbow_mode = ElbowMode::kUp,
                    const ArmParams& params = kDefaultParams);

// Forward kinematics: joint angles -> gripper tip position and tool pitch.
Pose forwardKinematics(const JointAngles& angles,
                       const ArmParams& params = kDefaultParams);

// Reachability check that does not throw.
bool isReachable(double x, double y, double z, double tool_pitch = -90.0,
                 const ArmParams& params = kDefaultParams);

}  // namespace arm
