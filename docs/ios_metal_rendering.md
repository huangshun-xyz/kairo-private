# iOS Metal Rendering

这份文档描述当前 iOS 平台层为什么切到 `CAMetalLayer`，以及这条路径在 `kairo` 里的职责边界。

## 为什么不用每帧导出 UIImage

如果每次渲染都走下面这条链路：

```text
SkBitmap
  -> CGImage
  -> UIImage
  -> CALayer.contents
```

会有几个明显问题：

- 每帧都会有 CPU raster
- 每帧都会有额外像素拷贝
- 每帧都会创建 `CGImage` / `UIImage`
- 每帧都会做整图替换

这条路适合最小 demo，不适合后续高频交互的 K 线图。

## 当前选择

当前 iOS 平台层选择：

- 使用 `CAMetalLayer` 作为宿主 layer
- 使用 Skia 的 Metal Ganesh backend
- 通过 `SkSurfaces::WrapCAMetalLayer(...)` 直接把 `CAMetalLayer` 包装成 `SkSurface`

这样当前每一帧的路径更接近：

```text
KChart
  -> SkCanvas
  -> SkSurface( backed by CAMetalLayer drawable )
  -> present drawable
```

这比导出 `UIImage` 更适合后续继续扩交互和数据量。

## 当前职责分层

推荐把 iOS 渲染职责理解成三层：

```text
KRChartView.swift
  ↓
KRChartBridge.mm
  ↓
kairo::KChart
```

其中：

- `KRChartView` 负责 `UIView` 生命周期和手势
- `KRChartBridge` 负责 `CAMetalLayer / MTLDevice / GrDirectContext`
- `KChart` 负责图表语义、布局、series、交互状态

## 为什么 bridge 里持有 Metal 对象

这是平台宿主层的职责，不应该进入 `KChart`。

`KChart` 不应该知道：

- `CAMetalLayer`
- `MTLDevice`
- `MTLCommandQueue`
- `CAMetalDrawable`

这些都属于 iOS 平台细节。

所以更合理的做法是：

- `KChart` 只对 `SkCanvas` 作画
- bridge 决定这个 `SkCanvas` 背后是 CPU surface 还是 Metal surface

## 当前实现形态

当前 iOS 侧的关键点是：

1. `KRChartView` 的 backing layer 改成 `CAMetalLayer`
2. `KRChartBridge` 在初始化时创建：
   - `MTLDevice`
   - `MTLCommandQueue`
   - `GrDirectContext`
3. 每次需要重绘时：
   - 更新 `CAMetalLayer.drawableSize`
   - 用 `SkSurfaces::WrapCAMetalLayer(...)` 创建 surface
   - 让 `KChart` 画到 surface 上
   - `flushAndSubmit`
   - present drawable

## 对后续的意义

这条路对后续扩展很重要，因为它天然支持继续往下演进：

- 更高刷新频率
- 更复杂指标
- 更大的数据量
- 更少的中间对象创建

而且它仍然保持了边界清楚：

- public API 不感知 Metal
- bridge 只负责平台对象
- core/render 继续负责图表逻辑和绘制

## 下一步可能继续优化的点

虽然已经切到 `CAMetalLayer`，但后面还可以继续优化：

- 用脏区和局部重绘减少无效绘制
- 把交互更新节奏和 display link 结合
- 评估是否要引入 `CADisplayLink`
- 评估是否要为更复杂场景预留 Graphite 路线

## 一句话总结

当前 iOS 渲染路径的推荐方向是：

> 用 `CAMetalLayer` 作为原生宿主，用 Skia Metal backend 直接绘制到 drawable，上层继续保持 `KRChartView -> KRChartBridge -> KChart` 这条清晰边界，而不是回退到每帧导出 `UIImage` 的路径。
