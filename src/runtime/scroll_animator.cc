#include "scroll_animator.h"

#include <algorithm>

#include "kchart.h"
#include "time_types.h"

namespace kairo {

namespace {

double ClampFrameDelta(double delta_seconds) {
  return std::clamp(delta_seconds, 1.0 / 240.0, 1.0 / 20.0);
}

}  // namespace

KScrollAnimator::KScrollAnimator(KChart* chart) : chart_(chart) {}

void KScrollAnimator::BindHost(const KRuntimeHost& host) {
  host_ = host;
}

void KScrollAnimator::UnbindHost() {
  Cancel();
  host_ = {};
}

void KScrollAnimator::Cancel() {
  pan_active_ = false;
  StopAnimation();
  if (chart_ != nullptr) {
    chart_->StopScrollAnimation();
  }
}

bool KScrollAnimator::IsAnimating() const {
  return subscribed_;
}

void KScrollAnimator::OnPanBegin(float x, float y) {
  (void)x;
  (void)y;

  if (chart_ == nullptr) {
    return;
  }

  StopAnimation();
  chart_->StopScrollAnimation();
  chart_->BeginScrollGesture();
  pan_active_ = true;
}

void KScrollAnimator::OnPanUpdate(float delta_x, float delta_y) {
  (void)delta_y;

  if (chart_ == nullptr) {
    return;
  }

  if (!pan_active_) {
    OnPanBegin(0.0f, 0.0f);
  }

  if (chart_->ScrollGestureByPixels(delta_x)) {
    RequestRedraw();
  }
}

void KScrollAnimator::OnPanEnd(float velocity_x, float velocity_y) {
  (void)velocity_y;

  if (chart_ == nullptr) {
    return;
  }

  if (!pan_active_) {
    return;
  }

  pan_active_ = false;
  const bool active = chart_->EndScrollGesture(velocity_x);
  RequestRedraw();
  if (active) {
    StartAnimation();
  } else {
    StopAnimation();
  }
}

void KScrollAnimator::OnFrame(const KFrameArgs& args) {
  if (chart_ == nullptr) {
    StopAnimation();
    return;
  }

  double delta_seconds = 1.0 / 120.0;
  if (last_frame_time_.has_value()) {
    delta_seconds = ToSeconds(args.frame_time - *last_frame_time_);
  } else if (args.interval.micros > 0) {
    delta_seconds = ToSeconds(args.interval);
  }
  delta_seconds = ClampFrameDelta(delta_seconds);
  last_frame_time_ = args.frame_time;

  const bool active = chart_->StepScrollAnimation(delta_seconds);
  RequestRedraw();
  if (!active || !chart_->IsScrollAnimationActive()) {
    StopAnimation();
  }
}

void KScrollAnimator::StartAnimation() {
  if (subscribed_ || host_.frame_source == nullptr) {
    return;
  }

  last_frame_time_.reset();
  host_.frame_source->AddObserver(this);
  subscribed_ = true;
}

void KScrollAnimator::StopAnimation() {
  if (subscribed_ && host_.frame_source != nullptr) {
    host_.frame_source->RemoveObserver(this);
  }

  subscribed_ = false;
  last_frame_time_.reset();
}

void KScrollAnimator::RequestRedraw() const {
  if (host_.invalidation_sink != nullptr) {
    host_.invalidation_sink->RequestRedraw();
  }
}

}  // namespace kairo
