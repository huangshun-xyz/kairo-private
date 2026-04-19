#include "chart.h"

#include <algorithm>

#include "chart_controller.h"
#include "include/core/SkCanvas.h"
#include "render_context.h"

namespace kairo {

namespace {

SkRect ApplyInsets(const SkRect& rect, const Insets& insets) {
  const float left = std::min(rect.right(), rect.left() + insets.left);
  const float top = std::min(rect.bottom(), rect.top() + insets.top);
  const float right = std::max(left, rect.right() - insets.right);
  const float bottom = std::max(top, rect.bottom() - insets.bottom);
  return SkRect::MakeLTRB(left, top, right, bottom);
}

bool IsStretchPane(const Pane& pane) {
  return pane.layout().size_mode == PaneSizeMode::kStretch;
}

float GetFixedHeight(const Pane& pane) {
  return IsStretchPane(pane) ? 0.0f : std::max(pane.layout().height, 0.0f);
}

}  // namespace

Chart::Chart() {
  controller_ = std::make_unique<ChartController>(this);
}

Chart::~Chart() = default;

void Chart::SetBounds(const SkRect& bounds) {
  Layout(bounds);
}

void Chart::Layout(const SkRect& bounds) {
  bounds_ = bounds;
  LayoutPanes();
}

const SkRect& Chart::bounds() const {
  return bounds_;
}

const SkRect& Chart::content_bounds() const {
  return content_bounds_;
}

void Chart::SetContentInsets(const Insets& insets) {
  content_insets_ = insets;
  LayoutPanes();
}

const Insets& Chart::content_insets() const {
  return content_insets_;
}

void Chart::SetViewport(const Viewport& viewport) {
  if (!viewport.IsValid()) {
    return;
  }

  viewport_ = viewport;
}

const Viewport& Chart::viewport() const {
  return viewport_;
}

UniformBarXScale* Chart::x_scale() {
  return &x_scale_;
}

const UniformBarXScale* Chart::x_scale() const {
  return &x_scale_;
}

Pane* Chart::AddPane(std::unique_ptr<Pane> pane) {
  if (pane == nullptr) {
    return nullptr;
  }

  panes_.push_back(std::move(pane));
  LayoutPanes();
  return panes_.back().get();
}

std::vector<std::unique_ptr<Pane>>& Chart::panes() {
  return panes_;
}

const std::vector<std::unique_ptr<Pane>>& Chart::panes() const {
  return panes_;
}

ChartOverlay* Chart::AddOverlay(std::unique_ptr<ChartOverlay> overlay) {
  if (overlay == nullptr) {
    return nullptr;
  }

  overlays_.push_back(std::move(overlay));
  return overlays_.back().get();
}

std::vector<std::unique_ptr<ChartOverlay>>& Chart::overlays() {
  return overlays_;
}

const std::vector<std::unique_ptr<ChartOverlay>>& Chart::overlays() const {
  return overlays_;
}

ChartController* Chart::controller() {
  return controller_.get();
}

const ChartController* Chart::controller() const {
  return controller_.get();
}

void Chart::Draw(SkCanvas& canvas) {
  if (panes_.empty() || bounds_.isEmpty()) {
    return;
  }

  SyncScales();

  for (const auto& pane : panes_) {
    RenderContext ctx;
    ctx.canvas = &canvas;
    ctx.chart_bounds = bounds_;
    ctx.chart_content_bounds = content_bounds_;
    ctx.pane_frame = pane->frame_rect();
    ctx.pane_content_bounds = pane->content_rect();
    ctx.x_scale = &x_scale_;
    ctx.y_scale = pane->y_scale();
    ctx.viewport = &viewport_;

    if (ctx.pane_content_bounds.isEmpty()) {
      continue;
    }

    canvas.save();
    canvas.clipRect(ctx.pane_content_bounds);

    for (const auto& layer : pane->layers()) {
      if (layer->draw_order() == LayerDrawOrder::kUnderlay) {
        layer->Draw(&ctx);
      }
    }

    for (const auto& series : pane->series()) {
      series->Draw(&ctx);
    }

    for (const auto& layer : pane->layers()) {
      if (layer->draw_order() == LayerDrawOrder::kOverlay) {
        layer->Draw(&ctx);
      }
    }

    canvas.restore();
  }

  ChartRenderContext chart_ctx;
  chart_ctx.canvas = &canvas;
  chart_ctx.chart = this;
  chart_ctx.chart_bounds = bounds_;
  chart_ctx.chart_content_bounds = content_bounds_;
  chart_ctx.x_scale = &x_scale_;
  chart_ctx.viewport = &viewport_;

  canvas.save();
  canvas.clipRect(content_bounds_);
  for (const auto& overlay : overlays_) {
    overlay->Draw(&chart_ctx);
  }
  canvas.restore();
}

void Chart::LayoutPanes() {
  content_bounds_ = ApplyInsets(bounds_, content_insets_);

  if (panes_.empty()) {
    return;
  }

  float total_fixed_height = 0.0f;
  size_t stretch_count = 0;
  for (const auto& pane : panes_) {
    total_fixed_height += GetFixedHeight(*pane);
    stretch_count += IsStretchPane(*pane) ? 1u : 0u;
  }

  const float remaining_height = std::max(0.0f, content_bounds_.height() - total_fixed_height);
  const float stretch_height =
      stretch_count > 0 ? (remaining_height / static_cast<float>(stretch_count)) : 0.0f;

  float current_top = content_bounds_.top();
  for (auto& pane : panes_) {
    const float pane_height = IsStretchPane(*pane) ? stretch_height : GetFixedHeight(*pane);
    const SkRect frame = SkRect::MakeXYWH(
        content_bounds_.left(), current_top, content_bounds_.width(), pane_height);
    const SkRect content = ApplyInsets(frame, pane->layout().insets);
    pane->SetFrameRect(frame);
    pane->SetContentRect(content);
    current_top += pane_height;
  }
}

void Chart::SyncScales() {
  LayoutPanes();

  x_scale_.SetViewport(viewport_);
  x_scale_.SetBounds(content_bounds_);

  for (const auto& pane : panes_) {
    pane->UpdateAutoRange(viewport_);
  }
}

}  // namespace kairo
