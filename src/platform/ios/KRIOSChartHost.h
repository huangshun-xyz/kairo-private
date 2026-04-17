#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>

// Internal iOS host for KRChartView. Not part of the business-facing API surface.
typedef struct KRCandleValue {
  double open;
  double high;
  double low;
  double close;
  double volume;
} KRCandleValue;

typedef NS_ENUM(NSInteger, KRScrollBoundaryBehaviorValue) {
  KRScrollBoundaryBehaviorValueClamp = 0,
  KRScrollBoundaryBehaviorValueBounce = 1,
};

typedef struct KRScrollInteractionOptionsValue {
  KRScrollBoundaryBehaviorValue boundaryBehavior;
  BOOL alwaysScrollable;
  BOOL allowInertia;
  double dragSensitivity;
  double overscrollFriction;
  double decelerationRate;
  double springMass;
  double springStiffness;
  double springDampingRatio;
} KRScrollInteractionOptionsValue;

NS_ASSUME_NONNULL_BEGIN

@class KRIOSChartHost;

@protocol KRIOSChartHostDelegate <NSObject>

- (void)chartHostNeedsDisplay:(KRIOSChartHost*)host;

@end

@interface KRIOSChartHost : NSObject

@property(nonatomic, assign, getter=isCrosshairEnabled) BOOL crosshairEnabled;
@property(nonatomic, assign, nullable) id<KRIOSChartHostDelegate> delegate;

- (void)resetToDefaultPriceVolumeLayout;
- (void)setCandles:(const KRCandleValue* _Nullable)candles
             count:(NSInteger)count NS_SWIFT_NAME(setCandles(_:count:));
- (void)setHostActive:(BOOL)active NS_SWIFT_NAME(setHostActive(_:));
- (void)setScrollInteractionOptions:(KRScrollInteractionOptionsValue)options
    NS_SWIFT_NAME(setScrollInteractionOptions(_:));
- (void)handlePanBeganAtX:(CGFloat)x y:(CGFloat)y NS_SWIFT_NAME(handlePanBegan(atX:y:));
- (void)handlePanChangedByDeltaX:(CGFloat)deltaX
                          deltaY:(CGFloat)deltaY NS_SWIFT_NAME(handlePanChanged(deltaX:deltaY:));
- (void)handlePanEndedWithVelocityX:(CGFloat)velocityX
                           velocityY:(CGFloat)velocityY NS_SWIFT_NAME(handlePanEnded(velocityX:velocityY:));
- (void)handlePinchBeganAtAnchorRatio:(float)anchorRatio NS_SWIFT_NAME(handlePinchBegan(anchorRatio:));
- (void)handlePinchChangedScale:(CGFloat)scale
                    anchorRatio:(float)anchorRatio NS_SWIFT_NAME(handlePinchChanged(scale:anchorRatio:));
- (void)handlePinchEnded NS_SWIFT_NAME(handlePinchEnded());
- (void)handleLongPressBeganAtX:(CGFloat)x y:(CGFloat)y NS_SWIFT_NAME(handleLongPressBegan(atX:y:));
- (void)handleLongPressMovedToX:(CGFloat)x y:(CGFloat)y NS_SWIFT_NAME(handleLongPressMoved(toX:y:));
- (void)handleLongPressEnded NS_SWIFT_NAME(handleLongPressEnded());
- (void)clearCrosshair;
- (void)renderToMetalLayer:(CAMetalLayer*)metalLayer
                      size:(CGSize)size
                     scale:(CGFloat)scale NS_SWIFT_NAME(render(to:size:scale:));

@end

NS_ASSUME_NONNULL_END
