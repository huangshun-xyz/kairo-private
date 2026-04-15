# Public API Design

这份文档描述 `kairo` 当前新增的 public API 设计。

目标不是替代 [chart_architecture.md](/Users/bytedance/Desktop/lark/kairo-private/docs/chart_architecture.md)，而是在现有 `core/render` 之上补出一层更稳定、更接近成熟 K 线 SDK 习惯的对外接口。

## 为什么要有 public 层

当前仓库原本已经有一套很清晰的底层分层：

- `src/core`
- `src/render`

其中：

- `core` 负责 chart、pane、viewport、scale、controller 这些内部对象
- `render` 负责 candle、volume、grid、crosshair 这些绘制实现

这套结构很适合继续扩展引擎，但并不适合直接暴露给业务层或各平台 wrapper。

原因是：

- native 层不应该直接理解 `Pane / Layer / YScale / ChartOverlay`
- 如果每个内部对象都做 wrapper，平台层会迅速变重
- internal graph 一旦对外暴露，后续重构成本会很高

所以本次新增了一层：

- `src/public`

这一层继续使用 `namespace kairo`，但职责变成：

- 面向业务提供稳定、低心智成本的 K 线图接口
- 对 `core/render` 做收口
- 为 iOS、Android、鸿蒙、Windows、macOS 提供统一语义

## 总体分层

当前推荐把项目理解成三层：

```text
public API
  ↓
core / render
  ↓
platform view bridge
```

具体对应仓库目录：

- `src/public`
- `src/core`
- `src/render`
- `platform/ios`

职责如下。

### src/public

这一层提供稳定的对外对象模型。

当前已经实现的核心入口是：

- `KChart`

它负责：

- 管理业务数据
- 管理 public 级别的 series 配置
- 负责默认的“价格主图 + 成交量副图”布局
- 提供 scroll / zoom / crosshair 这类业务常用能力
- 在内部把 public 配置翻译成 `Chart / Pane / Series / Overlay`

### src/core + src/render

这一层继续作为引擎内部实现。

它负责：

- chart/pane/layout/scale 的组织
- 系列绘制
- overlay 绘制
- 视口变化
- auto-range

public 不会把这些类原样暴露给业务方。

### platform/ios

这一层是薄桥接。

它负责：

- `UIView` 生命周期
- 手势转发
- 渲染 surface 宿主
- 将 OC/ObjC++ 对象映射到 `KChart`

它不负责：

- 自己管理 pane
- 自己拼装 series graph
- 自己维护 crosshair 状态

## 为什么参考业界要把 public 收成 Chart / Series / Pane

这套设计不是拍脑袋出来的，背后的参考主要来自几类成熟方案：

- TradingView Lightweight Charts
- Highcharts Stock
- SciChart

它们公开接口虽然细节不同，但有明显共识：

- 顶层一定有一个 chart/surface API
- 数据内容通常以 series 为单位暴露
- pane / axis / placement 是布局能力，不是渲染内部实现
- modifier / interaction 会单独建模
- 内部 renderer 不直接暴露给业务

你可以把 `KChart` 理解成 `createChart()/ChartSurface` 这一层语义在 kairo 里的落点。

## 当前 public API 的核心对象

文件位置：

- [src/public/kchart.h](/Users/bytedance/Desktop/lark/kairo-private/src/public/kchart.h)

### KCandleEntry

`KCandleEntry` 是 public 层的数据结构。

它包含：

- `open`
- `high`
- `low`
- `close`
- `volume`

这是因为当前最小闭环先围绕 K 线主图和成交量图展开。

### KSeriesSpec

`KSeriesSpec` 是 public 层对 series 的稳定描述。

当前它包含这些重要字段：

- `id`
- `title`
- `type`
- `placement`
- `visible`
- `style`

当前已实现的 series type：

- `kCandles`
- `kVolume`

这里刻意没有把 `CandleSeries / VolumeSeries / GridLayer / CrosshairOverlay` 暴露出去。

业务看到的是 “我加一条什么类型的 series，它放在哪里”，而不是 “我要直接 new 一个底层 renderer”。

### KPanePlacement

`KPanePlacement` 是这次设计里非常关键的一层抽象。

它对应成熟 K 线图里最常见的布局需求：

- 主图 pane
- 新建副图 pane
- 复用已有 pane
- 叠加到某个已有 series 所在 pane

当前已定义的 placement kind：

- `kMainPane`
- `kNewPane`
- `kExistingPane`
- `kOverlayOnSeries`

虽然这次代码只完整落地了 `价格 + 成交量` 这条链路，但接口已经按成熟图表 SDK 的扩展方向预留好了。

### KChart

`KChart` 是 public 层的主入口。

当前它负责：

- `SetData(...)`
- `ResetToDefaultPriceVolumeLayout()`
- `AddSeries(...)`
- `RemoveSeries(...)`
- `SetVisibleRange(...)`
- `ScrollBy(...)`
- `ZoomBy(...)`
- `UpdateCrosshair(...)`
- `ClearCrosshair()`
- `Draw(...)`

这套接口的语义有意贴近成熟图表库：

- chart 是顶层入口
- series 是内容对象
- pane placement 决定图是叠加还是分 pane
- crosshair / scroll / zoom 是 chart 级交互

## 当前已实现的最小 K 线图能力

本次真正落地的能力是：

1. 一个默认的主图价格 series
2. 一个默认的副图成交量 series
3. grid 背景
4. scroll
5. pinch zoom
6. long press crosshair

默认布局是：

```text
KChart
  Pane(main, stretch)
    CandleSeries

  Pane(volume, fixed)
    VolumeSeries

  CrosshairOverlay
```

这正是成熟 K 线 SDK 最常见的第一阶段形态。

## 为什么现在不直接开放内部对象

一个常见误区是：

“既然底层已经有 `Chart / Pane / Series / Layer`，那 public 直接把它们原样暴露就好了。”

这会带来几个问题：

- iOS/Android wrapper 会直接依赖内部类
- 业务方会学习过多引擎概念
- 后续一旦调整 internal graph，就会影响所有平台接口
- 生命周期管理会变复杂

所以当前 public API 的原则是：

- 暴露稳定业务语义
- 不暴露内部实现对象图

## 当前实现如何映射到内部引擎

`KChart` 内部并不是自己绘图。

它会把 public 配置翻译成：

- `Chart`
- `Pane`
- `CandleSeries`
- `VolumeSeries`
- `GridLayer`
- `CrosshairOverlay`

也就是说：

- public 负责描述“画什么”
- core/render 负责执行“怎么画”

这就是 public 层存在的价值。

## 为什么默认先做价格 + 成交量

这是行业里几乎所有 K 线图的最小稳定组合。

原因很简单：

- 价格和成交量共享同一套 X 轴
- 价格和成交量必须拆成不同 pane
- 这套结构天然验证 pane layout / shared viewport / independent y-scale

如果这组能力收不稳，后面的 MA、MACD、自定义指标都会更难。

## 后续指标该怎么沿着这套 API 扩

虽然这次还没有把 `MA / MACD / 自定义指标` 都实现掉，但接口方向已经按后续需要留好了。

推荐扩展路线如下。

### 叠加型指标

例如：

- MA
- EMA
- BOLL
- BBI

这类指标通常可以设计成：

- 继续作为 `series`
- `placement.kind = kOverlayOnSeries`
- `placement.target_series_id = "price"`

这样使用者的心智是：

- 我在价格图上叠加一条指标线

而不是：

- 我去 new 一个 pane 内部 layer

### 独立副图指标

例如：

- MACD
- RSI
- KDJ

这类指标通常应该放到独立 pane：

- `placement.kind = kNewPane`
- `placement.pane_height = ...`

如果产品想把某些指标叠加主图，也可以通过 placement 去控制，但是否允许、是否 warning，应该由 public 层做能力校验。

### 自定义指标

后续更推荐分两层开放：

1. 自定义计算
2. 内建绘制通道

也就是：

- 用户自己算输出值
- SDK 负责把输出值渲染成 line / histogram / points

这会比一开始就开放完全自定义 renderer 更容易维护，也更适合多平台统一。

## 当前设计的几个重要取舍

### 1. public 继续用 C++

这次没有把 public 设计成 C ABI。

原因是当前项目还是早期阶段，直接用一层收敛过的 C++ facade 更容易维护，也更容易和 `core/render` 对接。

但要注意：

这不等于把 internal C++ 直接暴露给外部。

public 仍然是一层刻意收敛过的 SDK 风格接口。

### 2. 不使用过于平台绑定的高级语法

因为目标平台包括：

- iOS
- Android
- 鸿蒙
- Windows
- macOS

所以 public 层没有设计成强依赖某种语言语法糖的接口，而是尽量使用：

- enum
- plain struct
- string id
- 顶层 facade class

这样更容易在多语言做薄映射。

### 3. 先做最小闭环，再扩自定义

这次实现没有贸然把指标系统全部做完。

原因不是不重要，而是先把下面这些稳定下来更有价值：

- public / core / platform 三层边界
- 价格 + 成交量的 pane 模型
- 交互如何走 public 层
- iOS 原生 view 如何只做薄桥接

## 一句话总结

当前 public API 的核心目标可以概括成一句话：

> 用 `KChart + SeriesSpec + PanePlacement` 这组稳定语义，对外表达成熟 K 线 SDK 常见能力；用 `core/render` 继续承载内部图表引擎实现；让各平台 native 只做薄薄的一层 view 和交互桥接。

如果你在继续阅读代码时抓住这条主线，后面无论是 MA、MACD 还是自定义指标，都会更容易落到同一套模型里。
