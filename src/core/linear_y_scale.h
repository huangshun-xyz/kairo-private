#pragma once

#include "include/core/SkRect.h"
#include "range.h"

namespace kairo {

// Pane 内的线性纵轴。
class LinearYScale {
 public:
  LinearYScale();

  void SetRange(const Range& range);
  const Range& range() const;

  void SetBounds(const SkRect& bounds);
  const SkRect& bounds() const;

  float ValueToScreen(double value) const;
  double ScreenToValue(float screen_y) const;

 private:
  Range range_;
  SkRect bounds_ = SkRect::MakeEmpty();
};

}  // namespace kairo
