#pragma once

#include <optional>

#include "frame_source.h"
#include "runtime_host.h"

namespace kairo {

class KChart;

class KScrollAnimator final : public KFrameObserver {
 public:
  explicit KScrollAnimator(KChart* chart);

  void BindHost(const KRuntimeHost& host);
  void UnbindHost();

  void Cancel();
  bool IsAnimating() const;

  void OnPanBegin(float x, float y);
  void OnPanUpdate(float delta_x, float delta_y);
  void OnPanEnd(float velocity_x, float velocity_y);

  void OnFrame(const KFrameArgs& args) override;

 private:
  void StartAnimation();
  void StopAnimation();
  void RequestRedraw() const;

  KChart* chart_ = nullptr;
  KRuntimeHost host_;
  bool subscribed_ = false;
  bool pan_active_ = false;
  std::optional<KTimeTicks> last_frame_time_;
};

}  // namespace kairo
