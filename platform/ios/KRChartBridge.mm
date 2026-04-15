#import "KRChartBridge.h"

#include <cmath>
#include <memory>
#include <vector>

#import <Metal/Metal.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendSurface.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "include/gpu/ganesh/mtl/GrMtlTypes.h"
#include "kchart.h"

namespace {

std::vector<kairo::KCandleEntry> ToPublicCandles(const KRCandleValue* candles, NSInteger count) {
  std::vector<kairo::KCandleEntry> public_candles;
  if (candles == nullptr || count <= 0) {
    return public_candles;
  }

  public_candles.reserve(static_cast<std::size_t>(count));
  for (NSInteger index = 0; index < count; ++index) {
    const KRCandleValue& candle = candles[index];
    public_candles.push_back(kairo::KCandleEntry {
        candle.open,
        candle.high,
        candle.low,
        candle.close,
        candle.volume,
    });
  }

  return public_candles;
}

}  // namespace

@implementation KRChartBridge {
  std::unique_ptr<kairo::KChart> _chart;
  id<MTLDevice> _metalDevice;
  id<MTLCommandQueue> _metalQueue;
  sk_sp<GrDirectContext> _directContext;
}

- (instancetype)init {
  self = [super init];
  if (self == nil) {
    return nil;
  }

  _chart = std::make_unique<kairo::KChart>();
  _crosshairEnabled = YES;

  _metalDevice = MTLCreateSystemDefaultDevice();
  if (_metalDevice != nil) {
    _metalQueue = [_metalDevice newCommandQueue];
  }

  if (_metalDevice != nil && _metalQueue != nil) {
    GrMtlBackendContext backend_context = {};
    backend_context.fDevice.retain((__bridge GrMTLHandle)_metalDevice);
    backend_context.fQueue.retain((__bridge GrMTLHandle)_metalQueue);
    _directContext = GrDirectContexts::MakeMetal(backend_context);
  }

  return self;
}

- (void)setCrosshairEnabled:(BOOL)crosshairEnabled {
  _crosshairEnabled = crosshairEnabled;
  if (_chart != nullptr) {
    _chart->SetCrosshairEnabled(crosshairEnabled);
    if (!crosshairEnabled) {
      _chart->ClearCrosshair();
    }
  }
}

- (void)resetToDefaultPriceVolumeLayout {
  if (_chart == nullptr) {
    return;
  }

  _chart->ResetToDefaultPriceVolumeLayout();
  _chart->SetCrosshairEnabled(self.crosshairEnabled);
}

- (void)setCandles:(const KRCandleValue*)candles count:(NSInteger)count {
  if (_chart == nullptr) {
    return;
  }

  _chart->SetData(ToPublicCandles(candles, count));
  _chart->SetCrosshairEnabled(self.crosshairEnabled);
}

- (void)scrollBy:(double)deltaLogical {
  if (_chart != nullptr) {
    _chart->ScrollBy(deltaLogical);
  }
}

- (void)zoomBy:(double)scaleFactor anchorRatio:(float)anchorRatio {
  if (_chart != nullptr) {
    _chart->ZoomBy(scaleFactor, anchorRatio);
  }
}

- (void)updateCrosshairAtX:(float)x y:(float)y {
  if (_chart != nullptr && self.crosshairEnabled) {
    _chart->UpdateCrosshair(x, y);
  }
}

- (void)clearCrosshair {
  if (_chart != nullptr) {
    _chart->ClearCrosshair();
  }
}

- (void)renderToMetalLayer:(CAMetalLayer*)metalLayer size:(CGSize)size scale:(CGFloat)scale {
  if (_chart == nullptr || _directContext == nullptr || _metalDevice == nil || _metalQueue == nil ||
      metalLayer == nil || CGSizeEqualToSize(size, CGSizeZero)) {
    return;
  }

  const int pixel_width = static_cast<int>(std::lround(size.width * scale));
  const int pixel_height = static_cast<int>(std::lround(size.height * scale));
  if (pixel_width <= 0 || pixel_height <= 0) {
    return;
  }

  metalLayer.device = _metalDevice;
  metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  metalLayer.framebufferOnly = NO;
  metalLayer.opaque = YES;
  metalLayer.contentsScale = scale;
  metalLayer.drawableSize =
      CGSizeMake(static_cast<CGFloat>(pixel_width), static_cast<CGFloat>(pixel_height));
  if ([metalLayer respondsToSelector:@selector(setAllowsNextDrawableTimeout:)]) {
    metalLayer.allowsNextDrawableTimeout = NO;
  }

  id<CAMetalDrawable> current_drawable = [metalLayer nextDrawable];
  if (current_drawable == nil) {
    return;
  }

  GrMtlTextureInfo texture_info;
  texture_info.fTexture.retain((__bridge GrMTLHandle)current_drawable.texture);
  GrBackendRenderTarget backend_render_target =
      GrBackendRenderTargets::MakeMtl(pixel_width, pixel_height, texture_info);

  sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(
      _directContext.get(),
      backend_render_target,
      kTopLeft_GrSurfaceOrigin,
      kBGRA_8888_SkColorType,
      nullptr,
      nullptr);
  if (surface == nullptr) {
    return;
  }

  SkCanvas* canvas = surface->getCanvas();
  if (canvas == nullptr) {
    return;
  }

  canvas->clear(SK_ColorWHITE);
  _chart->SetBounds(static_cast<float>(pixel_width), static_cast<float>(pixel_height));
  _chart->Draw(*canvas);
  _directContext->flushAndSubmit(surface.get(), GrSyncCpu::kNo);

  id<MTLCommandBuffer> command_buffer = [_metalQueue commandBuffer];
  if (command_buffer == nil) {
    return;
  }

  [command_buffer presentDrawable:current_drawable];
  [command_buffer commit];
}

@end
