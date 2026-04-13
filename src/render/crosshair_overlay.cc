#include "crosshair_overlay.h"

#include "chart.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "render_context.h"

namespace kairo {

void CrosshairOverlay::SetCrosshair(size_t pane_index, double logical_x, double value) {
  active_pane_index_ = pane_index;
  logical_x_ = logical_x;
  value_ = value;
}

void CrosshairOverlay::SetVisible(bool visible) {
  visible_ = visible;
}

void CrosshairOverlay::SetColor(SkColor color) {
  color_ = color;
}

void CrosshairOverlay::Draw(ChartRenderContext* ctx) {
  if (!visible_ || ctx == nullptr || ctx->canvas == nullptr || ctx->chart == nullptr ||
      ctx->x_scale == nullptr) {
    return;
  }

  const auto& panes = ctx->chart->panes();
  if (panes.empty() || active_pane_index_ >= panes.size()) {
    return;
  }

  SkPaint crosshair_paint;
  crosshair_paint.setAntiAlias(true);
  crosshair_paint.setColor(color_);
  crosshair_paint.setStrokeWidth(1.0f);

  const float x = ctx->x_scale->DataToScreen(logical_x_);
  for (const auto& pane : panes) {
    const SkRect& bounds = pane->content_rect();
    if (bounds.isEmpty()) {
      continue;
    }

    ctx->canvas->drawLine(x, bounds.top(), x, bounds.bottom(), crosshair_paint);
  }

  const Pane* active_pane = panes[active_pane_index_].get();
  if (active_pane == nullptr || active_pane->y_scale() == nullptr) {
    return;
  }

  const SkRect& active_bounds = active_pane->content_rect();
  if (active_bounds.isEmpty()) {
    return;
  }

  const float y = active_pane->y_scale()->ValueToScreen(value_);
  ctx->canvas->drawLine(active_bounds.left(), y, active_bounds.right(), y, crosshair_paint);
  ctx->canvas->drawCircle(x, y, 3.0f, crosshair_paint);
}

bool CrosshairOverlay::HitTest(ChartRenderContext* ctx, SkPoint point, HitResult* result) {
  if (!visible_ || ctx == nullptr || result == nullptr) {
    return false;
  }

  result->position = point;
  result->logical_x = logical_x_;
  result->value = value_;
  return false;
}

}  // namespace kairo
