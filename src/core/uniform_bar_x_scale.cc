#include "uniform_bar_x_scale.h"

#include <algorithm>

namespace kairo {

UniformBarXScale::UniformBarXScale() = default;

void UniformBarXScale::SetViewport(const Viewport& viewport) {
  viewport_ = viewport;
  UpdateMetrics();
}

const Viewport& UniformBarXScale::viewport() const {
  return viewport_;
}

void UniformBarXScale::SetBounds(const SkRect& bounds) {
  bounds_ = bounds;
  UpdateMetrics();
}

const SkRect& UniformBarXScale::bounds() const {
  return bounds_;
}

float UniformBarXScale::DataToScreen(double logical_x) const {
  return bounds_.left() +
         static_cast<float>((logical_x - viewport_.visible_from + 0.5) * bar_spacing_);
}

double UniformBarXScale::ScreenToData(float screen_x) const {
  if (bar_spacing_ <= 0.0f) {
    return viewport_.visible_from;
  }

  return viewport_.visible_from + ((screen_x - bounds_.left()) / bar_spacing_) - 0.5;
}

float UniformBarXScale::BarSpacing() const {
  return bar_spacing_;
}

void UniformBarXScale::UpdateMetrics() {
  const double viewport_width = std::max(viewport_.width(), 1.0);
  const float width = bounds_.width();
  bar_spacing_ = width > 0.0f ? (width / static_cast<float>(viewport_width)) : 1.0f;
}

}  // namespace kairo
