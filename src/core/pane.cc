#include "pane.h"

#include <algorithm>

#include "linear_y_scale.h"

namespace kairo {

Pane::Pane() : Pane(PaneLayout{}, std::make_unique<LinearYScale>()) {}

Pane::Pane(std::unique_ptr<YScale> y_scale) : Pane(PaneLayout{}, std::move(y_scale)) {}

Pane::Pane(const PaneLayout& layout) : Pane(layout, std::make_unique<LinearYScale>()) {}

Pane::Pane(const PaneLayout& layout, std::unique_ptr<YScale> y_scale)
    : y_scale_(std::move(y_scale)) {
  SetLayout(layout);
}

Pane::~Pane() = default;

void Pane::SetLayout(const PaneLayout& layout) {
  layout_ = layout;
  layout_.height = std::max(layout.height, 0.0f);
}

const PaneLayout& Pane::layout() const {
  return layout_;
}

void Pane::SetFrameRect(const SkRect& rect) {
  frame_rect_ = rect;
}

const SkRect& Pane::frame_rect() const {
  return frame_rect_;
}

void Pane::SetContentRect(const SkRect& rect) {
  content_rect_ = rect;
  if (y_scale_ != nullptr) {
    y_scale_->SetBounds(content_rect_);
  }
}

const SkRect& Pane::content_rect() const {
  return content_rect_;
}

YScale* Pane::y_scale() {
  return y_scale_.get();
}

const YScale* Pane::y_scale() const {
  return y_scale_.get();
}

void Pane::AddSeries(std::unique_ptr<Series> series) {
  if (series != nullptr) {
    series_.push_back(std::move(series));
  }
}

void Pane::AddLayer(std::unique_ptr<Layer> layer) {
  if (layer != nullptr) {
    layers_.push_back(std::move(layer));
  }
}

std::vector<std::unique_ptr<Series>>& Pane::series() {
  return series_;
}

const std::vector<std::unique_ptr<Series>>& Pane::series() const {
  return series_;
}

std::vector<std::unique_ptr<Layer>>& Pane::layers() {
  return layers_;
}

const std::vector<std::unique_ptr<Layer>>& Pane::layers() const {
  return layers_;
}

void Pane::UpdateAutoRange(const Viewport& viewport) {
  Range visible_range;
  for (const auto& series : series_) {
    visible_range.Include(series->GetVisibleRange(viewport));
  }

  if (!visible_range.IsValid()) {
    visible_range = Range(0.0, 1.0);
  }

  if (y_scale_ != nullptr) {
    y_scale_->SetBounds(content_rect_);
    y_scale_->SetRange(visible_range.Expanded(0.08, 1.0));
  }
}

}  // namespace kairo
