#include "candle_series.h"

#include <algorithm>
#include <cmath>

#include "render_context.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"

namespace kairo {

namespace {

constexpr float kCandleWidthRatio = 0.68f;

}  // namespace

CandleSeries::CandleSeries(std::shared_ptr<ICandleDataSource> data_source)
    : data_source_(std::move(data_source)) {}

void CandleSeries::SetUpColor(SkColor color) {
  up_color_ = color;
}

void CandleSeries::SetDownColor(SkColor color) {
  down_color_ = color;
}

void CandleSeries::SetWickColor(SkColor color) {
  wick_color_ = color;
}

void CandleSeries::Draw(RenderContext* ctx) {
  if (ctx == nullptr || ctx->canvas == nullptr || ctx->x_scale == nullptr ||
      ctx->y_scale == nullptr || data_source_ == nullptr) {
    return;
  }

  const size_t candle_count = data_source_->GetCount();
  if (candle_count == 0) {
    return;
  }

  const int first_index = std::max(0, static_cast<int>(std::floor(ctx->viewport->visible_from)));
  const int last_index = std::min(
      static_cast<int>(candle_count) - 1,
      static_cast<int>(std::ceil(ctx->viewport->visible_to)));
  if (first_index > last_index) {
    return;
  }

  const float candle_width = std::max(1.0f, ctx->x_scale->BarSpacing() * kCandleWidthRatio);

  SkPaint wick_paint;
  wick_paint.setAntiAlias(true);
  wick_paint.setStrokeWidth(1.0f);

  SkPaint body_paint;
  body_paint.setAntiAlias(true);

  for (int index = first_index; index <= last_index; ++index) {
    CandleData candle;
    if (!data_source_->GetCandle(static_cast<size_t>(index), &candle)) {
      continue;
    }

    const float x = ctx->x_scale->DataToScreen(static_cast<double>(index));
    const float high_y = ctx->y_scale->ValueToScreen(candle.high);
    const float low_y = ctx->y_scale->ValueToScreen(candle.low);
    const float open_y = ctx->y_scale->ValueToScreen(candle.open);
    const float close_y = ctx->y_scale->ValueToScreen(candle.close);

    const bool is_up = candle.close >= candle.open;
    const SkColor candle_color = is_up ? up_color_ : down_color_;

    wick_paint.setColor(wick_color_);
    body_paint.setColor(candle_color);

    ctx->canvas->drawLine(x, high_y, x, low_y, wick_paint);

    const float body_top = std::min(open_y, close_y);
    float body_bottom = std::max(open_y, close_y);
    if (std::abs(body_bottom - body_top) < 1.0f) {
      body_bottom = body_top + 1.0f;
    }

    const SkRect body_rect = SkRect::MakeLTRB(
        x - (candle_width * 0.5f), body_top, x + (candle_width * 0.5f), body_bottom);
    ctx->canvas->drawRect(body_rect, body_paint);
  }
}

Range CandleSeries::GetVisibleRange(const Viewport& viewport) const {
  if (data_source_ == nullptr || data_source_->GetCount() == 0) {
    return Range();
  }

  Range visible_range;
  const int first_index = std::max(0, static_cast<int>(std::floor(viewport.visible_from)));
  const int last_index = std::min(
      static_cast<int>(data_source_->GetCount()) - 1,
      static_cast<int>(std::ceil(viewport.visible_to)));

  for (int index = first_index; index <= last_index; ++index) {
    CandleData candle;
    if (!data_source_->GetCandle(static_cast<size_t>(index), &candle)) {
      continue;
    }

    visible_range.Include(candle.low);
    visible_range.Include(candle.high);
  }

  return visible_range;
}

bool CandleSeries::HitTest(RenderContext* ctx, SkPoint point, HitResult* result) {
  if (ctx == nullptr || ctx->x_scale == nullptr || data_source_ == nullptr) {
    return false;
  }

  const int nearest_index = static_cast<int>(std::round(ctx->x_scale->ScreenToData(point.x())));
  if (nearest_index < 0 || nearest_index >= static_cast<int>(data_source_->GetCount())) {
    return false;
  }

  CandleData candle;
  if (!data_source_->GetCandle(static_cast<size_t>(nearest_index), &candle)) {
    return false;
  }

  if (result != nullptr) {
    result->position = point;
    result->logical_x = nearest_index;
    result->value = candle.close;
  }
  return true;
}

}  // namespace kairo
