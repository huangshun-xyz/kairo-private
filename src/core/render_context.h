#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "viewport.h"
#include "x_scale.h"
#include "y_scale.h"

namespace kairo {

// RenderContext 承载某个 pane 在一次绘制过程中的上下文。
struct RenderContext {
  SkCanvas* canvas = nullptr;
  SkRect pane_bounds = SkRect::MakeEmpty();
  const XScale* x_scale = nullptr;
  const YScale* y_scale = nullptr;
  const Viewport* viewport = nullptr;
};

}  // namespace kairo
