#pragma once

#include <memory>
#include <vector>

#include "include/core/SkRect.h"
#include "pane.h"
#include "viewport.h"
#include "x_scale.h"

class SkCanvas;

namespace kairo {

class ChartController;

// Chart 是顶层对象，负责管理共享的横轴状态并调度各个 pane 的绘制。
class Chart {
 public:
  Chart();
  explicit Chart(std::unique_ptr<XScale> x_scale);
  ~Chart();

  Chart(const Chart&) = delete;
  Chart& operator=(const Chart&) = delete;
  Chart(Chart&&) = delete;
  Chart& operator=(Chart&&) = delete;

  void SetBounds(const SkRect& bounds);
  const SkRect& bounds() const;

  void SetViewport(const Viewport& viewport);
  const Viewport& viewport() const;

  XScale* x_scale();
  const XScale* x_scale() const;

  Pane* AddPane(std::unique_ptr<Pane> pane);
  std::vector<std::unique_ptr<Pane>>& panes();
  const std::vector<std::unique_ptr<Pane>>& panes() const;

  ChartController* controller();
  const ChartController* controller() const;

  void Draw(SkCanvas& canvas);

 private:
  void LayoutPanes();
  void SyncScales();

  SkRect bounds_ = SkRect::MakeEmpty();
  Viewport viewport_ {0.0, 24.0};
  std::unique_ptr<XScale> x_scale_;
  std::vector<std::unique_ptr<Pane>> panes_;
  std::unique_ptr<ChartController> controller_;
};

}  // namespace kairo
