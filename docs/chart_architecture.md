# Chart Architecture

这份文档描述当前仓库里 chart 模块的设计思路。

目标不是把 API 一条条列出来，而是说明这套结构为什么这样拆、每层各自负责什么、以后应该沿着什么方向演进。

如果你关注的是下一阶段的“交互运行时、跨平台 timer / frame source、平台宿主边界”设计，请继续看：

- [docs/cross_platform_runtime_design.md](/Users/bytedance/Desktop/lark/kairo-private/docs/cross_platform_runtime_design.md)

当前代码对应的主要目录：

- `src/core`
- `src/render`

建议阅读顺序：

1. `src/core/chart.h`
2. `src/core/pane.h`
3. `src/core/render_context.h`
4. `src/core/series.h`
5. `src/core/layer.h`
6. `src/core/chart_overlay.h`
7. `src/render/demo_chart_factory.cc`

## 设计目标

当前这版 chart 主要解决下面几个问题：

- 支持典型证券图表的多 pane 纵向布局
- 所有 pane 共享一个横轴
- 每个 pane 有自己的纵轴
- 主图、副图、局部装饰、全局覆盖层分层清楚
- 不把“布局对象”和“绘制对象”混在一个抽象里

这套设计尤其针对下面这种场景：

- 上面是 K 线主图
- 下面是成交量
- 再下面可能是 MACD、RSI 等副图
- 十字线需要跨多个 pane 画一条共同的竖线

## 总体模型

当前结构可以概括成下面四层：

```text
Chart
  ├─ Pane
  │    ├─ Series
  │    └─ Layer
  └─ ChartOverlay
```

每层的职责如下。

### Chart

`Chart` 是顶层对象。

它负责：

- 保存 chart 的整体 bounds
- 保存 chart content inset
- 管理共享的 `Viewport`
- 管理共享的 `XScale`
- 持有多个 `Pane`
- 持有 chart 级别的 `ChartOverlay`
- 调度布局和绘制

它不负责：

- 直接持有具体业务数据
- 为单个 pane 计算 Y 轴范围
- 替 pane 决定里面有哪些 series 或 layer

一句话说，`Chart` 是全局容器和总调度器。

### Pane

`Pane` 表示 chart 里的一个垂直区域。

它负责：

- 保存自己的布局信息 `PaneLayout`
- 保存自己的 `frame_rect`
- 保存自己的 `content_rect`
- 持有自己的 `YScale`
- 持有本 pane 的 `Series`
- 持有本 pane 的 `Layer`
- 根据可见数据更新自己的 Y 轴范围

它不负责：

- 决定自己在 chart 里的纵向顺序
- 管理共享横轴
- 管理跨多个 pane 的覆盖物

一句话说，`Pane` 是局部坐标系和局部内容容器。

### Series

`Series` 是 pane 内的主业务内容。

当前它有三个核心能力：

- `Draw(RenderContext*)`
- `GetVisibleRange(const Viewport&)`
- `HitTest(...)`

最关键的是 `GetVisibleRange(...)`。

这意味着 `Series` 不只是会画，还参与 pane 的 auto-range 计算。也就是：

- 主图 K 线会影响主图 Y 轴
- 成交量柱子会影响成交量 pane 的 Y 轴

所以当前语义里，`Series` 更像“数据内容对象”。

### Layer

`Layer` 也是 pane 内对象，但更偏装饰或辅助。

当前它只有：

- `Draw(RenderContext*)`
- `HitTest(...)`
- `draw_order()`

它没有 `GetVisibleRange(...)`，所以默认不参与 auto-range。

这意味着 grid、局部标记线之类的对象可以放进 `Layer`，但它们不会改变 pane 的 Y 轴范围。

所以当前语义里，`Layer` 更像“局部附加绘制对象”。

### ChartOverlay

`ChartOverlay` 是 chart 级对象。

它和 `Layer` 最大的区别不是“是不是 overlay”，而是：

- `Layer` 是 pane-local
- `ChartOverlay` 是 chart-global

它适合承载：

- 跨 pane 的十字线
- 全局选择框
- 跨 pane 的 hover 辅助元素

当前的 `CrosshairOverlay` 就是这个层级。

## 为什么要有 ChartOverlay

这是当前设计里最重要的判断之一。

一开始很容易想到：十字线也是一层，那就把十字线继续做成 `Pane::AddLayer(...)` 不就好了。

但一旦 chart 进入多 pane 模式，这种做法就开始别扭：

- 十字线竖线应该贯穿所有 pane
- 十字线横线只属于当前活跃 pane
- 十字线的 X 坐标来自共享横轴
- 十字线的 Y 坐标来自某一个 pane 的纵轴

这说明十字线本质上不是“某个 pane 里的局部 layer”，而是“chart 级 overlay + 当前 active pane 的局部语义”。

所以当前设计里：

- `Pane::Layer` 负责 pane 内部覆盖
- `ChartOverlay` 负责跨 pane 覆盖

这比让 `AddPane` 支持添加任意 `Layer` 更清晰。

## 布局模型

当前 `Chart` 使用最小可用的垂直布局模型。

对应类型在 `src/core/pane_layout.h`：

- `Insets`
- `PaneSizeMode`
- `PaneLayout`

### PaneSizeMode

目前只支持两种高度模式：

- `kFixed`
- `kStretch`

含义很简单：

- `kFixed`：使用固定高度
- `kStretch`：吃掉剩余高度

这套模型刻意没有引入更复杂的 box layout 概念，比如：

- flex grow
- min/max height
- relative positioning
- overlay pane

原因是证券图表的第一版场景其实很稳定：

- 主图通常是 `kStretch`
- 成交量、副图通常是 `kFixed`

### frame_rect 和 content_rect

每个 pane 会保存两个矩形：

- `frame_rect`：pane 在 chart 中分到的外框
- `content_rect`：扣掉 pane inset 后真正绘制内容的区域

这样做的好处是：

- 布局信息和绘制信息都能表达清楚
- 以后如果要给 pane 加顶部标题区、轴标签区，会更容易扩展

### Layout 流程

`Chart::LayoutPanes()` 的逻辑可以概括为：

1. 先从 `bounds_` 扣掉 chart 自己的 `content_insets_`
2. 统计所有 fixed pane 的总高度
3. 计算 stretch pane 可分到的剩余高度
4. 从上到下逐个写入 pane 的 `frame_rect`
5. 再根据 pane 自己的 inset 算出 `content_rect`

这是一套刻意保持简单的垂直堆叠布局。

## 坐标系统

当前 chart 有两套核心 scale：

- `XScale`：chart 共享
- `YScale`：每个 pane 自己一套

含义是：

- 所有 pane 使用同一个横轴逻辑区间
- 每个 pane 对自己的数值空间负责

这很符合证券图表的直觉：

- K 线和成交量必须对齐同一根 bar 的 X 位置
- 但价格和成交量显然不能共用一个 Y 轴

## RenderContext 的作用

`RenderContext` 是 pane 内绘制上下文。

它现在会把下面这些东西一起传给 `Series` 和 `Layer`：

- `chart_bounds`
- `chart_content_bounds`
- `pane_frame`
- `pane_content_bounds`
- `x_scale`
- `y_scale`
- `viewport`

这里有一个容易误解的点：

- `Series` 虽然有显式 `GetVisibleRange(const Viewport&)`
- `Layer` 虽然没有这个接口
- 但在 `Draw(RenderContext*)` 阶段，二者其实都能读到 `ctx->viewport`

真正的语义差别是：

- `Series` 参与 auto-range
- `Layer` 不参与 auto-range

所以当前区分 `Series` 和 `Layer`，核心不是“谁能访问 viewport”，而是“谁有数据范围语义”。

## 当前绘制流程

`Chart::Draw()` 的执行顺序是：

1. `SyncScales()`
2. 遍历每个 pane
3. 先画 pane underlay layers
4. 再画 pane series
5. 再画 pane overlay layers
6. 所有 pane 画完后，再画 chart overlays

可以写成：

```text
for pane in panes:
  underlay layers
  series
  overlay layers

for overlay in chart_overlays:
  draw overlay
```

这个顺序背后的设计是：

- grid 这种背景元素在底部
- K 线、成交量这类主数据在中间
- pane 内局部装饰在数据上面
- chart 级覆盖物最后统一压顶

## Auto-range 的设计思路

当前 auto-range 放在 `Pane::UpdateAutoRange()`。

它只遍历 `series_`，对所有 `Series::GetVisibleRange(viewport)` 做合并，然后更新当前 pane 的 `YScale`。

这意味着：

- pane 的纵轴由本 pane 的 series 决定
- layer 不影响纵轴
- 不同 pane 的 Y 轴天然独立

这和常见的图表库设计比较一致。

## Demo 是怎么组装的

`src/render/demo_chart_factory.cc` 展示了当前推荐的组装方式：

- 创建一个 `Chart`
- 设置共享 `Viewport`
- 设置 chart content inset
- 添加一个主图 pane
- 添加一个成交量 pane
- 最后添加一个 `CrosshairOverlay`

当前 demo 的结构大致是：

```text
Chart
  Pane(price, stretch)
    GridLayer
    CandleSeries

  Pane(volume, fixed 96)
    GridLayer
    VolumeSeries

  CrosshairOverlay
```

这里有一个重要判断：

- `Volume` 是新 pane
- `BBI/MA` 这类与价格共轴的指标，通常应放在主图 pane 里做 `Series`

也就是说，“是否单独开 pane”主要看是否需要独立 Y 轴，而不是看它是不是指标。

## 这套设计想避免什么

当前设计有几个明确的边界。

### 不让 AddPane 兼任绘制入口

`AddPane` 只加 pane。

如果一个对象需要跨 pane 覆盖，就应该是 `ChartOverlay`，而不是“塞进 AddPane 的某个特殊 layer”。

### 不把布局对象和绘制对象混为一谈

`Pane` 是 layout node。

`Series`、`Layer`、`ChartOverlay` 是 render node。

如果这两类对象混在一个层级里，后面通常会出现大量语义模糊的问题。

### 不过早引入复杂布局

当前没有：

- overlay pane
- anchor pane
- 相对布局
- 自动内容高度

这些能力并不是永远不做，而是当前没有必要。

## 当前的优点

- 结构清楚，容易解释
- 很贴近证券图表常见场景
- 十字线的归属清晰
- 多 pane + 共享 X 轴 + 独立 Y 轴的模型稳定
- 以后加更多副图成本较低

## 当前的不足

也有几个目前还不够成熟的地方。

### Series 和 Layer 仍然是两套基类

这能保留“数据对象”和“装饰对象”的语义差异，但公开 API 确实多了一个概念。

后续一个可能的收敛方向是引入统一的 `PaneObject`，让：

- `Series`
- `Layer`

在实现层合并，但在能力上仍保留差异，比如：

- 是否参与 `GetVisibleRange`
- 绘制阶段
- 是否默认参与命中

### Crosshair 状态目前还在 overlay 自己内部

这对 demo 足够，但更成熟的方向是：

- 交互状态放在 `ChartController` 或独立 chart state
- overlay 只负责读取状态并绘制

### 交互链路还很薄

目前有最小的 `ChartController`，只处理：

- `ScrollBy`
- `ZoomBy`
- `SetVisibleRange`

后面如果要做完整交互，还需要把：

- pointer/touch 事件
- active pane 计算
- crosshair 更新
- hit-test 协调

再补完整。

## 适不适合继续开源演进

我认为适合。

原因不是它已经“足够完整”，而是它已经具备一个比较健康的骨架：

- `Chart` 管全局
- `Pane` 管局部
- `Series` 管数据内容
- `Layer` 管 pane 内装饰
- `ChartOverlay` 管跨 pane 覆盖

这比“所有东西都是 layer”或者“让 pane 同时承担布局和全局覆盖职责”更容易长期演进。

如果以后继续收敛，比较自然的方向是：

1. 把 crosshair 状态从 overlay 挪到 controller/state
2. 评估是否把 `Series` 和 `Layer` 收敛成统一 `PaneObject`
3. 补完整交互和命中测试链路
4. 增加轴、标签、tooltip 等更完整的展示层

## 一句话总结

当前 chart 的设计思路可以概括成一句话：

> `Chart` 负责全局横轴、纵向布局和跨 pane overlay，`Pane` 负责局部纵轴和局部内容，`Series`/`Layer` 负责 pane 内绘制，`ChartOverlay` 负责跨 pane 绘制。

如果你在读代码时抓住这条主线，后面很多具体实现都会变得很好理解。
