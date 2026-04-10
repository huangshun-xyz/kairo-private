#pragma once

namespace kairo::core {
class Chart;
}  // namespace kairo::core

class SkCanvas;

namespace kairo::render {

class SkiaRenderer {
 public:
  void DrawChart(SkCanvas& canvas, const core::Chart& chart, int width, int height) const;
};

}  // namespace kairo::render
