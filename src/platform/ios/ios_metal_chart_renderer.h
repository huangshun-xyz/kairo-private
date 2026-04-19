#pragma once

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "include/gpu/ganesh/GrDirectContext.h"

namespace kairo {

class KChart;

class KRIOSMetalChartRenderer final {
 public:
  KRIOSMetalChartRenderer();
  ~KRIOSMetalChartRenderer();

  KRIOSMetalChartRenderer(const KRIOSMetalChartRenderer&) = delete;
  KRIOSMetalChartRenderer& operator=(const KRIOSMetalChartRenderer&) = delete;

  bool RenderChart(KChart& chart, CAMetalLayer* metal_layer, CGSize size, CGFloat scale);

 private:
  id<MTLDevice> metal_device_ = nil;
  id<MTLCommandQueue> metal_queue_ = nil;
  sk_sp<GrDirectContext> direct_context_;
};

}  // namespace kairo
