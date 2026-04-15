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

}  // namespace

struct KChart::Impl {
  struct PaneBinding {
    std::string pane_id;
    std::vector<KSeriesType> series_types;
  };

  KChartOptions options;
  std::vector<KCandleEntry> candles;
  std::vector<KSeriesSpec> series_specs;
  KVisibleRange visible_range;
  bool has_explicit_visible_range = false;
  float width = 0.0f;
  float height = 0.0f;
  std::shared_ptr<VectorCandleDataSource> data_source;
  std::unique_ptr<Chart> chart;
  CrosshairOverlay* crosshair_overlay = nullptr;
  std::vector<PaneBinding> pane_bindings;

  void RebuildGraph();
  void UpdateBounds();
  void UpdateCrosshairAt(float screen_x, float screen_y);
  void ClearCrosshair();
  std::optional<std::size_t> FindPaneIndex(float screen_y) const;
  double ScreenToLogicalX(float screen_x) const;
};

KChart::KChart() : impl_(std::make_unique<Impl>()) {
  ResetToDefaultPriceVolumeLayout();
}

KChart::~KChart() = default;

KChart::KChart(KChart&&) noexcept = default;

KChart& KChart::operator=(KChart&&) noexcept = default;

void KChart::SetOptions(const KChartOptions& options) {
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

void KChart::SetVisibleRange(const KVisibleRange& range) {
  if (!range.IsValid()) {
    return;
  }

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
  impl_->has_explicit_visible_range = true;
  if (impl_->chart != nullptr && impl_->chart->controller() != nullptr) {
    impl_->chart->controller()->ScrollBy(delta_logical);
    const Viewport& viewport = impl_->chart->viewport();
    impl_->visible_range = KVisibleRange {viewport.visible_from, viewport.visible_to};
  }
}

void KChart::ZoomBy(double scale_factor, float anchor_ratio) {
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
