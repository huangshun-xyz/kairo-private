#include "crosshair_layer.h"

#include "render_context.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"

namespace kairo {

void CrosshairLayer::SetCrosshair(double logical_x, double value) {
  logical_x_ = logical_x;
  value_ = value;
}

void CrosshairLayer::SetVisible(bool visible) {
  visible_ = visible;
}

void CrosshairLayer::SetColor(SkColor color) {
  color_ = color;
}

LayerDrawOrder CrosshairLayer::draw_order() const {
  return LayerDrawOrder::kOverlay;
}

void CrosshairLayer::Draw(RenderContext* ctx) {
  if (!visible_ || ctx == nullptr || ctx->canvas == nullptr || ctx->x_scale == nullptr ||
      ctx->y_scale == nullptr) {
    return;
  }

  const float x = ctx->x_scale->DataToScreen(logical_x_);
  const float y = ctx->y_scale->ValueToScreen(value_);

  SkPaint crosshair_paint;
  crosshair_paint.setAntiAlias(true);
  crosshair_paint.setColor(color_);
  crosshair_paint.setStrokeWidth(1.0f);

  ctx->canvas->drawLine(x, ctx->pane_bounds.top(), x, ctx->pane_bounds.bottom(), crosshair_paint);
  ctx->canvas->drawLine(ctx->pane_bounds.left(), y, ctx->pane_bounds.right(), y, crosshair_paint);
  ctx->canvas->drawCircle(x, y, 3.0f, crosshair_paint);
}

bool CrosshairLayer::HitTest(RenderContext* ctx, SkPoint point, HitResult* result) {
  if (!visible_ || ctx == nullptr || result == nullptr) {
    return false;
  }

  result->position = point;
  result->logical_x = logical_x_;
  result->value = value_;
  return false;
}

}  // namespace kairo
