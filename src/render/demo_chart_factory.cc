#include "demo_chart_factory.h"

#include <memory>
#include <vector>

#include "candle_data_source.h"
#include "candle_series.h"
#include "chart.h"
#include "crosshair_layer.h"
#include "grid_layer.h"

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

  auto data_source = std::make_shared<VectorCandleDataSource>(candles);
  auto pane = std::make_unique<Pane>();
  pane->AddLayer(std::make_unique<GridLayer>());
  pane->AddSeries(std::make_unique<CandleSeries>(data_source));

  auto crosshair = std::make_unique<CrosshairLayer>();
  crosshair->SetVisible(true);
  crosshair->SetCrosshair(static_cast<double>(candles.size() - 2), candles[candles.size() - 2].close);
  pane->AddLayer(std::move(crosshair));

  chart->AddPane(std::move(pane));
  return chart;
}

}  // namespace kairo
