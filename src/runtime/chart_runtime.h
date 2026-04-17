#pragma once

#include "runtime_host.h"
#include "scroll_animator.h"

namespace kairo {

class KChart;

class KChartRuntime final {
 public:
  explicit KChartRuntime(KChart* chart);

  void BindHost(const KRuntimeHost& host);
  void UnbindHost();
  void CancelTransientInteractions();

  void OnPanBegin(float x, float y);
  void OnPanUpdate(float delta_x, float delta_y);
  void OnPanEnd(float velocity_x, float velocity_y);

  void OnPinchBegin(float anchor_ratio);
  void OnPinchUpdate(float scale, float anchor_ratio);
  void OnPinchEnd();

  void OnLongPressBegin(float x, float y);
  void OnLongPressMove(float x, float y);
  void OnLongPressEnd();

 private:
  void RequestRedraw() const;

  KChart* chart_ = nullptr;
  KRuntimeHost host_;
  KScrollAnimator scroll_animator_;
  bool pinch_active_ = false;
  float last_pinch_scale_ = 1.0f;
};

}  // namespace kairo
