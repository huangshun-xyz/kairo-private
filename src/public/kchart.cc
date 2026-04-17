#include "kchart.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "candle_data_source.h"
#include "candle_series.h"
#include "chart.h"
#include "chart_controller.h"
#include "crosshair_overlay.h"
#include "grid_layer.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "pane.h"
#include "scroll_interaction.h"
#include "volume_series.h"

namespace kairo {

namespace {

SkColor ToSkColor(KColor color) {
  return static_cast<SkColor>(color);
}

CandleData ToCoreCandle(const KCandleEntry& candle) {
  return CandleData {candle.open, candle.high, candle.low, candle.close, candle.volume};
}

std::string DefaultPaneIdForSeries(const KSeriesSpec& spec) {
  if (!spec.placement.pane_id.empty()) {
    return spec.placement.pane_id;
  }

  if (spec.type == KSeriesType::kVolume) {
    return "volume";
  }

  return "main";
}

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

struct KChart::Impl {
  struct PaneBinding {
    std::string pane_id;
    std::vector<KSeriesType> series_types;
  };

  KChartOptions options;
  KScrollInteractionOptions scroll_options;
  std::vector<KCandleEntry> candles;
  std::vector<KSeriesSpec> series_specs;
  KVisibleRange visible_range;
  bool has_explicit_visible_range = false;
  bool scroll_gesture_active = false;
  float device_scale_factor = 1.0f;
  float width = 0.0f;
  float height = 0.0f;
  std::shared_ptr<VectorCandleDataSource> data_source;
  std::unique_ptr<Chart> chart;
  CrosshairOverlay* crosshair_overlay = nullptr;
  std::vector<PaneBinding> pane_bindings;
  ScrollInteractionController scroll_controller;

  void RebuildGraph();
  void UpdateBounds();
  void UpdateCrosshairAt(float screen_x, float screen_y);
  void ClearCrosshair();
  std::optional<std::size_t> FindPaneIndex(float screen_y) const;
  double ScreenToLogicalX(float screen_x) const;
  double ViewportWidth() const;
  double InteractionContentWidth() const;
  double BarSpacingInputUnits() const;
  double ScrollPositionInputUnits() const;
  ScrollMetrics BuildScrollMetrics() const;
  bool SetScrollPositionInputUnits(double position_input_units);
  void StopScrollState();
};

KChart::KChart() : impl_(std::make_unique<Impl>()) {
  impl_->scroll_controller.SetOptions(ToCoreScrollOptions(impl_->scroll_options));
  ResetToDefaultPriceVolumeLayout();
}

KChart::~KChart() = default;

KChart::KChart(KChart&&) noexcept = default;

KChart& KChart::operator=(KChart&&) noexcept = default;

void KChart::SetOptions(const KChartOptions& options) {
  impl_->StopScrollState();
  impl_->options = options;
  for (KSeriesSpec& spec : impl_->series_specs) {
    if (spec.type == KSeriesType::kVolume &&
        spec.placement.kind == KPlacementKind::kNewPane &&
        spec.placement.pane_height <= 0.0f) {
      spec.placement.pane_height = impl_->options.default_sub_pane_height;
    }
  }
  impl_->RebuildGraph();
}

const KChartOptions& KChart::options() const {
  return impl_->options;
}

void KChart::ResetToDefaultPriceVolumeLayout() {
  impl_->StopScrollState();
  impl_->series_specs.clear();

  KSeriesSpec price;
  price.id = "price";
  price.title = "Price";
  price.type = KSeriesType::kCandles;
  price.placement.kind = KPlacementKind::kMainPane;

  KSeriesSpec volume;
  volume.id = "volume";
  volume.title = "Volume";
  volume.type = KSeriesType::kVolume;
  volume.placement.kind = KPlacementKind::kNewPane;
  volume.placement.pane_id = "volume";
  volume.placement.pane_height = impl_->options.default_sub_pane_height;

  impl_->series_specs.push_back(price);
  impl_->series_specs.push_back(volume);
  impl_->RebuildGraph();
}

bool KChart::AddSeries(const KSeriesSpec& spec) {
  if (spec.id.empty()) {
    return false;
  }

  impl_->StopScrollState();
  auto it = std::find_if(
      impl_->series_specs.begin(), impl_->series_specs.end(), [&](const KSeriesSpec& current) {
        return current.id == spec.id;
      });
  if (it != impl_->series_specs.end()) {
    *it = spec;
  } else {
    impl_->series_specs.push_back(spec);
  }

  impl_->RebuildGraph();
  return true;
}

bool KChart::RemoveSeries(const std::string& series_id) {
  impl_->StopScrollState();
  const auto before_size = impl_->series_specs.size();
  impl_->series_specs.erase(
      std::remove_if(
          impl_->series_specs.begin(),
          impl_->series_specs.end(),
          [&](const KSeriesSpec& spec) { return spec.id == series_id; }),
      impl_->series_specs.end());

  if (impl_->series_specs.size() == before_size) {
    return false;
  }

  impl_->RebuildGraph();
  return true;
}

bool KChart::HasSeries(const std::string& series_id) const {
  return std::any_of(
      impl_->series_specs.begin(),
      impl_->series_specs.end(),
      [&](const KSeriesSpec& spec) { return spec.id == series_id; });
}

const std::vector<KSeriesSpec>& KChart::series_specs() const {
  return impl_->series_specs;
}

void KChart::SetData(std::vector<KCandleEntry> candles) {
  impl_->StopScrollState();
  impl_->candles = std::move(candles);
  if (!impl_->has_explicit_visible_range) {
    const double count = static_cast<double>(impl_->candles.size());
    if (count <= 0.0) {
      impl_->visible_range = KVisibleRange {0.0, 24.0};
    } else {
      impl_->visible_range = KVisibleRange {std::max(0.0, count - 24.0), count};
    }
  }

  impl_->RebuildGraph();
}

const std::vector<KCandleEntry>& KChart::data() const {
  return impl_->candles;
}

void KChart::SetBounds(float width, float height) {
  impl_->width = std::max(width, 0.0f);
  impl_->height = std::max(height, 0.0f);
  impl_->UpdateBounds();
}

void KChart::SetDeviceScaleFactor(float scale) {
  impl_->device_scale_factor = std::max(scale, 1.0f);
}

float KChart::device_scale_factor() const {
  return impl_->device_scale_factor;
}

void KChart::SetVisibleRange(const KVisibleRange& range) {
  if (!range.IsValid()) {
    return;
  }

  impl_->StopScrollState();
  impl_->visible_range = range;
  impl_->has_explicit_visible_range = true;
  if (impl_->chart != nullptr) {
    impl_->chart->SetViewport(Viewport {range.from, range.to});
  }
}

KVisibleRange KChart::visible_range() const {
  return impl_->visible_range;
}

void KChart::ScrollBy(double delta_logical) {
  impl_->StopScrollState();
  impl_->has_explicit_visible_range = true;
  if (impl_->chart != nullptr && impl_->chart->controller() != nullptr) {
    impl_->chart->controller()->ScrollBy(delta_logical);
    const Viewport& viewport = impl_->chart->viewport();
    impl_->visible_range = KVisibleRange {viewport.visible_from, viewport.visible_to};
  }
}

void KChart::ZoomBy(double scale_factor, float anchor_ratio) {
  impl_->StopScrollState();
  impl_->has_explicit_visible_range = true;
  if (impl_->chart == nullptr || impl_->chart->controller() == nullptr) {
    return;
  }

  const SkRect& content_bounds = impl_->chart->content_bounds();
  const float clamped_ratio = std::clamp(anchor_ratio, 0.0f, 1.0f);
  const float anchor_x = content_bounds.left() + (content_bounds.width() * clamped_ratio);
  const double logical_anchor = impl_->ScreenToLogicalX(anchor_x);
  impl_->chart->controller()->ZoomBy(scale_factor, logical_anchor);

  const Viewport& viewport = impl_->chart->viewport();
  impl_->visible_range = KVisibleRange {viewport.visible_from, viewport.visible_to};
}

void KChart::SetScrollInteractionOptions(const KScrollInteractionOptions& options) {
  impl_->StopScrollState();
  impl_->scroll_options = options;
  impl_->scroll_controller.SetOptions(ToCoreScrollOptions(options));
}

const KScrollInteractionOptions& KChart::scroll_interaction_options() const {
  return impl_->scroll_options;
}

void KChart::BeginScrollGesture() {
  impl_->scroll_gesture_active = true;
  impl_->scroll_controller.BeginGesture(impl_->ScrollPositionInputUnits());
}

bool KChart::ScrollGestureByPixels(float delta_pixels) {
  if (!impl_->scroll_gesture_active) {
    BeginScrollGesture();
  }

  const ScrollMetrics metrics = impl_->BuildScrollMetrics();
  const double position = impl_->scroll_controller.ApplyGestureDelta(-static_cast<double>(delta_pixels), metrics);
  impl_->has_explicit_visible_range = true;
  return impl_->SetScrollPositionInputUnits(position);
}

bool KChart::EndScrollGesture(float velocity_pixels_per_second) {
  if (!impl_->scroll_gesture_active) {
    BeginScrollGesture();
  }

  impl_->scroll_gesture_active = false;
  const ScrollMetrics metrics = impl_->BuildScrollMetrics();
  const bool active = impl_->scroll_controller.EndGesture(
      -static_cast<double>(velocity_pixels_per_second), metrics);
  impl_->has_explicit_visible_range = true;
  impl_->SetScrollPositionInputUnits(impl_->scroll_controller.position());
  return active;
}

bool KChart::StepScrollAnimation(double delta_seconds) {
  const ScrollMetrics metrics = impl_->BuildScrollMetrics();
  const bool active = impl_->scroll_controller.Advance(delta_seconds, metrics);
  if (!active && !impl_->scroll_controller.IsActive()) {
    impl_->scroll_gesture_active = false;
  }
  impl_->has_explicit_visible_range = true;
  return impl_->SetScrollPositionInputUnits(impl_->scroll_controller.position()) && active;
}

bool KChart::IsScrollAnimationActive() const {
  return impl_->scroll_controller.IsActive();
}

void KChart::StopScrollAnimation() {
  impl_->StopScrollState();
}

void KChart::SetCrosshairEnabled(bool enabled) {
  impl_->options.crosshair_enabled = enabled;
  if (!enabled) {
    impl_->ClearCrosshair();
  }
}

bool KChart::crosshair_enabled() const {
  return impl_->options.crosshair_enabled;
}

void KChart::UpdateCrosshair(float screen_x, float screen_y) {
  impl_->UpdateCrosshairAt(screen_x, screen_y);
}

void KChart::ClearCrosshair() {
  impl_->ClearCrosshair();
}

void KChart::Draw(SkCanvas& canvas) {
  if (impl_->chart == nullptr) {
    return;
  }

  impl_->UpdateBounds();
  impl_->chart->Draw(canvas);
}

void KChart::Impl::RebuildGraph() {
  std::vector<CandleData> core_candles;
  core_candles.reserve(candles.size());
  for (const KCandleEntry& candle : candles) {
    core_candles.push_back(ToCoreCandle(candle));
  }

  data_source = std::make_shared<VectorCandleDataSource>(std::move(core_candles));
  chart = std::make_unique<Chart>();
  chart->SetContentInsets(options.content_insets);
  chart->SetViewport(Viewport {visible_range.from, visible_range.to});

  pane_bindings.clear();
  std::unordered_map<std::string, Pane*> pane_map;
  std::unordered_map<std::string, std::size_t> pane_index_map;
  std::unordered_map<std::string, std::string> series_to_pane_map;

  auto ensure_pane = [&](const std::string& pane_id, const KPanePlacement& placement) -> Pane* {
    auto pane_it = pane_map.find(pane_id);
    if (pane_it != pane_map.end()) {
      return pane_it->second;
    }

    PaneLayout layout;
    if (pane_id == "main") {
      layout.size_mode = PaneSizeMode::kStretch;
      layout.insets.bottom = 4.0f;
    } else {
      layout.size_mode = PaneSizeMode::kFixed;
      layout.height = placement.pane_height > 0.0f ? placement.pane_height : options.default_sub_pane_height;
      layout.insets.top = 4.0f;
    }

    auto pane = std::make_unique<Pane>(layout);
    pane->AddLayer(std::make_unique<GridLayer>());
    Pane* pane_ptr = chart->AddPane(std::move(pane));
    pane_index_map[pane_id] = pane_bindings.size();
    pane_bindings.push_back(PaneBinding {pane_id, {}});
    pane_map[pane_id] = pane_ptr;
    return pane_ptr;
  };

  auto resolve_pane_id = [&](const KSeriesSpec& spec) {
    switch (spec.placement.kind) {
      case KPlacementKind::kMainPane:
        return std::string("main");
      case KPlacementKind::kNewPane:
        return DefaultPaneIdForSeries(spec);
      case KPlacementKind::kExistingPane:
        return spec.placement.pane_id.empty() ? DefaultPaneIdForSeries(spec) : spec.placement.pane_id;
      case KPlacementKind::kOverlayOnSeries: {
        auto source_it = series_to_pane_map.find(spec.placement.target_series_id);
        if (source_it != series_to_pane_map.end()) {
          return source_it->second;
        }
        return std::string("main");
      }
    }
    return std::string("main");
  };

  for (const KSeriesSpec& spec : series_specs) {
    if (!spec.visible) {
      continue;
    }

    const std::string pane_id = resolve_pane_id(spec);
    Pane* pane = ensure_pane(pane_id, spec.placement);
    if (pane == nullptr) {
      continue;
    }

    switch (spec.type) {
      case KSeriesType::kCandles: {
        auto series = std::make_unique<CandleSeries>(data_source);
        series->SetUpColor(ToSkColor(spec.style.primary_color));
        series->SetDownColor(ToSkColor(spec.style.secondary_color));
        series->SetWickColor(ToSkColor(spec.style.tertiary_color));
        pane->AddSeries(std::move(series));
        break;
      }
      case KSeriesType::kVolume: {
        auto series = std::make_unique<VolumeSeries>(data_source);
        series->SetUpColor(ToSkColor(spec.style.primary_color));
        series->SetDownColor(ToSkColor(spec.style.secondary_color));
        pane->AddSeries(std::move(series));
        break;
      }
    }

    series_to_pane_map[spec.id] = pane_id;
    auto index_it = pane_index_map.find(pane_id);
    if (index_it != pane_index_map.end()) {
      pane_bindings[index_it->second].series_types.push_back(spec.type);
    }
  }

  crosshair_overlay = nullptr;
  if (options.crosshair_enabled) {
    auto crosshair = std::make_unique<CrosshairOverlay>();
    crosshair->SetVisible(false);
    crosshair_overlay = crosshair.get();
    chart->AddOverlay(std::move(crosshair));
  }

  UpdateBounds();
}

void KChart::Impl::UpdateBounds() {
  if (chart == nullptr) {
    return;
  }

  chart->SetBounds(SkRect::MakeWH(width, height));
  chart->SetViewport(Viewport {visible_range.from, visible_range.to});
}

double KChart::Impl::ViewportWidth() const {
  return std::max(visible_range.to - visible_range.from, 1.0);
}

double KChart::Impl::InteractionContentWidth() const {
  if (chart == nullptr) {
    return 0.0;
  }

  const float content_width = chart->content_bounds().width();
  if (content_width <= 0.0f) {
    return 0.0;
  }

  return static_cast<double>(content_width) / std::max(device_scale_factor, 1.0f);
}

double KChart::Impl::BarSpacingInputUnits() const {
  return InteractionContentWidth() / ViewportWidth();
}

double KChart::Impl::ScrollPositionInputUnits() const {
  return visible_range.from * BarSpacingInputUnits();
}

ScrollMetrics KChart::Impl::BuildScrollMetrics() const {
  ScrollMetrics metrics;
  metrics.viewport_dimension = InteractionContentWidth();

  const double bar_spacing = BarSpacingInputUnits();
  if (bar_spacing <= 0.0) {
    return metrics;
  }

  const double max_from = std::max(0.0, static_cast<double>(candles.size()) - ViewportWidth());
  metrics.min_position = 0.0;
  metrics.max_position = max_from * bar_spacing;
  return metrics;
}

bool KChart::Impl::SetScrollPositionInputUnits(double position_input_units) {
  const double bar_spacing = BarSpacingInputUnits();
  if (chart == nullptr || bar_spacing <= 0.0) {
    return false;
  }

  const double viewport_width = ViewportWidth();
  visible_range.from = position_input_units / bar_spacing;
  visible_range.to = visible_range.from + viewport_width;
  chart->SetViewport(Viewport {visible_range.from, visible_range.to});
  return true;
}

void KChart::Impl::StopScrollState() {
  scroll_gesture_active = false;
  scroll_controller.Stop();
}

void KChart::Impl::UpdateCrosshairAt(float screen_x, float screen_y) {
  if (!options.crosshair_enabled || chart == nullptr || crosshair_overlay == nullptr || candles.empty()) {
    return;
  }

  const std::optional<std::size_t> pane_index = FindPaneIndex(screen_y);
  if (!pane_index.has_value()) {
    crosshair_overlay->SetVisible(false);
    return;
  }

  const double logical_x = std::clamp(
      std::round(ScreenToLogicalX(screen_x)), 0.0, static_cast<double>(candles.size() - 1));
  const KCandleEntry& candle = candles[static_cast<std::size_t>(logical_x)];

  double crosshair_value = candle.close;
  const PaneBinding& binding = pane_bindings[*pane_index];
  if (std::find(binding.series_types.begin(), binding.series_types.end(), KSeriesType::kVolume) !=
      binding.series_types.end()) {
    crosshair_value = candle.volume;
  }

  crosshair_overlay->SetVisible(true);
  crosshair_overlay->SetCrosshair(*pane_index, logical_x, crosshair_value);
}

void KChart::Impl::ClearCrosshair() {
  if (crosshair_overlay != nullptr) {
    crosshair_overlay->SetVisible(false);
  }
}

std::optional<std::size_t> KChart::Impl::FindPaneIndex(float screen_y) const {
  if (chart == nullptr) {
    return std::nullopt;
  }

  const auto& panes = chart->panes();
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

double KChart::Impl::ScreenToLogicalX(float screen_x) const {
  if (chart == nullptr) {
    return visible_range.from;
  }

  const SkRect& bounds = chart->content_bounds();
  const double width_value = std::max(visible_range.to - visible_range.from, 1.0);
  const float bar_spacing = bounds.width() > 0.0f
      ? (bounds.width() / static_cast<float>(width_value))
      : 1.0f;
  if (bar_spacing <= 0.0f) {
    return visible_range.from;
  }

  return visible_range.from + ((screen_x - bounds.left()) / bar_spacing) - 0.5;
}

}  // namespace kairo
