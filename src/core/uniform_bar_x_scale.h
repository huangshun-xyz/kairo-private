#pragma once

#include "x_scale.h"

namespace kairo {

// 默认的 XScale 实现：可视区内每根 bar 使用统一间距。
class UniformBarXScale final : public XScale {
 public:
  UniformBarXScale();

  void SetViewport(const Viewport& viewport) override;
  const Viewport& viewport() const override;

  void SetBounds(const SkRect& bounds) override;
  const SkRect& bounds() const override;

  float DataToScreen(double logical_x) const override;
  double ScreenToData(float screen_x) const override;
  float BarSpacing() const override;

 private:
  void UpdateMetrics();

  Viewport viewport_;
  SkRect bounds_ = SkRect::MakeEmpty();
  float bar_spacing_ = 1.0f;
};

}  // namespace kairo
