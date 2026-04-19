#pragma once

#include "include/core/SkRect.h"
#include "viewport.h"

namespace kairo {

// 共享横轴：可视区内每根 bar 使用统一间距。
class UniformBarXScale {
 public:
  UniformBarXScale();

  void SetViewport(const Viewport& viewport);
  const Viewport& viewport() const;

  void SetBounds(const SkRect& bounds);
  const SkRect& bounds() const;

  float DataToScreen(double logical_x) const;
  double ScreenToData(float screen_x) const;
  float BarSpacing() const;

 private:
  void UpdateMetrics();

  Viewport viewport_;
  SkRect bounds_ = SkRect::MakeEmpty();
  float bar_spacing_ = 1.0f;
};

}  // namespace kairo
