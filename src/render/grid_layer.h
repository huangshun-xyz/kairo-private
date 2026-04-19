#pragma once

#include "include/core/SkColor.h"
#include "layer.h"

namespace kairo {

class GridLayer final : public Layer {
 public:
  void SetBackgroundColor(SkColor color);
  void SetGridLineColor(SkColor color);

  LayerDrawOrder draw_order() const override;
  void Draw(RenderContext* ctx) override;

 private:
  SkColor background_color_ = SK_ColorWHITE;
  SkColor grid_line_color_ = SkColorSetRGB(229, 234, 240);
};

}  // namespace kairo
