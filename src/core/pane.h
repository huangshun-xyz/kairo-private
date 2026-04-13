#pragma once

#include <memory>
#include <vector>

#include "include/core/SkRect.h"
#include "layer.h"
#include "pane_layout.h"
#include "series.h"
#include "y_scale.h"

namespace kairo {

// Pane 表示 chart 中的一个垂直区域，拥有自己的布局信息、YScale、Series 和 Layer。
class Pane {
 public:
  Pane();
  explicit Pane(std::unique_ptr<YScale> y_scale);
  explicit Pane(const PaneLayout& layout);
  Pane(const PaneLayout& layout, std::unique_ptr<YScale> y_scale);
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
  PaneLayout layout_;
  SkRect frame_rect_ = SkRect::MakeEmpty();
  SkRect content_rect_ = SkRect::MakeEmpty();
  std::unique_ptr<YScale> y_scale_;
  std::vector<std::unique_ptr<Series>> series_;
  std::vector<std::unique_ptr<Layer>> layers_;
};

}  // namespace kairo
