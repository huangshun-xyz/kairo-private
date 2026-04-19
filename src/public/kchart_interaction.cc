#include "kchart.h"

#include <algorithm>
#include <cmath>

#include "chart.h"
#include "chart_controller.h"
#include "crosshair_overlay.h"

namespace kairo {

void KChart::ScrollBy(double delta_logical) {
  StopScrollState();
  model_.has_explicit_visible_range = true;
  if (graph_.chart != nullptr && graph_.chart->controller() != nullptr) {
    graph_.chart->controller()->ScrollBy(delta_logical);
    const Viewport& viewport = graph_.chart->viewport();
    model_.visible_range = KVisibleRange {viewport.visible_from, viewport.visible_to};
  }
}

void KChart::ZoomBy(double scale_factor, float anchor_ratio) {
  StopScrollState();
  model_.has_explicit_visible_range = true;
  if (graph_.chart == nullptr || graph_.chart->controller() == nullptr) {
    return;
  }

  const SkRect& content_bounds = graph_.chart->content_bounds();
  const float clamped_ratio = std::clamp(anchor_ratio, 0.0f, 1.0f);
  const float anchor_x = content_bounds.left() + (content_bounds.width() * clamped_ratio);
  const double logical_anchor = ScreenToLogicalX(anchor_x);
  graph_.chart->controller()->ZoomBy(scale_factor, logical_anchor);

  const Viewport& viewport = graph_.chart->viewport();
  model_.visible_range = KVisibleRange {viewport.visible_from, viewport.visible_to};
}

void KChart::BeginScrollGestureInternal() {
  interaction_.scroll_gesture_active = true;
  interaction_.scroll_controller.BeginGesture(ScrollPositionInputUnits());
}

bool KChart::ScrollGestureByPixelsInternal(float delta_pixels) {
  if (!interaction_.scroll_gesture_active) {
    BeginScrollGestureInternal();
  }

  const ScrollMetrics metrics = BuildScrollMetrics();
  const double position =
      interaction_.scroll_controller.ApplyGestureDelta(-static_cast<double>(delta_pixels), metrics);
  model_.has_explicit_visible_range = true;
  return SetScrollPositionInputUnits(position);
}

bool KChart::EndScrollGestureInternal(float velocity_pixels_per_second) {
  if (!interaction_.scroll_gesture_active) {
    BeginScrollGestureInternal();
  }

  interaction_.scroll_gesture_active = false;
  const ScrollMetrics metrics = BuildScrollMetrics();
  const bool active = interaction_.scroll_controller.EndGesture(
      -static_cast<double>(velocity_pixels_per_second), metrics);
  model_.has_explicit_visible_range = true;
  SetScrollPositionInputUnits(interaction_.scroll_controller.position());
  return active;
}

bool KChart::StepScrollAnimationInternal(double delta_seconds) {
  const ScrollMetrics metrics = BuildScrollMetrics();
  const bool active = interaction_.scroll_controller.Advance(delta_seconds, metrics);
  if (!active && !interaction_.scroll_controller.IsActive()) {
    interaction_.scroll_gesture_active = false;
  }
  model_.has_explicit_visible_range = true;
  return SetScrollPositionInputUnits(interaction_.scroll_controller.position()) && active;
}

bool KChart::IsScrollAnimationActiveInternal() const {
  return interaction_.scroll_controller.IsActive();
}

void KChart::StopScrollAnimationInternal() {
  StopScrollState();
}

double KChart::ViewportWidth() const {
  return std::max(model_.visible_range.to - model_.visible_range.from, 1.0);
}

double KChart::InteractionContentWidth() const {
  if (graph_.chart == nullptr) {
    return 0.0;
  }

  const float content_width = graph_.chart->content_bounds().width();
  if (content_width <= 0.0f) {
    return 0.0;
  }

  return static_cast<double>(content_width) / std::max(interaction_.device_scale_factor, 1.0f);
}

double KChart::BarSpacingInputUnits() const {
  return InteractionContentWidth() / ViewportWidth();
}

double KChart::ScrollPositionInputUnits() const {
  return model_.visible_range.from * BarSpacingInputUnits();
}

ScrollMetrics KChart::BuildScrollMetrics() const {
  ScrollMetrics metrics;
  metrics.viewport_dimension = InteractionContentWidth();

  const double bar_spacing = BarSpacingInputUnits();
  if (bar_spacing <= 0.0) {
    return metrics;
  }

  const double max_from =
      std::max(0.0, static_cast<double>(model_.candles.size()) - ViewportWidth());
  metrics.min_position = 0.0;
  metrics.max_position = max_from * bar_spacing;
  return metrics;
}

bool KChart::SetScrollPositionInputUnits(double position_input_units) {
  const double bar_spacing = BarSpacingInputUnits();
  if (graph_.chart == nullptr || bar_spacing <= 0.0) {
    return false;
  }

  const double viewport_width = ViewportWidth();
  model_.visible_range.from = position_input_units / bar_spacing;
  model_.visible_range.to = model_.visible_range.from + viewport_width;
  graph_.chart->SetViewport(Viewport {model_.visible_range.from, model_.visible_range.to});
  return true;
}

void KChart::StopScrollState() {
  interaction_.scroll_gesture_active = false;
  interaction_.scroll_controller.Stop();
}

void KChart::UpdateCrosshairAt(float screen_x, float screen_y) {
  if (!model_.options.crosshair_enabled || graph_.chart == nullptr ||
      graph_.crosshair_overlay == nullptr || model_.candles.empty()) {
    return;
  }

  const std::optional<std::size_t> pane_index = FindPaneIndex(screen_y);
  if (!pane_index.has_value()) {
    graph_.crosshair_overlay->SetVisible(false);
    return;
  }

  const double logical_x = std::clamp(
      std::round(ScreenToLogicalX(screen_x)), 0.0, static_cast<double>(model_.candles.size() - 1));
  const KCandleEntry& candle = model_.candles[static_cast<std::size_t>(logical_x)];

  double crosshair_value = candle.close;
  const PaneBinding& binding = graph_.pane_bindings[*pane_index];
  if (std::find(binding.series_types.begin(), binding.series_types.end(), KSeriesType::kVolume) !=
      binding.series_types.end()) {
    crosshair_value = candle.volume;
  }

  graph_.crosshair_overlay->SetVisible(true);
  graph_.crosshair_overlay->SetCrosshair(*pane_index, logical_x, crosshair_value);
}

void KChart::ClearCrosshairInternal() {
  if (graph_.crosshair_overlay != nullptr) {
    graph_.crosshair_overlay->SetVisible(false);
  }
}

std::optional<std::size_t> KChart::FindPaneIndex(float screen_y) const {
  if (graph_.chart == nullptr) {
    return std::nullopt;
  }

  const auto& panes = graph_.chart->panes();
  for (std::size_t index = 0; index < panes.size(); ++index) {
    const Pane* pane = panes[index].get();
    if (pane == nullptr) {
      continue;
    }

    const SkRect& rect = pane->content_rect();
    if (screen_y >= rect.top() && screen_y <= rect.bottom()) {
      return index;
    }
  }

  return std::nullopt;
}

double KChart::ScreenToLogicalX(float screen_x) const {
  if (graph_.chart == nullptr) {
    return model_.visible_range.from;
  }

  const SkRect& bounds = graph_.chart->content_bounds();
  const double width_value = std::max(model_.visible_range.to - model_.visible_range.from, 1.0);
  const float bar_spacing =
      bounds.width() > 0.0f ? (bounds.width() / static_cast<float>(width_value)) : 1.0f;
  if (bar_spacing <= 0.0f) {
    return model_.visible_range.from;
  }

  return model_.visible_range.from + ((screen_x - bounds.left()) / bar_spacing) - 0.5;
}

}  // namespace kairo
