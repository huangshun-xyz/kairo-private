#include "pane.h"

#include <algorithm>

#include "linear_y_scale.h"

namespace kairo {

Pane::Pane() : Pane(std::make_unique<LinearYScale>()) {}

Pane::Pane(std::unique_ptr<YScale> y_scale) : y_scale_(std::move(y_scale)) {}

Pane::~Pane() = default;

void Pane::SetBounds(const SkRect& bounds) {
  bounds_ = bounds;
  if (y_scale_ != nullptr) {
    y_scale_->SetBounds(bounds_);
  }
}

const SkRect& Pane::bounds() const {
  return bounds_;
}

void Pane::SetHeightWeight(float weight) {
  height_weight_ = std::max(weight, 0.01f);
}

float Pane::height_weight() const {
  return height_weight_;
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
    y_scale_->SetBounds(bounds_);
    y_scale_->SetRange(visible_range.Expanded(0.08, 1.0));
  }
}

}  // namespace kairo
