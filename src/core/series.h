#pragma once

#include "hit_result.h"
#include "include/core/SkPoint.h"
#include "range.h"
#include "viewport.h"

namespace kairo {

struct RenderContext;

class Series {
 public:
  virtual ~Series() = default;

  virtual void Draw(RenderContext* ctx) = 0;
  virtual Range GetVisibleRange(const Viewport& viewport) const = 0;
  virtual bool HitTest(RenderContext* ctx, SkPoint point, HitResult* result) = 0;
};

}  // namespace kairo
