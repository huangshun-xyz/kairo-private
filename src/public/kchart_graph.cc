#include "kchart.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "candle_data_source.h"
#include "candle_series.h"
#include "chart.h"
#include "crosshair_overlay.h"
#include "grid_layer.h"
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

std::optional<KColor> DebugPaneBackgroundColorForPane(const std::string& pane_id) {
  if (pane_id == "main") {
    return KColorARGB(255, 255, 237, 242);
  }
  if (pane_id == "volume") {
    return KColorARGB(255, 252, 242, 230);
  }
  return std::nullopt;
}

}  // namespace

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
    } else {
      layout.size_mode = PaneSizeMode::kFixed;
      layout.height =
          placement.pane_height > 0.0f ? placement.pane_height : model_.options.default_sub_pane_height;
    }

    auto pane = std::make_unique<Pane>(layout);
    const std::optional<KColor> background_color = DebugPaneBackgroundColorForPane(pane_id);
    if (background_color.has_value()) {
      pane->SetBackgroundColor(ToSkColor(*background_color));
    }
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

}  // namespace kairo
