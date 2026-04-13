#include "grid_layer.h"

#include <algorithm>

#include "render_context.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"

namespace kairo {

namespace {

constexpr float kGridStrokeWidth = 1.0f;
constexpr int kGridRows = 4;
constexpr int kGridColumns = 4;

}  // namespace

void GridLayer::SetBackgroundColor(SkColor color) {
  background_color_ = color;
}

void GridLayer::SetGridLineColor(SkColor color) {
  grid_line_color_ = color;
}

LayerDrawOrder GridLayer::draw_order() const {
  return LayerDrawOrder::kUnderlay;
}

void GridLayer::Draw(RenderContext* ctx) {
  if (ctx == nullptr || ctx->canvas == nullptr) {
    return;
  }

  SkPaint background_paint;
  background_paint.setColor(background_color_);
  ctx->canvas->drawRect(ctx->pane_content_bounds, background_paint);

  SkPaint grid_paint;
  grid_paint.setAntiAlias(true);
  grid_paint.setColor(grid_line_color_);
  grid_paint.setStrokeWidth(kGridStrokeWidth);

  const int rows = std::max(kGridRows, 2);
  const int columns = std::max(kGridColumns, 2);

  for (int row = 0; row < rows; ++row) {
    const float y = ctx->pane_content_bounds.top() +
        (ctx->pane_content_bounds.height() * static_cast<float>(row) /
         static_cast<float>(rows - 1));
    ctx->canvas->drawLine(
        ctx->pane_content_bounds.left(), y, ctx->pane_content_bounds.right(), y, grid_paint);
  }

  for (int column = 0; column < columns; ++column) {
    const float x = ctx->pane_content_bounds.left() +
        (ctx->pane_content_bounds.width() * static_cast<float>(column) /
         static_cast<float>(columns - 1));
    ctx->canvas->drawLine(
        x, ctx->pane_content_bounds.top(), x, ctx->pane_content_bounds.bottom(), grid_paint);
  }
}

bool GridLayer::HitTest(RenderContext* ctx, SkPoint point, HitResult* result) {
  (void)ctx;
  (void)point;
  (void)result;
  return false;
}

}  // namespace kairo
