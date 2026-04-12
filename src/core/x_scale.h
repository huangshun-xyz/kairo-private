#pragma once

#include "include/core/SkRect.h"
#include "viewport.h"

namespace kairo {

// XScale 负责管理逻辑索引与屏幕横坐标之间的映射。
class XScale {
 public:
  virtual ~XScale() = default;

  // 更新当前可见的逻辑区间。
  virtual void SetViewport(const Viewport& viewport) = 0;
  virtual const Viewport& viewport() const = 0;

  // 更新映射使用的像素边界。
  virtual void SetBounds(const SkRect& bounds) = 0;
  virtual const SkRect& bounds() const = 0;

  // 在逻辑索引空间和屏幕横坐标之间做双向转换。
  virtual float DataToScreen(double logical_x) const = 0;
  virtual double ScreenToData(float screen_x) const = 0;

  // 返回当前每根 bar 占用的像素宽度。
  virtual float BarSpacing() const = 0;
};

}  // namespace kairo
