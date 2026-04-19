#pragma once

#include <cstddef>

#include "chart_overlay.h"
#include "include/core/SkColor.h"

namespace kairo {

class CrosshairOverlay final : public ChartOverlay {
 public:
  void SetCrosshair(size_t pane_index, double logical_x, double value);
  void SetVisible(bool visible);
  void SetColor(SkColor color);

  void Draw(ChartRenderContext* ctx) override;

 private:
  SkColor color_ = SkColorSetARGB(190, 30, 99, 255);
  bool visible_ = false;
  size_t active_pane_index_ = 0;
  double logical_x_ = 0.0;
  double value_ = 0.0;
};

}  // namespace kairo
