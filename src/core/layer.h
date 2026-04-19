#pragma once

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
};

}  // namespace kairo
