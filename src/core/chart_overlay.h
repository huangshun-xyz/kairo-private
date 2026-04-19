#pragma once

namespace kairo {

struct ChartRenderContext;

class ChartOverlay {
 public:
  virtual ~ChartOverlay() = default;

  virtual void Draw(ChartRenderContext* ctx) = 0;
};

}  // namespace kairo
