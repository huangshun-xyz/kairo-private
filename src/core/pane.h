#pragma once

#include <memory>
#include <vector>

#include "include/core/SkRect.h"
#include "layer.h"
#include "series.h"
#include "y_scale.h"

namespace kairo {

// Pane 表示一个纵向堆叠的图表区域，拥有自己的 YScale、Series 和 Layer。
class Pane {
 public:
  Pane();
  explicit Pane(std::unique_ptr<YScale> y_scale);
  ~Pane();

  Pane(const Pane&) = delete;
  Pane& operator=(const Pane&) = delete;
  Pane(Pane&&) = delete;
  Pane& operator=(Pane&&) = delete;

  void SetBounds(const SkRect& bounds);
  const SkRect& bounds() const;

  void SetHeightWeight(float weight);
  float height_weight() const;

  YScale* y_scale();
  const YScale* y_scale() const;

  void AddSeries(std::unique_ptr<Series> series);
  void AddLayer(std::unique_ptr<Layer> layer);

  std::vector<std::unique_ptr<Series>>& series();
  const std::vector<std::unique_ptr<Series>>& series() const;

  std::vector<std::unique_ptr<Layer>>& layers();
  const std::vector<std::unique_ptr<Layer>>& layers() const;

  void UpdateAutoRange(const Viewport& viewport);

 private:
  SkRect bounds_ = SkRect::MakeEmpty();
  float height_weight_ = 1.0f;
  std::unique_ptr<YScale> y_scale_;
  std::vector<std::unique_ptr<Series>> series_;
  std::vector<std::unique_ptr<Layer>> layers_;
};

}  // namespace kairo
