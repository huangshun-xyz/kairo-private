#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "pane_layout.h"

class SkCanvas;

namespace kairo {

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
  void BeginScrollGesture();
  bool ScrollGestureByPixels(float delta_pixels);
  bool EndScrollGesture(float velocity_pixels_per_second);
  bool StepScrollAnimation(double delta_seconds);
  bool IsScrollAnimationActive() const;
  void StopScrollAnimation();

  void SetCrosshairEnabled(bool enabled);
  bool crosshair_enabled() const;
  void UpdateCrosshair(float screen_x, float screen_y);
  void ClearCrosshair();

  void Draw(SkCanvas& canvas);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace kairo
