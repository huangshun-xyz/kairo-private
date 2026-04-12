#pragma once

#include "hit_result.h"
#include "include/core/SkPoint.h"

namespace kairo {

struct RenderContext;

enum class LayerDrawOrder {
  kUnderlay,
  kOverlay,
};

class Layer {
 public:
  virtual ~Layer() = default;

  virtual LayerDrawOrder draw_order() const {
    return LayerDrawOrder::kOverlay;
  }

  virtual void Draw(RenderContext* ctx) = 0;
  virtual bool HitTest(RenderContext* ctx, SkPoint point, HitResult* result) = 0;
};

}  // namespace kairo
