#include "volume_series.h"

#include <algorithm>
#include <cmath>

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "render_context.h"

namespace kairo {

namespace {

constexpr float kVolumeBarWidthRatio = 0.68f;

}  // namespace

VolumeSeries::VolumeSeries(std::shared_ptr<VectorCandleDataSource> data_source)
    : data_source_(std::move(data_source)) {}

void VolumeSeries::SetUpColor(SkColor color) {
  up_color_ = color;
}

void VolumeSeries::SetDownColor(SkColor color) {
  down_color_ = color;
}

void VolumeSeries::Draw(RenderContext* ctx) {
  if (ctx == nullptr || ctx->canvas == nullptr || ctx->x_scale == nullptr ||
      ctx->y_scale == nullptr || ctx->viewport == nullptr || data_source_ == nullptr) {
    return;
  }

  const size_t candle_count = data_source_->GetCount();
  if (candle_count == 0 || ctx->pane_content_bounds.isEmpty()) {
    return;
  }

  const int first_index = std::max(0, static_cast<int>(std::floor(ctx->viewport->visible_from)));
  const int last_index = std::min(
      static_cast<int>(candle_count) - 1,
      static_cast<int>(std::ceil(ctx->viewport->visible_to)));
  if (first_index > last_index) {
    return;
  }

  const float bar_width = std::max(1.0f, ctx->x_scale->BarSpacing() * kVolumeBarWidthRatio);
  const float bottom = ctx->pane_content_bounds.bottom();

  SkPaint bar_paint;
  bar_paint.setAntiAlias(true);

  for (int index = first_index; index <= last_index; ++index) {
    CandleData candle;
    if (!data_source_->GetCandle(static_cast<size_t>(index), &candle)) {
      continue;
    }

    const float x = ctx->x_scale->DataToScreen(static_cast<double>(index));
    const float volume_y = ctx->y_scale->ValueToScreen(candle.volume);
    const float top = std::min(volume_y, bottom);
    const SkRect volume_rect =
        SkRect::MakeLTRB(x - (bar_width * 0.5f), top, x + (bar_width * 0.5f), bottom);

    bar_paint.setColor(candle.close >= candle.open ? up_color_ : down_color_);
    ctx->canvas->drawRect(volume_rect, bar_paint);
  }
}

Range VolumeSeries::GetVisibleRange(const Viewport& viewport) const {
  if (data_source_ == nullptr || data_source_->GetCount() == 0) {
    return Range();
  }

  Range visible_range;
  visible_range.Include(0.0);

  const int first_index = std::max(0, static_cast<int>(std::floor(viewport.visible_from)));
  const int last_index = std::min(
      static_cast<int>(data_source_->GetCount()) - 1,
      static_cast<int>(std::ceil(viewport.visible_to)));

  for (int index = first_index; index <= last_index; ++index) {
    CandleData candle;
    if (!data_source_->GetCandle(static_cast<size_t>(index), &candle)) {
      continue;
    }

    visible_range.Include(candle.volume);
  }

  return visible_range;
}

bool VolumeSeries::HitTest(RenderContext* ctx, SkPoint point, HitResult* result) {
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
    result->value = candle.volume;
  }
  return true;
}

}  // namespace kairo
