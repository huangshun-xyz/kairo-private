#pragma once

#include <algorithm>

namespace kairo {

enum class ScrollBoundaryBehavior {
  kClamp,
  kBounce,
};

struct ScrollSpringConfig {
  double mass = 0.5;
  double stiffness = 100.0;
  double damping_ratio = 1.1;
};

struct ScrollInteractionOptions {
  ScrollBoundaryBehavior boundary_behavior = ScrollBoundaryBehavior::kBounce;
  bool always_scrollable = true;
  bool allow_inertia = true;
  double drag_sensitivity = 1.0;
  double overscroll_friction = 0.52;
  double deceleration_rate = 0.135;
  ScrollSpringConfig spring;
};

struct ScrollMetrics {
  double min_position = 0.0;
  double max_position = 0.0;
  double viewport_dimension = 0.0;

  bool HasScrollableExtent() const {
    return max_position > min_position;
  }
};

// Platform-agnostic horizontal scroll physics inspired by Flutter's
// BouncingScrollPhysics/BouncingScrollSimulation split.
class ScrollInteractionController final {
 public:
  ScrollInteractionController() = default;

  void SetOptions(const ScrollInteractionOptions& options);
  const ScrollInteractionOptions& options() const;

  void Stop();
  void BeginGesture(double position);
  double ApplyGestureDelta(double delta, const ScrollMetrics& metrics);
  bool EndGesture(double velocity, const ScrollMetrics& metrics);
  bool Advance(double delta_seconds, const ScrollMetrics& metrics);

  bool IsActive() const;
  double position() const;

 private:
  enum class Mode {
    kIdle,
    kBallistic,
    kSpring,
  };

  static constexpr double kMinFlingVelocity = 100.0;
  static constexpr double kMaxFlingVelocity = 8000.0;
  static constexpr double kMaxCarriedMomentum = 40000.0;
  static constexpr double kMaxSpringTransferVelocity = 5000.0;
  static constexpr double kPositionTolerance = 0.5;
  static constexpr double kVelocityTolerance = 5.0;
  static constexpr double kMaxStepSeconds = 1.0 / 120.0;

  bool CanOverscroll(const ScrollMetrics& metrics) const;
  bool IsOutOfRange(const ScrollMetrics& metrics) const;
  double ClampedPosition(const ScrollMetrics& metrics) const;
  double SpringTarget(const ScrollMetrics& metrics) const;
  double FrictionFactor(double overscroll_fraction) const;
  double CarriedMomentum(double existing_velocity) const;

  static double ApplyFriction(double extent_outside, double abs_delta, double gamma);

  void StartSpring(const ScrollMetrics& metrics);
  double TimeToBallisticPosition(double start_position,
                                 double start_velocity,
                                 double target_position) const;
  void AdvanceBallistic(double delta_seconds, const ScrollMetrics& metrics);
  void AdvanceSpring(double delta_seconds, const ScrollMetrics& metrics);

  ScrollInteractionOptions options_;
  Mode mode_ = Mode::kIdle;
  double position_ = 0.0;
  double velocity_ = 0.0;
  double spring_target_ = 0.0;
  double carried_velocity_ = 0.0;
};

}  // namespace kairo
