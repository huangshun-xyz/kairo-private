#pragma once

#include "include/core/SkRect.h"
#include "range.h"

namespace kairo {

// YScale 负责管理单个 pane 内数值与屏幕纵坐标之间的映射。
class YScale {
 public:
  virtual ~YScale() = default;

  // 更新当前 pane 的可见数值范围。
  virtual void SetRange(const Range& range) = 0;
  virtual const Range& range() const = 0;

  // 更新映射使用的 pane 像素边界。
  virtual void SetBounds(const SkRect& bounds) = 0;
  virtual const SkRect& bounds() const = 0;

  // 在业务数值和屏幕纵坐标之间做双向转换。
  virtual float ValueToScreen(double value) const = 0;
  virtual double ScreenToValue(float screen_y) const = 0;
};

}  // namespace kairo
