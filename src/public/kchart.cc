#include "kchart.h"

#include <algorithm>
#include <cmath>
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

void KChart::ResetToDefaultPriceVolumeLayout() {
  StopScrollState();
  model_.series_specs.clear();

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
  volume.placement.pane_height = model_.options.default_sub_pane_height;

  model_.series_specs.push_back(price);
  model_.series_specs.push_back(volume);
  RebuildGraph();
}

bool KChart::AddSeries(const KSeriesSpec& spec) {
  if (spec.id.empty()) {
    return false;
  }

  StopScrollState();
  auto it = std::find_if(
      model_.series_specs.begin(), model_.series_specs.end(), [&](const KSeriesSpec& current) {
        return current.id == spec.id;
      });
  if (it != model_.series_specs.end()) {
    *it = spec;
  } else {
    model_.series_specs.push_back(spec);
  }

  RebuildGraph();
  return true;
}

bool KChart::RemoveSeries(const std::string& series_id) {
  StopScrollState();
  const auto before_size = model_.series_specs.size();
  model_.series_specs.erase(
      std::remove_if(
          model_.series_specs.begin(),
          model_.series_specs.end(),
          [&](const KSeriesSpec& spec) { return spec.id == series_id; }),
      model_.series_specs.end());

  if (model_.series_specs.size() == before_size) {
    return false;
  }

  RebuildGraph();
  return true;
}

bool KChart::HasSeries(const std::string& series_id) const {
  return std::any_of(
      model_.series_specs.begin(),
      model_.series_specs.end(),
      [&](const KSeriesSpec& spec) { return spec.id == series_id; });
}

const std::vector<KSeriesSpec>& KChart::series_specs() const {
  return model_.series_specs;
}

void KChart::SetData(std::vector<KCandleEntry> candles) {
  StopScrollState();
  model_.candles = std::move(candles);
  if (!model_.has_explicit_visible_range) {
    const double count = static_cast<double>(model_.candles.size());
    if (count <= 0.0) {
      model_.visible_range = KVisibleRange {0.0, 24.0};
    } else {
      model_.visible_range = KVisibleRange {std::max(0.0, count - 24.0), count};
    }
  }

  RebuildGraph();
}

const std::vector<KCandleEntry>& KChart::data() const {
  return model_.candles;
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

void KChart::SetScrollInteractionOptions(const KScrollInteractionOptions& options) {
  StopScrollState();
  model_.scroll_options = options;
  interaction_.scroll_controller.SetOptions(ToCoreScrollOptions(options));
}

const KScrollInteractionOptions& KChart::scroll_interaction_options() const {
  return model_.scroll_options;
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
  const double position = interaction_.scroll_controller.ApplyGestureDelta(-static_cast<double>(delta_pixels), metrics);
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

void KChart::RebuildGraph() {
  std::vector<CandleData> core_candles;
  core_candles.reserve(model_.candles.size());
  for (const KCandleEntry& candle : model_.candles) {
    core_candles.push_back(ToCoreCandle(candle));
  }

  graph_.data_source = std::make_shared<VectorCandleDataSource>(std::move(core_candles));
  graph_.chart = std::make_unique<Chart>();
  graph_.chart->SetContentInsets(model_.options.content_insets);
  graph_.chart->SetViewport(Viewport {model_.visible_range.from, model_.visible_range.to});

  graph_.pane_bindings.clear();
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
      layout.height =
          placement.pane_height > 0.0f ? placement.pane_height : model_.options.default_sub_pane_height;
      layout.insets.top = 4.0f;
    }

    auto pane = std::make_unique<Pane>(layout);
    pane->AddLayer(std::make_unique<GridLayer>());
    Pane* pane_ptr = graph_.chart->AddPane(std::move(pane));
    pane_index_map[pane_id] = graph_.pane_bindings.size();
    graph_.pane_bindings.push_back(PaneBinding {pane_id, {}});
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

  for (const KSeriesSpec& spec : model_.series_specs) {
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
        auto series = std::make_unique<CandleSeries>(graph_.data_source);
        series->SetUpColor(ToSkColor(spec.style.primary_color));
        series->SetDownColor(ToSkColor(spec.style.secondary_color));
        series->SetWickColor(ToSkColor(spec.style.tertiary_color));
        pane->AddSeries(std::move(series));
        break;
      }
      case KSeriesType::kVolume: {
        auto series = std::make_unique<VolumeSeries>(graph_.data_source);
        series->SetUpColor(ToSkColor(spec.style.primary_color));
        series->SetDownColor(ToSkColor(spec.style.secondary_color));
        pane->AddSeries(std::move(series));
        break;
      }
    }

    series_to_pane_map[spec.id] = pane_id;
    auto index_it = pane_index_map.find(pane_id);
    if (index_it != pane_index_map.end()) {
      graph_.pane_bindings[index_it->second].series_types.push_back(spec.type);
    }
  }

  graph_.crosshair_overlay = nullptr;
  if (model_.options.crosshair_enabled) {
    auto crosshair = std::make_unique<CrosshairOverlay>();
    crosshair->SetVisible(false);
    graph_.crosshair_overlay = crosshair.get();
    graph_.chart->AddOverlay(std::move(crosshair));
  }

  UpdateBounds();
}

void KChart::UpdateBounds() {
  if (graph_.chart == nullptr) {
    return;
  }

  graph_.chart->SetBounds(SkRect::MakeWH(interaction_.width, interaction_.height));
  graph_.chart->SetViewport(Viewport {model_.visible_range.from, model_.visible_range.to});
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

  const double max_from = std::max(0.0, static_cast<double>(model_.candles.size()) - ViewportWidth());
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
  if (!model_.options.crosshair_enabled || graph_.chart == nullptr || graph_.crosshair_overlay == nullptr ||
      model_.candles.empty()) {
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
  const float bar_spacing = bounds.width() > 0.0f
      ? (bounds.width() / static_cast<float>(width_value))
      : 1.0f;
  if (bar_spacing <= 0.0f) {
    return model_.visible_range.from;
  }

  return model_.visible_range.from + ((screen_x - bounds.left()) / bar_spacing) - 0.5;
}

}  // namespace kairo
