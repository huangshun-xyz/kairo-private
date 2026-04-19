#import "KRIOSChartHost.h"

#include <memory>
#include <vector>
#include "chart_runtime.h"
#include "display_link_frame_source.h"
#include "invalidation_sink.h"
#include "ios_metal_chart_renderer.h"
#include "kchart.h"
#include "runtime_host.h"

@interface KRIOSChartHost ()

- (void)notifyNeedsDisplay;

@end

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

kairo::KScrollBoundaryBehavior ToPublicBoundaryBehavior(KRScrollBoundaryBehaviorValue behavior) {
  switch (behavior) {
    case KRScrollBoundaryBehaviorValueClamp:
      return kairo::KScrollBoundaryBehavior::kClamp;
    case KRScrollBoundaryBehaviorValueBounce:
      return kairo::KScrollBoundaryBehavior::kBounce;
  }
  return kairo::KScrollBoundaryBehavior::kBounce;
}

kairo::KScrollInteractionOptions ToPublicScrollOptions(KRScrollInteractionOptionsValue options) {
  kairo::KScrollInteractionOptions public_options;
  public_options.boundary_behavior = ToPublicBoundaryBehavior(options.boundaryBehavior);
  public_options.always_scrollable = options.alwaysScrollable;
  public_options.allow_inertia = options.allowInertia;
  public_options.drag_sensitivity = options.dragSensitivity;
  public_options.overscroll_friction = options.overscrollFriction;
  public_options.deceleration_rate = options.decelerationRate;
  public_options.spring.mass = options.springMass;
  public_options.spring.stiffness = options.springStiffness;
  public_options.spring.damping_ratio = options.springDampingRatio;
  return public_options;
}

}  // namespace

class HostInvalidationSink final : public kairo::KInvalidationSink {
 public:
  explicit HostInvalidationSink(KRIOSChartHost* owner) : owner_(owner) {}

  void RequestRedraw() override {
    KRIOSChartHost* owner = owner_;
    if (owner == nil) {
      return;
    }
    [owner notifyNeedsDisplay];
  }

 private:
  KRIOSChartHost* owner_ = nil;
};

@implementation KRIOSChartHost {
  std::unique_ptr<kairo::KChart> _chart;
  std::unique_ptr<kairo::KChartRuntime> _runtime;
  std::unique_ptr<kairo::DisplayLinkFrameSource> _frameSource;
  std::unique_ptr<HostInvalidationSink> _invalidationSink;
  std::unique_ptr<kairo::KRIOSMetalChartRenderer> _renderer;
  CGFloat _inputScale;
}

- (instancetype)init {
  self = [super init];
  if (self == nil) {
    return nil;
  }

  _chart = std::make_unique<kairo::KChart>();
  _runtime = std::make_unique<kairo::KChartRuntime>(_chart.get());
  _frameSource = std::make_unique<kairo::DisplayLinkFrameSource>();
  _invalidationSink = std::make_unique<HostInvalidationSink>(self);
  _renderer = std::make_unique<kairo::KRIOSMetalChartRenderer>();
  _crosshairEnabled = YES;
  _inputScale = 1.0;

  _runtime->BindHost(kairo::KRuntimeHost {
      _frameSource.get(),
      _invalidationSink.get(),
  });

  return self;
}

- (void)dealloc {
  if (_runtime != nullptr) {
    _runtime->UnbindHost();
  }
  [super dealloc];
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

  if (_runtime != nullptr) {
    _runtime->CancelTransientInteractions();
  }
  _chart->ResetToDefaultPriceVolumeLayout();
  _chart->SetCrosshairEnabled(self.crosshairEnabled);
}

- (void)setCandles:(const KRCandleValue*)candles count:(NSInteger)count {
  if (_chart == nullptr) {
    return;
  }

  if (_runtime != nullptr) {
    _runtime->CancelTransientInteractions();
  }
  _chart->SetData(ToPublicCandles(candles, count));
  _chart->SetCrosshairEnabled(self.crosshairEnabled);
}

- (void)setHostActive:(BOOL)active {
  if (!active && _runtime != nullptr) {
    _runtime->CancelTransientInteractions();
  }
}

- (void)setScrollInteractionOptions:(KRScrollInteractionOptionsValue)options {
  if (_chart != nullptr) {
    _chart->SetScrollInteractionOptions(ToPublicScrollOptions(options));
  }
}

- (void)handlePanBeganAtX:(CGFloat)x y:(CGFloat)y {
  if (_runtime != nullptr) {
    _runtime->OnPanBegin(static_cast<float>(x), static_cast<float>(y));
  }
}

- (void)handlePanChangedByDeltaX:(CGFloat)deltaX deltaY:(CGFloat)deltaY {
  if (_runtime != nullptr) {
    _runtime->OnPanUpdate(static_cast<float>(deltaX), static_cast<float>(deltaY));
  }
}

- (void)handlePanEndedWithVelocityX:(CGFloat)velocityX velocityY:(CGFloat)velocityY {
  if (_runtime != nullptr) {
    _runtime->OnPanEnd(static_cast<float>(velocityX), static_cast<float>(velocityY));
  }
}

- (void)handlePinchBeganAtAnchorRatio:(float)anchorRatio {
  if (_runtime != nullptr) {
    _runtime->OnPinchBegin(anchorRatio);
  }
}

- (void)handlePinchChangedScale:(CGFloat)scale anchorRatio:(float)anchorRatio {
  if (_runtime != nullptr) {
    _runtime->OnPinchUpdate(static_cast<float>(scale), anchorRatio);
  }
}

- (void)handlePinchEnded {
  if (_runtime != nullptr) {
    _runtime->OnPinchEnd();
  }
}

- (void)handleLongPressBeganAtX:(CGFloat)x y:(CGFloat)y {
  if (_runtime != nullptr) {
    _runtime->OnLongPressBegin(
        static_cast<float>(x * _inputScale), static_cast<float>(y * _inputScale));
  }
}

- (void)handleLongPressMovedToX:(CGFloat)x y:(CGFloat)y {
  if (_runtime != nullptr) {
    _runtime->OnLongPressMove(
        static_cast<float>(x * _inputScale), static_cast<float>(y * _inputScale));
  }
}

- (void)handleLongPressEnded {
  if (_runtime != nullptr) {
    _runtime->OnLongPressEnd();
  }
}

- (void)clearCrosshair {
  if (_chart != nullptr) {
    _chart->ClearCrosshair();
  }
}

- (void)renderToMetalLayer:(CAMetalLayer*)metalLayer size:(CGSize)size scale:(CGFloat)scale {
  if (_chart == nullptr || _renderer == nullptr || metalLayer == nil || CGSizeEqualToSize(size, CGSizeZero)) {
    return;
  }

  _inputScale = scale;
  _renderer->RenderChart(*_chart, metalLayer, size, scale);
}

- (void)notifyNeedsDisplay {
  id<KRIOSChartHostDelegate> delegate = self.delegate;
  if (delegate != nil) {
    [delegate chartHostNeedsDisplay:self];
  }
}

@end
