#include "chart_controller.h"

#include <algorithm>

#include "chart.h"

namespace kairo {

ChartController::ChartController(Chart* chart) : chart_(chart) {}

void ChartController::ScrollBy(double delta_logical) {
  if (chart_ == nullptr) {
    return;
  }

  const Viewport current = chart_->viewport();
  chart_->SetViewport(
      Viewport{current.visible_from + delta_logical, current.visible_to + delta_logical});
}

void ChartController::ZoomBy(double scale_factor, double anchor_logical_x) {
  if (chart_ == nullptr || scale_factor <= 0.0) {
    return;
  }

  const Viewport current = chart_->viewport();
  const double current_width = current.width();
  if (current_width <= 0.0) {
    return;
  }

  const double normalized_anchor =
      (anchor_logical_x - current.visible_from) / std::max(current_width, 1.0);
  const double new_width = std::clamp(current_width / scale_factor, 5.0, 512.0);
  const double new_from = anchor_logical_x - (normalized_anchor * new_width);
  chart_->SetViewport(Viewport{new_from, new_from + new_width});
}

void ChartController::SetVisibleRange(const Viewport& viewport) {
  if (chart_ == nullptr) {
    return;
  }

  chart_->SetViewport(viewport);
}

}  // namespace kairo
