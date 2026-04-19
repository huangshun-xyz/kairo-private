#pragma once

#include <memory>

#include "candle_data_source.h"
#include "include/core/SkColor.h"
#include "series.h"

namespace kairo {

class VolumeSeries final : public Series {
 public:
  explicit VolumeSeries(std::shared_ptr<VectorCandleDataSource> data_source);

  void SetUpColor(SkColor color);
  void SetDownColor(SkColor color);

  void Draw(RenderContext* ctx) override;
  Range GetVisibleRange(const Viewport& viewport) const override;
  bool HitTest(RenderContext* ctx, SkPoint point, HitResult* result) override;

 private:
  std::shared_ptr<VectorCandleDataSource> data_source_;
  SkColor up_color_ = SkColorSetRGB(6, 176, 116);
  SkColor down_color_ = SkColorSetRGB(84, 82, 236);
};

}  // namespace kairo
