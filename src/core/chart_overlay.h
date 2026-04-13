#pragma once

#include "hit_result.h"
#include "include/core/SkPoint.h"

namespace kairo {

struct ChartRenderContext;

class ChartOverlay {
 public:
  virtual ~ChartOverlay() = default;

  virtual void Draw(ChartRenderContext* ctx) = 0;
  virtual bool HitTest(ChartRenderContext* ctx, SkPoint point, HitResult* result) {
    (void)ctx;
    (void)point;
    (void)result;
    return false;
  }
};

}  // namespace kairo
