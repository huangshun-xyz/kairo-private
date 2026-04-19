#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "pane_layout.h"
#include "scroll_interaction.h"

class SkCanvas;

namespace kairo {

class Chart;
class CrosshairOverlay;
class KScrollAnimator;
class VectorCandleDataSource;

struct KCandleEntry {
  double open = 0.0;
  double high = 0.0;
  double low = 0.0;
  double close = 0.0;
  double volume = 0.0;
};

using KColor = std::uint32_t;

constexpr KColor KColorARGB(std::uint8_t alpha,
                            std::uint8_t red,
                            std::uint8_t green,
                            std::uint8_t blue) {
  return (static_cast<KColor>(alpha) << 24u) | (static_cast<KColor>(red) << 16u) |
         (static_cast<KColor>(green) << 8u) | static_cast<KColor>(blue);
}

enum class KSeriesType {
  kCandles,
  kVolume,
};

enum class KScrollBoundaryBehavior {
  kClamp,
  kBounce,
};

enum class KPlacementKind {
  kMainPane,
  kNewPane,
  kExistingPane,
  kOverlayOnSeries,
};

struct KPanePlacement {
  KPlacementKind kind = KPlacementKind::kMainPane;
  std::string pane_id;
  std::string target_series_id;
  float pane_height = 96.0f;
};

struct KSeriesStyle {
  KColor primary_color = KColorARGB(255, 6, 176, 116);
  KColor secondary_color = KColorARGB(255, 84, 82, 236);
  KColor tertiary_color = KColorARGB(255, 197, 203, 213);
};

struct KSeriesSpec {
  std::string id;
  std::string title;
  KSeriesType type = KSeriesType::kCandles;
  KPanePlacement placement;
  bool visible = true;
  KSeriesStyle style;
};

struct KVisibleRange {
  double from = 0.0;
  double to = 24.0;

  bool IsValid() const {
    return to > from;
  }
};

struct KScrollSpringOptions {
  double mass = 0.5;
  double stiffness = 100.0;
  double damping_ratio = 1.1;
};

struct KScrollInteractionOptions {
  KScrollBoundaryBehavior boundary_behavior = KScrollBoundaryBehavior::kBounce;
  bool always_scrollable = true;
  bool allow_inertia = true;
  double drag_sensitivity = 1.0;
  double overscroll_friction = 0.52;
  double deceleration_rate = 0.135;
  KScrollSpringOptions spring;
};

struct KChartOptions {
  Insets content_insets {12.0f, 12.0f, 12.0f, 12.0f};
  float default_sub_pane_height = 96.0f;
  bool crosshair_enabled = true;
};

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
