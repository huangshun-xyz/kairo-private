#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>

typedef struct KRCandleValue {
  double open;
  double high;
  double low;
  double close;
  double volume;
} KRCandleValue;

NS_ASSUME_NONNULL_BEGIN

@interface KRChartBridge : NSObject

@property(nonatomic, assign, getter=isCrosshairEnabled) BOOL crosshairEnabled;

- (void)resetToDefaultPriceVolumeLayout;
- (void)setCandles:(const KRCandleValue* _Nullable)candles
             count:(NSInteger)count NS_SWIFT_NAME(setCandles(_:count:));
- (void)scrollBy:(double)deltaLogical;
- (void)zoomBy:(double)scaleFactor anchorRatio:(float)anchorRatio;
- (void)updateCrosshairAtX:(float)x y:(float)y NS_SWIFT_NAME(updateCrosshairAt(x:y:));
- (void)clearCrosshair;
- (void)renderToMetalLayer:(CAMetalLayer*)metalLayer
                      size:(CGSize)size
                     scale:(CGFloat)scale NS_SWIFT_NAME(render(to:size:scale:));

@end

NS_ASSUME_NONNULL_END
