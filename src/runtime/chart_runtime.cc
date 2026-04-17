#include "chart_runtime.h"

#include <algorithm>

#include "kchart.h"

namespace kairo {

KChartRuntime::KChartRuntime(KChart* chart) : chart_(chart), scroll_animator_(chart) {}

void KChartRuntime::BindHost(const KRuntimeHost& host) {
  host_ = host;
  scroll_animator_.BindHost(host);
}

void KChartRuntime::UnbindHost() {
  scroll_animator_.UnbindHost();
  host_ = {};
}

void KChartRuntime::CancelTransientInteractions() {
  pinch_active_ = false;
  last_pinch_scale_ = 1.0f;
  scroll_animator_.Cancel();
}

void KChartRuntime::OnPanBegin(float x, float y) {
  pinch_active_ = false;
  last_pinch_scale_ = 1.0f;
  scroll_animator_.OnPanBegin(x, y);
}

void KChartRuntime::OnPanUpdate(float delta_x, float delta_y) {
  scroll_animator_.OnPanUpdate(delta_x, delta_y);
}

void KChartRuntime::OnPanEnd(float velocity_x, float velocity_y) {
  scroll_animator_.OnPanEnd(velocity_x, velocity_y);
}

void KChartRuntime::OnPinchBegin(float anchor_ratio) {
  (void)anchor_ratio;

  scroll_animator_.Cancel();
  pinch_active_ = true;
  last_pinch_scale_ = 1.0f;
}

void KChartRuntime::OnPinchUpdate(float scale, float anchor_ratio) {
  if (chart_ == nullptr) {
    return;
  }

  if (!pinch_active_) {
    OnPinchBegin(anchor_ratio);
  }

  const float delta_scale = scale / std::max(last_pinch_scale_, 0.01f);
  last_pinch_scale_ = scale;
  chart_->ZoomBy(delta_scale, anchor_ratio);
  RequestRedraw();
}

void KChartRuntime::OnPinchEnd() {
  pinch_active_ = false;
  last_pinch_scale_ = 1.0f;
}

void KChartRuntime::OnLongPressBegin(float x, float y) {
  if (chart_ == nullptr) {
    return;
  }

  scroll_animator_.Cancel();
  chart_->UpdateCrosshair(x, y);
  RequestRedraw();
}

void KChartRuntime::OnLongPressMove(float x, float y) {
  if (chart_ == nullptr) {
    return;
  }

  chart_->UpdateCrosshair(x, y);
  RequestRedraw();
}

void KChartRuntime::OnLongPressEnd() {
  if (chart_ == nullptr) {
    return;
  }

  chart_->ClearCrosshair();
  RequestRedraw();
}

void KChartRuntime::RequestRedraw() const {
  if (host_.invalidation_sink != nullptr) {
    host_.invalidation_sink->RequestRedraw();
  }
}

}  // namespace kairo
