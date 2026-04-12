#pragma once

#include "include/core/SkColor.h"
#include "layer.h"

namespace kairo {

class CrosshairLayer final : public Layer {
 public:
  void SetCrosshair(double logical_x, double value);
  void SetVisible(bool visible);
  void SetColor(SkColor color);

  LayerDrawOrder draw_order() const override;
  void Draw(RenderContext* ctx) override;
  bool HitTest(RenderContext* ctx, SkPoint point, HitResult* result) override;

 private:
  SkColor color_ = SkColorSetARGB(190, 30, 99, 255);
  bool visible_ = false;
  double logical_x_ = 0.0;
  double value_ = 0.0;
};

}  // namespace kairo
