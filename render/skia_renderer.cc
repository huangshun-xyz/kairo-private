#include "skia_renderer.h"

#include <algorithm>

#include "chart.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRect.h"

namespace {

SkColor BarColorForIndex(int index) {
  static constexpr SkColor kPalette[] = {
      SkColorSetRGB(30, 99, 255),
      SkColorSetRGB(0, 176, 116),
      SkColorSetRGB(255, 154, 0),
      SkColorSetRGB(255, 89, 94),
      SkColorSetRGB(111, 88, 255),
  };
  return kPalette[index % (sizeof(kPalette) / sizeof(kPalette[0]))];
}

}  // namespace

namespace kairo::render {

void SkiaRenderer::DrawChart(
    SkCanvas& canvas,
    const core::Chart& chart,
    int width,
    int height) const {
  if (width <= 0 || height <= 0) {
    return;
  }

  const SkScalar safe_width = static_cast<SkScalar>(width);
  const SkScalar safe_height = static_cast<SkScalar>(height);

  canvas.clear(SkColorSetRGB(242, 245, 249));

  const SkScalar outer_padding = std::min(safe_width, safe_height) * 0.08f;
  const SkRect card_rect = SkRect::MakeXYWH(
      outer_padding,
      outer_padding,
      safe_width - (outer_padding * 2.0f),
      safe_height - (outer_padding * 2.0f));

  SkPaint card_paint;
  card_paint.setAntiAlias(true);
  card_paint.setColor(SK_ColorWHITE);
  canvas.drawRRect(SkRRect::MakeRectXY(card_rect, 28.0f, 28.0f), card_paint);

  const SkScalar plot_left = card_rect.left() + 28.0f;
  const SkScalar plot_top = card_rect.top() + 28.0f;
  const SkScalar plot_right = card_rect.right() - 28.0f;
  const SkScalar plot_bottom = card_rect.bottom() - 28.0f;
  const SkScalar plot_height = plot_bottom - plot_top;

  SkPaint grid_paint;
  grid_paint.setColor(SkColorSetRGB(229, 234, 240));
  grid_paint.setStrokeWidth(1.0f);

  for (int step = 0; step < 4; ++step) {
    const SkScalar y = plot_top + (plot_height * static_cast<SkScalar>(step) / 3.0f);
    canvas.drawLine(plot_left, y, plot_right, y, grid_paint);
  }

  const auto& entries = chart.entries();
  if (entries.empty()) {
    return;
  }

  const SkScalar bar_gap = 14.0f;
  const SkScalar bar_width =
      (plot_right - plot_left - (bar_gap * static_cast<SkScalar>(entries.size() - 1))) /
      static_cast<SkScalar>(entries.size());
  const float max_value = std::max(chart.max_value(), 1.0f);

  for (size_t index = 0; index < entries.size(); ++index) {
    const float normalized = entries[index].value / max_value;
    const SkScalar bar_height = plot_height * static_cast<SkScalar>(normalized);
    const SkScalar left =
        plot_left + (static_cast<SkScalar>(index) * (bar_width + bar_gap));
    const SkScalar top = plot_bottom - bar_height;

    SkPaint bar_paint;
    bar_paint.setAntiAlias(true);
    bar_paint.setColor(BarColorForIndex(static_cast<int>(index)));

    const SkRect bar_rect = SkRect::MakeXYWH(left, top, bar_width, bar_height);
    canvas.drawRRect(SkRRect::MakeRectXY(bar_rect, 14.0f, 14.0f), bar_paint);

    SkPaint marker_paint;
    marker_paint.setAntiAlias(true);
    marker_paint.setColor(SK_ColorWHITE);
    canvas.drawCircle(left + (bar_width * 0.5f), top + 12.0f, 5.0f, marker_paint);
  }
}

}  // namespace kairo::render
