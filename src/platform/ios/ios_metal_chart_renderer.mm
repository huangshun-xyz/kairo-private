#include "ios_metal_chart_renderer.h"

#include <cmath>

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

namespace kairo {

KRIOSMetalChartRenderer::KRIOSMetalChartRenderer() {
  metal_device_ = MTLCreateSystemDefaultDevice();
  if (metal_device_ != nil) {
    metal_queue_ = [metal_device_ newCommandQueue];
  }

  if (metal_device_ != nil && metal_queue_ != nil) {
    GrMtlBackendContext backend_context = {};
    backend_context.fDevice.retain((__bridge GrMTLHandle)metal_device_);
    backend_context.fQueue.retain((__bridge GrMTLHandle)metal_queue_);
    direct_context_ = GrDirectContexts::MakeMetal(backend_context);
  }
}

KRIOSMetalChartRenderer::~KRIOSMetalChartRenderer() = default;

bool KRIOSMetalChartRenderer::RenderChart(KChart& chart,
                                          CAMetalLayer* metal_layer,
                                          CGSize size,
                                          CGFloat scale) {
  if (direct_context_ == nullptr || metal_device_ == nil || metal_queue_ == nil ||
      metal_layer == nil || CGSizeEqualToSize(size, CGSizeZero)) {
    return false;
  }

  const int pixel_width = static_cast<int>(std::lround(size.width * scale));
  const int pixel_height = static_cast<int>(std::lround(size.height * scale));
  if (pixel_width <= 0 || pixel_height <= 0) {
    return false;
  }

  metal_layer.device = metal_device_;
  metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  metal_layer.framebufferOnly = NO;
  metal_layer.opaque = YES;
  metal_layer.contentsScale = scale;
  metal_layer.drawableSize =
      CGSizeMake(static_cast<CGFloat>(pixel_width), static_cast<CGFloat>(pixel_height));
  metal_layer.allowsNextDrawableTimeout = NO;

  id<CAMetalDrawable> current_drawable = [metal_layer nextDrawable];
  if (current_drawable == nil) {
    return false;
  }

  GrMtlTextureInfo texture_info;
  texture_info.fTexture.retain((__bridge GrMTLHandle)current_drawable.texture);
  GrBackendRenderTarget backend_render_target =
      GrBackendRenderTargets::MakeMtl(pixel_width, pixel_height, texture_info);

  sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(
      direct_context_.get(),
      backend_render_target,
      kTopLeft_GrSurfaceOrigin,
      kBGRA_8888_SkColorType,
      nullptr,
      nullptr);
  if (surface == nullptr) {
    return false;
  }

  SkCanvas* canvas = surface->getCanvas();
  if (canvas == nullptr) {
    return false;
  }

  canvas->clear(SK_ColorWHITE);
  chart.SetDeviceScaleFactor(static_cast<float>(scale));
  chart.SetBounds(static_cast<float>(pixel_width), static_cast<float>(pixel_height));
  chart.Draw(*canvas);
  direct_context_->flushAndSubmit(surface.get(), GrSyncCpu::kNo);

  id<MTLCommandBuffer> command_buffer = [metal_queue_ commandBuffer];
  if (command_buffer == nil) {
    return false;
  }

  [command_buffer presentDrawable:current_drawable];
  [command_buffer commit];
  return true;
}

}  // namespace kairo
