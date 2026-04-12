#pragma once

#include "y_scale.h"

namespace kairo {

// 默认的 YScale 实现：数值在线性区间内映射到 pane 像素坐标。
class LinearYScale final : public YScale {
 public:
  LinearYScale();

  void SetRange(const Range& range) override;
  const Range& range() const override;

  void SetBounds(const SkRect& bounds) override;
  const SkRect& bounds() const override;

  float ValueToScreen(double value) const override;
  double ScreenToValue(float screen_y) const override;

 private:
  Range range_;
  SkRect bounds_ = SkRect::MakeEmpty();
};

}  // namespace kairo
