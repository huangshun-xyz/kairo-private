#import "MainView.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <memory>

#import <QuartzCore/QuartzCore.h>

#include "chart.h"
#include "demo_chart_factory.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"

namespace {

void ReleaseBitmapCopy(void* info, const void* data, size_t size) {
  (void)info;
  (void)size;
  free(const_cast<void*>(data));
}

CGImageRef CreateImageFromBitmap(const SkBitmap& bitmap, int pixel_width, int pixel_height) {
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (color_space == nullptr) {
    return nullptr;
  }

  const size_t byte_size = bitmap.computeByteSize();
  void* pixel_copy = malloc(byte_size);
  if (pixel_copy == nullptr) {
    CGColorSpaceRelease(color_space);
    return nullptr;
  }
  memcpy(pixel_copy, bitmap.getPixels(), byte_size);

  const CGBitmapInfo bitmap_info =
      static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
  CGDataProviderRef provider = CGDataProviderCreateWithData(
      nullptr, pixel_copy, byte_size, ReleaseBitmapCopy);
  if (provider == nullptr) {
    free(pixel_copy);
    CGColorSpaceRelease(color_space);
    return nullptr;
  }

  CGImageRef image = CGImageCreate(
      pixel_width,
      pixel_height,
      8,
      32,
      bitmap.rowBytes(),
      color_space,
      bitmap_info,
      provider,
      nullptr,
      false,
      kCGRenderingIntentDefault);

  CGDataProviderRelease(provider);
  CGColorSpaceRelease(color_space);
  return image;
}

}  // namespace

@implementation MainView {
  std::unique_ptr<kairo::Chart> _chart;
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self == nil) {
    return nil;
  }

  self.backgroundColor = [UIColor whiteColor];
  self.contentScaleFactor = UIScreen.mainScreen.scale;
  self.layer.contentsGravity = kCAGravityResize;
  self.layer.contentsScale = self.contentScaleFactor;
  _chart = kairo::CreateDemoChart();
  return self;
}

- (void)didMoveToWindow {
  [super didMoveToWindow];
  [self renderChart];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self renderChart];
}

- (void)renderChart {
  const CGRect bounds = self.bounds;
  if (CGRectIsEmpty(bounds)) {
    return;
  }

  const CGFloat scale = self.contentScaleFactor;
  const int pixel_width = static_cast<int>(std::lround(bounds.size.width * scale));
  const int pixel_height = static_cast<int>(std::lround(bounds.size.height * scale));
  if (pixel_width <= 0 || pixel_height <= 0) {
    return;
  }

  SkBitmap bitmap;
  const SkImageInfo info = SkImageInfo::MakeN32Premul(pixel_width, pixel_height);
  if (!bitmap.tryAllocPixels(info)) {
    return;
  }

  SkCanvas canvas(bitmap);
  if (_chart == nullptr) {
    return;
  }

  canvas.clear(SK_ColorWHITE);
  _chart->SetBounds(SkRect::MakeWH(static_cast<float>(pixel_width), static_cast<float>(pixel_height)));
  _chart->Draw(canvas);

  CGImageRef image = CreateImageFromBitmap(bitmap, pixel_width, pixel_height);
  if (image == nullptr) {
    return;
  }

  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  self.layer.contents = (__bridge id)image;
  self.layer.contentsScale = scale;
  [CATransaction commit];
  CGImageRelease(image);
}

@end
