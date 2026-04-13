#include "demo_chart_factory.h"

#include <memory>
#include <vector>

#include "candle_data_source.h"
#include "candle_series.h"
#include "chart.h"
#include "crosshair_overlay.h"
#include "grid_layer.h"
#include "volume_series.h"

namespace kairo {

std::unique_ptr<Chart> CreateDemoChart() {
  std::vector<CandleData> candles = {
      {102.0, 106.0, 99.0, 104.0, 1200.0},
      {104.0, 107.0, 101.5, 103.0, 980.0},
      {103.0, 108.5, 102.0, 107.0, 1320.0},
      {107.0, 109.0, 104.5, 105.0, 1110.0},
      {105.0, 110.0, 104.0, 109.5, 1450.0},
      {109.5, 111.0, 108.0, 108.2, 1020.0},
      {108.2, 112.5, 107.0, 111.6, 1580.0},
      {111.6, 114.0, 110.0, 113.4, 1710.0},
      {113.4, 115.5, 111.8, 112.0, 1490.0},
      {112.0, 116.0, 110.5, 115.2, 1680.0},
      {115.2, 118.0, 114.0, 117.5, 1750.0},
      {117.5, 119.0, 115.8, 116.3, 1410.0},
      {116.3, 120.5, 115.5, 119.6, 1820.0},
      {119.6, 121.0, 117.2, 118.0, 1380.0},
      {118.0, 122.0, 117.0, 121.4, 1960.0},
      {121.4, 123.2, 118.5, 119.3, 1520.0},
      {119.3, 124.0, 118.0, 123.4, 2050.0},
      {123.4, 126.0, 122.2, 125.1, 2140.0},
      {125.1, 127.5, 123.8, 124.2, 1760.0},
      {124.2, 128.0, 123.5, 127.4, 2210.0},
  };

  auto chart = std::make_unique<Chart>();
  chart->SetViewport(Viewport{0.0, static_cast<double>(candles.size())});
  chart->SetContentInsets(Insets{12.0f, 12.0f, 12.0f, 12.0f});

  auto data_source = std::make_shared<VectorCandleDataSource>(candles);
  PaneLayout price_layout;
  price_layout.size_mode = PaneSizeMode::kStretch;
  price_layout.insets.bottom = 4.0f;

  auto price_pane = std::make_unique<Pane>(price_layout);
  price_pane->AddLayer(std::make_unique<GridLayer>());
  price_pane->AddSeries(std::make_unique<CandleSeries>(data_source));
  chart->AddPane(std::move(price_pane));

  PaneLayout volume_layout;
  volume_layout.size_mode = PaneSizeMode::kFixed;
  volume_layout.height = 96.0f;
  volume_layout.insets.top = 4.0f;

  auto volume_pane = std::make_unique<Pane>(volume_layout);
  volume_pane->AddLayer(std::make_unique<GridLayer>());
  volume_pane->AddSeries(std::make_unique<VolumeSeries>(data_source));
  chart->AddPane(std::move(volume_pane));

  auto crosshair = std::make_unique<CrosshairOverlay>();
  crosshair->SetVisible(true);
  crosshair->SetCrosshair(
      0u, static_cast<double>(candles.size() - 2), candles[candles.size() - 2].close);
  chart->AddOverlay(std::move(crosshair));
  return chart;
}

}  // namespace kairo
