#include "kchart.h"

#include <algorithm>

#include "chart.h"

namespace kairo {

namespace {

ScrollBoundaryBehavior ToCoreBoundaryBehavior(KScrollBoundaryBehavior behavior) {
  switch (behavior) {
    case KScrollBoundaryBehavior::kClamp:
      return ScrollBoundaryBehavior::kClamp;
    case KScrollBoundaryBehavior::kBounce:
      return ScrollBoundaryBehavior::kBounce;
  }
  return ScrollBoundaryBehavior::kBounce;
}

ScrollInteractionOptions ToCoreScrollOptions(const KScrollInteractionOptions& options) {
  ScrollInteractionOptions core_options;
  core_options.boundary_behavior = ToCoreBoundaryBehavior(options.boundary_behavior);
  core_options.always_scrollable = options.always_scrollable;
  core_options.allow_inertia = options.allow_inertia;
  core_options.drag_sensitivity = options.drag_sensitivity;
  core_options.overscroll_friction = options.overscroll_friction;
  core_options.deceleration_rate = options.deceleration_rate;
  core_options.spring.mass = options.spring.mass;
  core_options.spring.stiffness = options.spring.stiffness;
  core_options.spring.damping_ratio = options.spring.damping_ratio;
  return core_options;
}

}  // namespace

KChart::KChart() {
  interaction_.scroll_controller.SetOptions(ToCoreScrollOptions(model_.scroll_options));
  ResetToDefaultPriceVolumeLayout();
}

KChart::~KChart() = default;

KChart::KChart(KChart&&) noexcept = default;

KChart& KChart::operator=(KChart&&) noexcept = default;

void KChart::SetOptions(const KChartOptions& options) {
  StopScrollState();
  model_.options = options;
  for (KSeriesSpec& spec : model_.series_specs) {
    if (spec.type == KSeriesType::kVolume &&
        spec.placement.kind == KPlacementKind::kNewPane &&
        spec.placement.pane_height <= 0.0f) {
      spec.placement.pane_height = model_.options.default_sub_pane_height;
    }
  }
  RebuildGraph();
}

const KChartOptions& KChart::options() const {
  return model_.options;
}

void KChart::SetBounds(float width, float height) {
  interaction_.width = std::max(width, 0.0f);
  interaction_.height = std::max(height, 0.0f);
  UpdateBounds();
}

void KChart::SetDeviceScaleFactor(float scale) {
  interaction_.device_scale_factor = std::max(scale, 1.0f);
}

float KChart::device_scale_factor() const {
  return interaction_.device_scale_factor;
}

void KChart::SetVisibleRange(const KVisibleRange& range) {
  if (!range.IsValid()) {
    return;
  }

  StopScrollState();
  model_.visible_range = range;
  model_.has_explicit_visible_range = true;
  if (graph_.chart != nullptr) {
    graph_.chart->SetViewport(Viewport {range.from, range.to});
  }
}

KVisibleRange KChart::visible_range() const {
  return model_.visible_range;
}

void KChart::SetScrollInteractionOptions(const KScrollInteractionOptions& options) {
  StopScrollState();
  model_.scroll_options = options;
  interaction_.scroll_controller.SetOptions(ToCoreScrollOptions(options));
}

const KScrollInteractionOptions& KChart::scroll_interaction_options() const {
  return model_.scroll_options;
}

void KChart::SetCrosshairEnabled(bool enabled) {
  model_.options.crosshair_enabled = enabled;
  if (!enabled) {
    ClearCrosshairInternal();
  }
}

bool KChart::crosshair_enabled() const {
  return model_.options.crosshair_enabled;
}

void KChart::UpdateCrosshair(float screen_x, float screen_y) {
  UpdateCrosshairAt(screen_x, screen_y);
}

void KChart::ClearCrosshair() {
  ClearCrosshairInternal();
}

void KChart::Draw(SkCanvas& canvas) {
  if (graph_.chart == nullptr) {
    return;
  }

  UpdateBounds();
  graph_.chart->Draw(canvas);
}

}  // namespace kairo
