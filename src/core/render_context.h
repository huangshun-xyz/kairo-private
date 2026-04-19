#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "linear_y_scale.h"
#include "uniform_bar_x_scale.h"
#include "viewport.h"

namespace kairo {

class Chart;

// RenderContext 承载某个 pane 在一次绘制过程中的上下文。
struct RenderContext {
  SkCanvas* canvas = nullptr;
  SkRect chart_bounds = SkRect::MakeEmpty();
  SkRect chart_content_bounds = SkRect::MakeEmpty();
  SkRect pane_frame = SkRect::MakeEmpty();
  SkRect pane_content_bounds = SkRect::MakeEmpty();
  const UniformBarXScale* x_scale = nullptr;
  const LinearYScale* y_scale = nullptr;
  const Viewport* viewport = nullptr;
};

// ChartRenderContext 承载 chart 级 overlay 的绘制上下文。
struct ChartRenderContext {
  SkCanvas* canvas = nullptr;
  const Chart* chart = nullptr;
  SkRect chart_bounds = SkRect::MakeEmpty();
  SkRect chart_content_bounds = SkRect::MakeEmpty();
  const UniformBarXScale* x_scale = nullptr;
  const Viewport* viewport = nullptr;
};

}  // namespace kairo
