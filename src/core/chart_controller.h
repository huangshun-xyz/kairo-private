#pragma once

#include "viewport.h"

namespace kairo {

class Chart;

class ChartController {
 public:
  explicit ChartController(Chart* chart);

  void ScrollBy(double delta_logical);
  void ZoomBy(double scale_factor, double anchor_logical_x);
  void SetVisibleRange(const Viewport& viewport);

 private:
  Chart* chart_ = nullptr;
};

}  // namespace kairo
