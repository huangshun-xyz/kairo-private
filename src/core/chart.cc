#include "chart.h"

#include <algorithm>

#include "chart_controller.h"
#include "include/core/SkCanvas.h"
#include "render_context.h"
#include "uniform_bar_x_scale.h"

namespace kairo {

Chart::Chart() : Chart(std::make_unique<UniformBarXScale>()) {}

Chart::Chart(std::unique_ptr<XScale> x_scale) : x_scale_(std::move(x_scale)) {
  controller_ = std::make_unique<ChartController>(this);
}

Chart::~Chart() = default;

void Chart::SetBounds(const SkRect& bounds) {
  bounds_ = bounds;
}

const SkRect& Chart::bounds() const {
  return bounds_;
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

XScale* Chart::x_scale() {
  return x_scale_.get();
}

const XScale* Chart::x_scale() const {
  return x_scale_.get();
}

Pane* Chart::AddPane(std::unique_ptr<Pane> pane) {
  if (pane == nullptr) {
    return nullptr;
  }

  panes_.push_back(std::move(pane));
  return panes_.back().get();
}

std::vector<std::unique_ptr<Pane>>& Chart::panes() {
  return panes_;
}

const std::vector<std::unique_ptr<Pane>>& Chart::panes() const {
  return panes_;
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
    ctx.pane_bounds = pane->bounds();
    ctx.x_scale = x_scale_.get();
    ctx.y_scale = pane->y_scale();
    ctx.viewport = &viewport_;

    canvas.save();
    canvas.clipRect(pane->bounds());

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
}

void Chart::LayoutPanes() {
  if (panes_.empty() || bounds_.isEmpty()) {
    return;
  }

  float total_weight = 0.0f;
  for (const auto& pane : panes_) {
    total_weight += pane->height_weight();
  }
  total_weight = std::max(total_weight, 0.01f);

  float current_top = bounds_.top();
  const float available_height = bounds_.height();
  for (size_t index = 0; index < panes_.size(); ++index) {
    const bool is_last = index + 1 == panes_.size();
    const float pane_height = is_last
        ? (bounds_.bottom() - current_top)
        : (available_height * (panes_[index]->height_weight() / total_weight));
    panes_[index]->SetBounds(
        SkRect::MakeLTRB(bounds_.left(), current_top, bounds_.right(), current_top + pane_height));
    current_top += pane_height;
  }
}

void Chart::SyncScales() {
  LayoutPanes();

  if (x_scale_ != nullptr) {
    x_scale_->SetViewport(viewport_);
    x_scale_->SetBounds(bounds_);
  }

  for (const auto& pane : panes_) {
    pane->UpdateAutoRange(viewport_);
  }
}

}  // namespace kairo
