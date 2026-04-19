#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "kchart_types.h"

class SkCanvas;

namespace kairo {

class Chart;
class CrosshairOverlay;
class KScrollAnimator;
class VectorCandleDataSource;

class KChart {
 public:
  KChart();
  ~KChart();

  KChart(const KChart&) = delete;
  KChart& operator=(const KChart&) = delete;
  KChart(KChart&&) noexcept;
  KChart& operator=(KChart&&) noexcept;

  void SetOptions(const KChartOptions& options);
  const KChartOptions& options() const;

  void ResetToDefaultPriceVolumeLayout();
  bool AddSeries(const KSeriesSpec& spec);
  bool RemoveSeries(const std::string& series_id);
  bool HasSeries(const std::string& series_id) const;
  const std::vector<KSeriesSpec>& series_specs() const;

  void SetData(std::vector<KCandleEntry> candles);
  const std::vector<KCandleEntry>& data() const;

  void SetBounds(float width, float height);
  void SetDeviceScaleFactor(float scale);
  float device_scale_factor() const;
  void SetVisibleRange(const KVisibleRange& range);
  KVisibleRange visible_range() const;
  void ScrollBy(double delta_logical);
  void ZoomBy(double scale_factor, float anchor_ratio);
  void SetScrollInteractionOptions(const KScrollInteractionOptions& options);
  const KScrollInteractionOptions& scroll_interaction_options() const;

  void SetCrosshairEnabled(bool enabled);
  bool crosshair_enabled() const;
  void UpdateCrosshair(float screen_x, float screen_y);
  void ClearCrosshair();

  void Draw(SkCanvas& canvas);

 private:
  friend class KScrollAnimator;

  struct PaneBinding {
    std::string pane_id;
    std::vector<KSeriesType> series_types;
  };

  struct KChartModel {
    KChartOptions options;
    KScrollInteractionOptions scroll_options;
    std::vector<KCandleEntry> candles;
    std::vector<KSeriesSpec> series_specs;
    KVisibleRange visible_range;
    bool has_explicit_visible_range = false;
  };

  struct KChartGraph {
    std::shared_ptr<VectorCandleDataSource> data_source;
    std::unique_ptr<Chart> chart;
    CrosshairOverlay* crosshair_overlay = nullptr;
    std::vector<PaneBinding> pane_bindings;
  };

  struct KChartInteractionState {
    bool scroll_gesture_active = false;
    float device_scale_factor = 1.0f;
    float width = 0.0f;
    float height = 0.0f;
    ScrollInteractionController scroll_controller;
  };

  void BeginScrollGestureInternal();
  bool ScrollGestureByPixelsInternal(float delta_pixels);
  bool EndScrollGestureInternal(float velocity_pixels_per_second);
  bool StepScrollAnimationInternal(double delta_seconds);
  bool IsScrollAnimationActiveInternal() const;
  void StopScrollAnimationInternal();

  void RebuildGraph();
  void UpdateBounds();
  void UpdateCrosshairAt(float screen_x, float screen_y);
  void ClearCrosshairInternal();
  std::optional<std::size_t> FindPaneIndex(float screen_y) const;
  double ScreenToLogicalX(float screen_x) const;
  double ViewportWidth() const;
  double InteractionContentWidth() const;
  double BarSpacingInputUnits() const;
  double ScrollPositionInputUnits() const;
  ScrollMetrics BuildScrollMetrics() const;
  bool SetScrollPositionInputUnits(double position_input_units);
  void StopScrollState();

  KChartModel model_;
  KChartGraph graph_;
  KChartInteractionState interaction_;
};

}  // namespace kairo
