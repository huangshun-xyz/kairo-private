#pragma once

#include <memory>
#include <vector>

#include "chart_overlay.h"
#include "include/core/SkRect.h"
#include "pane.h"
#include "uniform_bar_x_scale.h"
#include "viewport.h"

class SkCanvas;

namespace kairo {

class ChartController;

// Chart 是顶层对象，负责管理共享横轴、纵向 pane 布局和 chart 级 overlay。
class Chart {
 public:
  Chart();
  ~Chart();

  Chart(const Chart&) = delete;
  Chart& operator=(const Chart&) = delete;
  Chart(Chart&&) = delete;
  Chart& operator=(Chart&&) = delete;

  void SetBounds(const SkRect& bounds);
  void Layout(const SkRect& bounds);
  const SkRect& bounds() const;
  const SkRect& content_bounds() const;

  void SetContentInsets(const Insets& insets);
  const Insets& content_insets() const;

  void SetViewport(const Viewport& viewport);
  const Viewport& viewport() const;

  UniformBarXScale* x_scale();
  const UniformBarXScale* x_scale() const;

  Pane* AddPane(std::unique_ptr<Pane> pane);
  std::vector<std::unique_ptr<Pane>>& panes();
  const std::vector<std::unique_ptr<Pane>>& panes() const;

  ChartOverlay* AddOverlay(std::unique_ptr<ChartOverlay> overlay);
  std::vector<std::unique_ptr<ChartOverlay>>& overlays();
  const std::vector<std::unique_ptr<ChartOverlay>>& overlays() const;

  ChartController* controller();
  const ChartController* controller() const;

  void Draw(SkCanvas& canvas);

 private:
  void LayoutPanes();
  void SyncScales();

  SkRect bounds_ = SkRect::MakeEmpty();
  SkRect content_bounds_ = SkRect::MakeEmpty();
  Insets content_insets_;
  Viewport viewport_ {0.0, 24.0};
  UniformBarXScale x_scale_;
  std::vector<std::unique_ptr<Pane>> panes_;
  std::vector<std::unique_ptr<ChartOverlay>> overlays_;
  std::unique_ptr<ChartController> controller_;
};

}  // namespace kairo
