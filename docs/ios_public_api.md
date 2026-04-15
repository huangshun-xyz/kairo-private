# iOS Public API

这份文档描述当前 iOS 侧的 public 接入方式。

文件位置：

- [platform/ios/KRChartView.swift](/Users/bytedance/Desktop/lark/kairo-private/platform/ios/KRChartView.swift)
- [platform/ios/KRChartBridge.h](/Users/bytedance/Desktop/lark/kairo-private/platform/ios/KRChartBridge.h)
- [platform/ios/KRChartBridge.mm](/Users/bytedance/Desktop/lark/kairo-private/platform/ios/KRChartBridge.mm)

## 设计目标

iOS 层不是图表引擎本身。

它的目标是：

- 提供原生 Swift `UIView`
- 提供 Swift 侧易用的数据模型
- 把 UIKit 手势映射成 `KChart` 的 scroll / zoom / crosshair
- 用 `CAMetalLayer` 作为原生渲染宿主，避免每帧导出 `UIImage`

也就是说，iOS 层只做很薄的一层桥接。

## 公开对象

### KRCandle

`KRCandle` 是 iOS 侧的单条 K 线数据对象。

它包含：

- `open`
- `high`
- `low`
- `close`
- `volume`

### KRChartView

`KRChartView` 是 iOS 侧的原生 Swift chart view。

当前主要属性和方法有：

- `candles`
- `crosshairEnabled`
- `resetToDefaultPriceVolumeLayout`
- `reloadData`

这套接口刻意保持简单。

目前 iOS 业务方不需要理解：

- pane
- yScale
- overlay
- render context

这些都在 `KChart` 和更底层引擎里消化了。

### KRChartBridge

`KRChartBridge` 是一层内部 ObjC++ bridge。

它的职责很克制：

- 持有 `kairo::KChart`
- 持有 Metal device / queue / Skia Ganesh context
- 把 `CAMetalLayer` 包装成可绘制的 `SkSurface`
- 给 Swift view 提供少量可调用方法

它不是给业务方直接使用的主入口。

## 当前默认行为

`KRChartView` 默认使用 `KChart` 的价格 + 成交量布局。

也就是：

- 主图 pane 显示 candle
- 下方 pane 显示 volume
- 使用 `CAMetalLayer` 直接承载绘制结果
- 支持拖动滚动
- 支持双指缩放
- 支持长按 crosshair

## 交互映射

当前 UIKit 手势与 `KChart` 的关系如下：

- `UIPanGestureRecognizer` -> `ScrollBy(...)`
- `UIPinchGestureRecognizer` -> `ZoomBy(...)`
- `UILongPressGestureRecognizer` -> `UpdateCrosshair(...)` / `ClearCrosshair()`

这个设计背后的原则是：

- native view 只负责采集平台事件
- 业务交互状态仍然收在 public/core 层

## Demo 使用方式

当前 demo 在：

- [app/ios/MainView.swift](/Users/bytedance/Desktop/lark/kairo-private/app/ios/MainView.swift)

它的做法很简单：

1. `MainView` 直接继承 `KRChartView`
2. 初始化时喂一组 `KRCandle`
3. 其余 layout / render / gesture 逻辑全部复用 `KRChartView`

这也是后续推荐的接入方式。

## 后续 iOS 层如何继续扩

未来如果 public 层补充更多能力，例如：

- add indicator
- set theme
- custom tooltip
- subscribe selection event

iOS 层仍然建议保持同样原则：

- 增加少量直观的 Swift 方法
- 不把内部引擎对象直接抛给业务方
- 继续让 `KRChartView` 成为主要入口

## 一句话总结

iOS public API 当前的定位非常明确：

> 用一个原生 Swift `UIView` + `CAMetalLayer` 承载 chart，把 UIKit 生命周期和手势转发给 `KChart`，并在底部保留最薄的 ObjC++ bridge，而不是把底层 `core/render` 直接暴露给 iOS 业务层。
