#include "pane.h"

#include <algorithm>

namespace kairo {

Pane::Pane() : Pane(PaneLayout{}) {}

Pane::Pane(const PaneLayout& layout) {
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
  y_scale_.SetBounds(content_rect_);
}

const SkRect& Pane::content_rect() const {
  return content_rect_;
}

void Pane::SetBackgroundColor(SkColor color) {
  background_color_ = color;
}

SkColor Pane::background_color() const {
  return background_color_;
}

LinearYScale* Pane::y_scale() {
  return &y_scale_;
}

const LinearYScale* Pane::y_scale() const {
  return &y_scale_;
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

  if (visible_range.span() <= 0.0) {
    visible_range = visible_range.Expanded(0.0, 1.0);
  }

  y_scale_.SetBounds(content_rect_);
  y_scale_.SetRange(visible_range);
}

}  // namespace kairo
