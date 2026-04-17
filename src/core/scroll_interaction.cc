#include "scroll_interaction.h"

#include <algorithm>
#include <cmath>

namespace kairo {

namespace {

double ClampPositive(double value, double fallback) {
  return value > 0.0 ? value : fallback;
}

}  // namespace

void ScrollInteractionController::SetOptions(const ScrollInteractionOptions& options) {
  options_ = options;
  options_.drag_sensitivity = std::max(options.drag_sensitivity, 0.0);
  options_.overscroll_friction = std::clamp(options.overscroll_friction, 0.01, 1.0);
  options_.deceleration_rate = std::clamp(options.deceleration_rate, 0.001, 0.999999);
  options_.spring.mass = ClampPositive(options.spring.mass, 0.5);
  options_.spring.stiffness = ClampPositive(options.spring.stiffness, 100.0);
  options_.spring.damping_ratio = ClampPositive(options.spring.damping_ratio, 1.1);
}

const ScrollInteractionOptions& ScrollInteractionController::options() const {
  return options_;
}

void ScrollInteractionController::Stop() {
  mode_ = Mode::kIdle;
  velocity_ = 0.0;
  carried_velocity_ = 0.0;
  spring_target_ = position_;
}

void ScrollInteractionController::BeginGesture(double position) {
  if (mode_ == Mode::kBallistic) {
    carried_velocity_ = velocity_;
  } else {
    carried_velocity_ = 0.0;
  }

  position_ = position;
  mode_ = Mode::kIdle;
  velocity_ = 0.0;
  spring_target_ = position_;
}

double ScrollInteractionController::ApplyGestureDelta(double delta, const ScrollMetrics& metrics) {
  const double adjusted_delta = delta * options_.drag_sensitivity;
  if (adjusted_delta == 0.0) {
    return position_;
  }

  if (options_.boundary_behavior == ScrollBoundaryBehavior::kClamp || !CanOverscroll(metrics)) {
    position_ = std::clamp(position_ + adjusted_delta, metrics.min_position, metrics.max_position);
    return position_;
  }

  if (!IsOutOfRange(metrics)) {
    position_ += adjusted_delta;
    return position_;
  }

  const double overscroll_past_start = std::max(metrics.min_position - position_, 0.0);
  const double overscroll_past_end = std::max(position_ - metrics.max_position, 0.0);
  const double overscroll_past = std::max(overscroll_past_start, overscroll_past_end);
  const bool easing = (overscroll_past_start > 0.0 && adjusted_delta > 0.0) ||
      (overscroll_past_end > 0.0 && adjusted_delta < 0.0);
  const double viewport_dimension = std::max(metrics.viewport_dimension, 1.0);
  const double overscroll_fraction =
      easing ? ((overscroll_past - std::abs(adjusted_delta)) / viewport_dimension)
             : (overscroll_past / viewport_dimension);
  const double friction = FrictionFactor(overscroll_fraction);
  const double applied_delta =
      std::copysign(ApplyFriction(overscroll_past, std::abs(adjusted_delta), friction), adjusted_delta);
  position_ += applied_delta;
  return position_;
}

bool ScrollInteractionController::EndGesture(double velocity, const ScrollMetrics& metrics) {
  const bool same_direction = (velocity > 0.0 && carried_velocity_ > 0.0) ||
      (velocity < 0.0 && carried_velocity_ < 0.0);
  if (same_direction) {
    velocity += CarriedMomentum(carried_velocity_);
  }
  carried_velocity_ = 0.0;
  velocity = std::clamp(velocity, -kMaxFlingVelocity, kMaxFlingVelocity);

  if (options_.boundary_behavior == ScrollBoundaryBehavior::kClamp || !CanOverscroll(metrics)) {
    position_ = ClampedPosition(metrics);
    if (!options_.allow_inertia || std::abs(velocity) < kMinFlingVelocity || !metrics.HasScrollableExtent()) {
      Stop();
      return false;
    }
    if ((position_ <= metrics.min_position && velocity < 0.0) ||
        (position_ >= metrics.max_position && velocity > 0.0)) {
      Stop();
      return false;
    }

    mode_ = Mode::kBallistic;
    velocity_ = velocity;
    return true;
  }

  if (IsOutOfRange(metrics)) {
    velocity_ = std::clamp(velocity, -kMaxSpringTransferVelocity, kMaxSpringTransferVelocity);
    StartSpring(metrics);
    return true;
  }

  if (!options_.allow_inertia || std::abs(velocity) < kMinFlingVelocity || !metrics.HasScrollableExtent()) {
    Stop();
    return false;
  }

  mode_ = Mode::kBallistic;
  velocity_ = velocity;
  return true;
}

bool ScrollInteractionController::Advance(double delta_seconds, const ScrollMetrics& metrics) {
  if (!IsActive() || delta_seconds <= 0.0) {
    return false;
  }

  double remaining = delta_seconds;
  while (remaining > 0.0 && IsActive()) {
    const double step = std::min(remaining, kMaxStepSeconds);
    remaining -= step;

    if (mode_ == Mode::kBallistic) {
      AdvanceBallistic(step, metrics);
    } else if (mode_ == Mode::kSpring) {
      AdvanceSpring(step, metrics);
    }
  }

  return IsActive();
}

bool ScrollInteractionController::IsActive() const {
  return mode_ != Mode::kIdle;
}

double ScrollInteractionController::position() const {
  return position_;
}

bool ScrollInteractionController::CanOverscroll(const ScrollMetrics& metrics) const {
  return options_.boundary_behavior == ScrollBoundaryBehavior::kBounce &&
      (options_.always_scrollable || metrics.HasScrollableExtent());
}

bool ScrollInteractionController::IsOutOfRange(const ScrollMetrics& metrics) const {
  return position_ < metrics.min_position || position_ > metrics.max_position;
}

double ScrollInteractionController::ClampedPosition(const ScrollMetrics& metrics) const {
  return std::clamp(position_, metrics.min_position, metrics.max_position);
}

double ScrollInteractionController::SpringTarget(const ScrollMetrics& metrics) const {
  if (position_ < metrics.min_position) {
    return metrics.min_position;
  }
  if (position_ > metrics.max_position) {
    return metrics.max_position;
  }
  return position_;
}

double ScrollInteractionController::FrictionFactor(double overscroll_fraction) const {
  return std::pow(1.0 - overscroll_fraction, 2.0) * options_.overscroll_friction;
}

double ScrollInteractionController::CarriedMomentum(double existing_velocity) const {
  return std::copysign(
      std::min(0.000816 * std::pow(std::abs(existing_velocity), 1.967), kMaxCarriedMomentum),
      existing_velocity);
}

double ScrollInteractionController::ApplyFriction(double extent_outside,
                                                  double abs_delta,
                                                  double gamma) {
  double total = 0.0;
  if (extent_outside > 0.0) {
    const double delta_to_limit = extent_outside / gamma;
    if (abs_delta < delta_to_limit) {
      return abs_delta * gamma;
    }
    total += extent_outside;
    abs_delta -= delta_to_limit;
  }
  return total + abs_delta;
}

void ScrollInteractionController::StartSpring(const ScrollMetrics& metrics) {
  spring_target_ = SpringTarget(metrics);
  mode_ = Mode::kSpring;
}

double ScrollInteractionController::TimeToBallisticPosition(double start_position,
                                                            double start_velocity,
                                                            double target_position) const {
  const double drag = std::log(options_.deceleration_rate);
  if (std::abs(drag) <= 1e-6 || std::abs(start_velocity) <= 1e-6) {
    return 0.0;
  }

  const double ratio = 1.0 + (((target_position - start_position) * drag) / start_velocity);
  if (ratio <= 0.0) {
    return 0.0;
  }

  return std::max(0.0, std::log(ratio) / drag);
}

void ScrollInteractionController::AdvanceBallistic(double delta_seconds, const ScrollMetrics& metrics) {
  const double start_position = position_;
  const double start_velocity = velocity_;
  const double decay = std::pow(options_.deceleration_rate, delta_seconds);
  const double drag = std::log(options_.deceleration_rate);
  const double delta = std::abs(drag) > 1e-6
      ? (velocity_ * (std::exp(drag * delta_seconds) - 1.0) / drag)
      : (velocity_ * delta_seconds);

  position_ += delta;
  velocity_ *= decay;

  if (options_.boundary_behavior == ScrollBoundaryBehavior::kClamp || !CanOverscroll(metrics)) {
    if (position_ < metrics.min_position) {
      position_ = metrics.min_position;
      Stop();
      return;
    }
    if (position_ > metrics.max_position) {
      position_ = metrics.max_position;
      Stop();
      return;
    }
  } else if (IsOutOfRange(metrics)) {
    const double boundary =
        position_ < metrics.min_position ? metrics.min_position : metrics.max_position;
    const double boundary_time =
        std::min(delta_seconds, TimeToBallisticPosition(start_position, start_velocity, boundary));
    const double remaining_time = std::max(0.0, delta_seconds - boundary_time);

    position_ = boundary;
    velocity_ = start_velocity * std::pow(options_.deceleration_rate, boundary_time);
    velocity_ = std::clamp(velocity_, -kMaxSpringTransferVelocity, kMaxSpringTransferVelocity);
    StartSpring(metrics);
    if (remaining_time > 0.0) {
      AdvanceSpring(remaining_time, metrics);
    }
    return;
  }

  if (std::abs(velocity_) < kVelocityTolerance) {
    Stop();
  }
}

void ScrollInteractionController::AdvanceSpring(double delta_seconds, const ScrollMetrics& metrics) {
  spring_target_ = SpringTarget(metrics);

  const double mass = options_.spring.mass;
  const double stiffness = options_.spring.stiffness;
  const double damping =
      options_.spring.damping_ratio * 2.0 * std::sqrt(options_.spring.mass * options_.spring.stiffness);
  const double displacement = position_ - spring_target_;
  const double acceleration = ((-stiffness * displacement) - (damping * velocity_)) / mass;

  velocity_ += acceleration * delta_seconds;
  position_ += velocity_ * delta_seconds;

  if (std::abs(position_ - spring_target_) <= kPositionTolerance &&
      std::abs(velocity_) <= kVelocityTolerance) {
    position_ = spring_target_;
    Stop();
  }
}

}  // namespace kairo
