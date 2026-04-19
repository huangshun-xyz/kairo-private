#pragma once

#include <memory>
#include <vector>

#include "include/core/SkColor.h"
#include "include/core/SkRect.h"
#include "layer.h"
#include "linear_y_scale.h"
#include "pane_layout.h"
#include "series.h"

namespace kairo {

// Pane 表示 chart 中的一个垂直区域，拥有自己的布局信息、纵轴、Series 和 Layer。
class Pane {
 public:
  Pane();
  explicit Pane(const PaneLayout& layout);
  ~Pane();

  Pane(const Pane&) = delete;
  Pane& operator=(const Pane&) = delete;
  Pane(Pane&&) = delete;
  Pane& operator=(Pane&&) = delete;

  void SetLayout(const PaneLayout& layout);
  const PaneLayout& layout() const;

  void SetFrameRect(const SkRect& rect);
  const SkRect& frame_rect() const;

  void SetContentRect(const SkRect& rect);
  const SkRect& content_rect() const;

  void SetBackgroundColor(SkColor color);
  SkColor background_color() const;

  LinearYScale* y_scale();
  const LinearYScale* y_scale() const;

  void AddSeries(std::unique_ptr<Series> series);
  void AddLayer(std::unique_ptr<Layer> layer);

  std::vector<std::unique_ptr<Series>>& series();
  const std::vector<std::unique_ptr<Series>>& series() const;

  std::vector<std::unique_ptr<Layer>>& layers();
  const std::vector<std::unique_ptr<Layer>>& layers() const;

  void UpdateAutoRange(const Viewport& viewport);

 private:
  PaneLayout layout_;
  SkRect frame_rect_ = SkRect::MakeEmpty();
  SkRect content_rect_ = SkRect::MakeEmpty();
  SkColor background_color_ = SK_ColorWHITE;
  LinearYScale y_scale_;
  std::vector<std::unique_ptr<Series>> series_;
  std::vector<std::unique_ptr<Layer>> layers_;
};

}  // namespace kairo
