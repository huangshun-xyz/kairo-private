# Public API Industry Proposal

这份文档不是当前实现说明，而是 `kairo` 后续对外 public API 的构思稿。

它的目标是：

- 参考业界成熟 K 线图方案整理一套更稳定的对外接口思路
- 解释为什么 `KChart` 应该成为统一入口
- 讨论主图 / 副图 / 叠加指标 / 自定义指标 / 交互 / 扩展点该怎么收口
- 给出接口草案和伪代码示例，供后续仓库演进时参考

这份文档里的很多接口还没有实现。

请把它理解成：

- 面向未来的 public API 设计提案
- 不是当前代码的最终定稿

如果你想看当前已经落地的最小版本，请先看：

- [docs/public_api_design.md](/Users/bytedance/Desktop/lark/kairo-private/docs/public_api_design.md)

## 为什么要单独写这份文档

`KChart` 这层 public API 后面会成为：

- iOS / macOS 的 Swift bridge 对接对象
- Android 的 JNI 对接对象
- 鸿蒙的 NAPI / NDK 对接对象
- Windows / 桌面宿主层的稳定入口

这意味着这层 API 一旦暴露出去，就会逐步变成整个仓库的对外契约。

所以它必须尽量参考成熟方案，而不是只根据当前最小 demo 的实现状态往前长。

## 参考了哪些成熟方案

这份文档主要参考了 3 类公开方案。

### 1. TradingView Lightweight Charts

它的特点是：

- 顶层有 `IChartApi`
- 系列统一通过 `addSeries(...)` / `addCustomSeries(...)` 暴露
- 多 pane 通过 `paneIndex` 和 `moveToPane(...)` 管理
- 插件能力区分 `custom series` 和 `primitives`

对我们最有价值的点：

- pane 是布局概念，不是渲染内部对象
- 自定义图形不一定都做成“新系列类型”，也可以做成依附某条 series 或 pane 的 primitive
- custom series 必须显式告诉图表 auto-scale / crosshair 应该怎么理解数据

### 2. Highcharts Stock

它的特点是：

- 技术指标基本都还是 `series`
- 指标通过 `linkedTo` 绑定主序列
- 可用 `yAxis` 决定指标是叠加主图还是独立副图
- 支持自定义技术指标，并要求实现 `getValues()`

对我们最有价值的点：

- 指标不一定要是特殊系统，很多时候它本质上还是一种 series
- 指标和主序列之间最好用稳定 id 建依赖，不要直接暴露内部指针引用
- 自定义指标最先应该开放“自定义计算”，而不是一上来就开放完全自定义渲染器

### 3. SciChart

它的特点是：

- 顶层是 `SCIChartSurface`
- 数据和渲染拆成 `DataSeries` + `RenderableSeries`
- 系列通过 `xAxisId` / `yAxisId` 绑定轴
- 交互通过 `ChartModifier`
- 标注通过 `Annotation`
- 自定义能力有 `Custom RenderableSeries`、`PointMarker`、`PaletteProvider`

对我们最有价值的点：

- chart / series / axis / modifier / annotation 是比较稳定的公开心智
- 交互能力应该单独建模，不要塞到 series 本身里
- 样式和绘制通道应该可以分层，而不是所有自定义都得接管整个渲染流程

## 从业界共性里抽出来的结论

把这些方案放在一起看，public API 最终应该收成下面几类概念：

1. `Chart`
2. `Series`
3. `Indicator`
4. `Pane`
5. `Modifier`
6. `Annotation`
7. `Custom Extension`

对应到 `kairo`，更推荐的心智是：

- `KChart` 是统一入口
- `Series` 是所有可视数据内容的统一抽象
- `Indicator` 是带依赖关系和默认布局策略的 series
- `Pane` 是布局对象，不是渲染对象
- `Modifier` 是交互行为
- `Annotation` 是附加标注
- `Custom Extension` 是给用户扩展的能力

## 推荐的 public API 心智模型

推荐用下面这套模型来思考对外 API：

```text
KChart
  ├─ MainSeries
  ├─ AdditionalSeries
  ├─ Indicators
  ├─ Panes
  ├─ Modifiers
  └─ Annotations
```

内部引擎依然可以继续是：

```text
Chart / Pane / Series / Layer / Overlay / Scale / Controller
```

但对外不应该直接暴露这棵内部对象树。

## 为什么主图和副图都建议按 series 处理

成熟方案里一个非常稳定的共识是：

- 主图 K 线是 series
- 成交量是 series
- MA / EMA / BOLL / BBI 也是 series
- MACD / RSI / KDJ 也是 series

区别不在于它是不是指标，而在于：

- 它依赖谁
- 它放在哪个 pane
- 它使用哪个 y-axis
- 它默认如何绘制

也就是说，public API 不应该按“业务名词”把系统拆碎。

更推荐这样收：

- `SeriesSpec` 表达“这是什么系列”
- `Placement` 表达“它放哪”
- `Dependency` 表达“它依赖谁”

## 一组更推荐的 public 对象

下面是更适合跨平台、多语言桥接的 public API 草案。

注意：

- 这是接口构思
- 不是当前实现状态

### 顶层入口

```cpp
namespace kairo {

class KChart {
 public:
  void SetOptions(const KChartOptions& options);
  void SetDataSource(std::shared_ptr<KDataSource> data_source);

  bool AddSeries(const KSeriesSpec& spec);
  bool AddIndicator(const KIndicatorSpec& spec);
  bool AddCustomIndicator(const KCustomIndicatorSpec& spec);

  bool RemoveObject(const std::string& object_id);
  bool MoveObject(const std::string& object_id, const KPlacement& placement);

  void SetVisibleRange(const KVisibleRange& range);
  void ScrollBy(double delta_logical);
  void ZoomBy(double scale_factor, float anchor_ratio);

  void AddModifier(const KModifierSpec& spec);
  void AddAnnotation(const KAnnotationSpec& spec);

  void SetListener(KChartListener* listener);
};

}  // namespace kairo
```

这里推荐把绝大多数业务入口都集中在 `KChart` 上。

原因是：

- 便于多平台 bridge 做薄映射
- 便于未来做事务更新和生命周期管理
- 便于把内部 graph 收在 public 之后

## 主图 / 副图的推荐接口

成熟 K 线图最小组合通常是：

- 一个价格主图
- 一个成交量副图

推荐 public API 不做专门的 `AddVolumePane()`，而是用统一的 `AddSeries(...) + placement` 表达。

### 推荐的 placement 定义

```cpp
enum class KPlacementKind {
  kMainPane,
  kNewPane,
  kExistingPane,
  kOverlayOnSeries,
};

struct KPlacement {
  KPlacementKind kind = KPlacementKind::kMainPane;
  std::string pane_id;
  std::string target_series_id;
  float pane_height = 96.0f;
};
```

这样价格和成交量可以统一表达成：

```cpp
KSeriesSpec price;
price.id = "price";
price.type = KSeriesType::kCandles;
price.placement.kind = KPlacementKind::kMainPane;

KSeriesSpec volume;
volume.id = "volume";
volume.type = KSeriesType::kVolume;
volume.placement.kind = KPlacementKind::kNewPane;
volume.placement.pane_id = "volume";
volume.placement.pane_height = 96.0f;

chart.AddSeries(price);
chart.AddSeries(volume);
```

这种设计和 Lightweight Charts 的 `paneIndex` / `moveToPane(...)` 思想很接近：

- 创建时指定 pane
- 后续允许移动 pane
- pane 是布局层，不是 series 内部细节

## MA 叠加主图，继续增加指标，推荐怎么设计

### 先区分两类指标

推荐在 public 层明确区分：

1. 叠加型指标
2. 独立 pane 型指标

叠加型指标通常包括：

- MA
- EMA
- BOLL
- BBI

独立 pane 型指标通常包括：

- MACD
- RSI
- KDJ
- Stochastic

但是这里有一个重要原则：

不要把指标名和布局硬编码绑死。

更推荐做法是：

- 指标定义里有默认布局建议
- 调用方仍然可以通过 placement 覆盖
- public API 负责校验这个 placement 是否允许

### 推荐的 indicator 定义

```cpp
enum class KIndicatorKind {
  kMA,
  kEMA,
  kBBI,
  kBOLL,
  kMACD,
  kRSI,
  kKDJ,
  kCustom,
};

struct KIndicatorParams {
  std::vector<double> values;
};

struct KIndicatorSpec {
  std::string id;
  KIndicatorKind kind = KIndicatorKind::kMA;
  std::string source_series_id;
  KPlacement placement;
  KIndicatorParams params;
  KStyle style;
};
```

### 推荐的使用伪代码

#### 1. MA 叠加主图

```cpp
KIndicatorSpec ma5;
ma5.id = "ma5";
ma5.kind = KIndicatorKind::kMA;
ma5.source_series_id = "price";
ma5.placement.kind = KPlacementKind::kOverlayOnSeries;
ma5.placement.target_series_id = "price";
ma5.params.values = {5};

chart.AddIndicator(ma5);
```

#### 2. 再加 MA10 / MA20

```cpp
KIndicatorSpec ma10;
ma10.id = "ma10";
ma10.kind = KIndicatorKind::kMA;
ma10.source_series_id = "price";
ma10.placement.kind = KPlacementKind::kOverlayOnSeries;
ma10.placement.target_series_id = "price";
ma10.params.values = {10};

KIndicatorSpec ma20;
ma20.id = "ma20";
ma20.kind = KIndicatorKind::kMA;
ma20.source_series_id = "price";
ma20.placement.kind = KPlacementKind::kOverlayOnSeries;
ma20.placement.target_series_id = "price";
ma20.params.values = {20};

chart.AddIndicator(ma10);
chart.AddIndicator(ma20);
```

这个模式和 Highcharts 的 `linkedTo: 'main-series'` 很接近：

- 指标通过 id 绑定主序列
- 主序列上可以挂多个指标

#### 3. MACD 默认放独立 pane

```cpp
KIndicatorSpec macd;
macd.id = "macd";
macd.kind = KIndicatorKind::kMACD;
macd.source_series_id = "price";
macd.placement.kind = KPlacementKind::kNewPane;
macd.placement.pane_id = "macd";
macd.placement.pane_height = 88.0f;
macd.params.values = {12, 26, 9};

chart.AddIndicator(macd);
```

这个模式和 Highcharts 用独立 `yAxis` 承载指标的思想一致。

### 一个更成熟的补充点

指标的定义不应该只有 `kind` 和 `params`。

建议内部还维护一份指标元信息：

```cpp
struct KIndicatorMeta {
  bool supports_overlay_on_price = false;
  bool supports_separate_pane = true;
  bool requires_volume_source = false;
  bool default_overlay_on_price = false;
};
```

这能让 public API 做这些事：

- 校验某指标能否叠加主图
- 自动给出默认 placement
- 校验是否需要 volume 数据
- 后续生成文档和 UI 配置更方便

## 自定义指标建议怎么开放

这是整个 public API 最难也最容易做重的一块。

参考 Highcharts、Lightweight Charts、SciChart 后，更推荐分 3 档开放。

### 第一档：自定义计算，复用内建绘制

这是最值得优先支持的能力。

调用方只提供：

- 数据源依赖
- 参数
- 计算函数
- 输出通道定义

推荐接口草案：

```cpp
struct KIndicatorOutputChannel {
  std::string id;
  KChannelType type = KChannelType::kLine;
  KStyle style;
};

struct KCustomIndicatorSpec {
  std::string id;
  std::vector<std::string> source_series_ids;
  KPlacement placement;
  std::vector<KIndicatorOutputChannel> channels;
  KCustomIndicatorProvider* provider = nullptr;
};

class KCustomIndicatorProvider {
 public:
  virtual ~KCustomIndicatorProvider() = default;
  virtual bool Compute(const KIndicatorComputeContext& ctx,
                       KIndicatorComputedResult* out) = 0;
};
```

适合场景：

- MA / EMA / BBI / BOLL
- MACD / RSI / KDJ
- 各种策略线、通道线

这和 Highcharts 自定义指标要求实现 `getValues()` 的思想最接近：

- 用户负责算值
- 框架负责承接系列语义和渲染

### 第二档：自定义 visual channels

用户不接管整个 renderer，只定义多个可视通道：

- line
- area
- histogram
- points
- text markers

例如：

```cpp
KCustomIndicatorSpec zone;
zone.id = "my-zone";
zone.source_series_ids = {"price"};
zone.channels = {
  {"main", KChannelType::kLine, line_style},
  {"top", KChannelType::kLine, red_style},
  {"bottom", KChannelType::kLine, green_style},
  {"points", KChannelType::kPoints, point_style},
};
```

适合用户想要的：

- 指标折线
- 指标圆点
- 多颜色
- 多条线
- 柱状通道

这一层非常重要，因为很多“自定义指标”实际上并不需要自定义底层 renderer。

### 第三档：自定义 series / primitive / modifier

只有当用户想做很特殊的图形或交互时，再开放这档能力。

推荐拆成三类：

1. `CustomSeries`
2. `SeriesPrimitive`
3. `PanePrimitive`

这个思路明显受 Lightweight Charts 影响：

- custom series 用来定义新 series 类型
- series primitive 用来附着在 series 上
- pane primitive 用来附着在 pane 上

对 `kairo` 来说更自然的命名可以是：

```cpp
class KCustomSeriesProvider { ... };
class KSeriesPrimitiveProvider { ... };
class KPanePrimitiveProvider { ... };
```

## 自定义指标一定要考虑的 4 个能力

这是业界方案里最容易被忽略、但很重要的点。

### 1. Auto-range 能力

Lightweight Charts 的 custom series 要求提供 `priceValueBuilder`。

原因是：

- 图表必须知道你的自定义数据怎么参与 auto-scale
- 也必须知道 crosshair 和 price line 该取哪个值

所以 `kairo` 后续如果开放自定义指标，建议用户至少能告诉框架：

- 当前 item 的 min value
- 当前 item 的 max value
- 当前 item 的 main display value

推荐抽象：

```cpp
struct KAutoscaleHint {
  double min_value = 0.0;
  double max_value = 0.0;
  double display_value = 0.0;
  bool valid = false;
};
```

### 2. 生命周期能力

Lightweight Charts 的 custom series 明确有 `destroy()`。

这个点很值得借鉴。

因为自定义对象可能会持有：

- 缓存
- 纹理
- 订阅
- 业务侧 context

所以 `kairo` 后续如果开放 custom provider，建议也有清理入口。

### 3. Hit-test / crosshair contribution

如果一个自定义指标能被用户看到，通常就意味着：

- 它可能要参与 tooltip
- 它可能要参与 crosshair snapshot
- 它可能要参与 hit-test

所以建议不要只开放 draw。

至少要考虑：

```cpp
struct KHitTestResult { ... };
struct KTooltipEntry { ... };
struct KCrosshairValueEntry { ... };
```

### 4. Whitespace / missing data

Lightweight Charts 的 custom series 专门有 `isWhitespace`。

这说明成熟方案非常重视“数据缺失时图表该怎么处理”。

`kairo` 后续建议也明确：

- 某个点是否为空
- 是否需要断线
- 是否参与 auto-range

## 建议把交互单独建模成 Modifier

SciChart 很值得借鉴的一点是：

- 缩放
- 拖动
- tooltip
- legend
- selection

都更倾向于放进 modifier 系统，而不是绑死在 series 上。

这对 `kairo` 的好处很大：

- 系列对象更纯粹
- 交互策略更容易组合
- 平台桥接更统一

推荐接口草案：

```cpp
enum class KModifierType {
  kPan,
  kPinchZoom,
  kCrosshair,
  kTooltip,
  kSelection,
  kLegend,
  kCustom,
};

struct KModifierSpec {
  std::string id;
  KModifierType type = KModifierType::kCrosshair;
  KModifierOptions options;
};
```

推荐使用伪代码：

```cpp
chart.AddModifier({.id = "pan", .type = KModifierType::kPan});
chart.AddModifier({.id = "zoom", .type = KModifierType::kPinchZoom});
chart.AddModifier({.id = "crosshair", .type = KModifierType::kCrosshair});
chart.AddModifier({.id = "tooltip", .type = KModifierType::kTooltip});
```

## 建议把标注单独建模成 Annotation

SciChart 把 annotation 独立成一整套系统，这也非常值得借鉴。

不是所有附加图形都应该做成 indicator 或 primitive。

例如：

- 画趋势线
- 画矩形区间
- 画价格标记
- 画文本说明

这些更适合归到 `Annotation`。

推荐接口草案：

```cpp
enum class KAnnotationType {
  kLine,
  kBox,
  kText,
  kPriceLabel,
  kMarker,
};

struct KAnnotationSpec {
  std::string id;
  KAnnotationType type = KAnnotationType::kLine;
  std::string pane_id;
  KAnnotationGeometry geometry;
  KStyle style;
};
```

## 事件系统建议也提早想清楚

业界成熟方案里，图表公开接口通常不只有“设置数据”，还会有很多订阅事件。

这部分如果不尽早想清楚，后面 bridge 层会越来越难做。

推荐至少预留：

```cpp
class KChartListener {
 public:
  virtual ~KChartListener() = default;

  virtual void OnVisibleRangeChanged(const KVisibleRange& range) {}
  virtual void OnCrosshairChanged(const KCrosshairSnapshot& snapshot) {}
  virtual void OnSelectionChanged(const KSelectionSnapshot& snapshot) {}
  virtual void OnRequestMoreHistory(const KHistoryRequest& request) {}
  virtual void OnLayoutChanged(const KLayoutSnapshot& snapshot) {}
};
```

### 为什么 `OnRequestMoreHistory(...)` 值得现在就想

这是一个很多最小 demo 都不会先做、但实际产品很关键的点。

成熟证券图表在滚动到左边界时，经常需要：

- 请求更多历史数据
- 先占位
- 数据回来后补齐

如果 public API 不考虑这个能力，后面会很别扭。

## 生命周期和依赖关系建议怎么设计

参考 Highcharts 的 `linkedTo` 思想，推荐：

- source / indicator 之间通过 id 建依赖
- 不让 indicator 直接长期持有 source 裸指针
- 删除 source 时，依赖 indicator 自动级联处理

推荐内部模型：

```text
KChartModel
  series_by_id
  indicator_by_id
  pane_by_id
  dependency_graph
```

推荐对外语义：

- `RemoveObject("price")` 时，默认级联删除 `ma5`, `ma10`, `macd`
- 或者根据策略把它们标成 invalid

这比把生命周期交给各平台 wrapper 自己管稳定得多。

## 推荐支持批量事务更新

这是一个现在很容易没想到，但后面非常有用的点。

图表里经常会有这种操作：

- 同时添加多条指标
- 同时移动 pane
- 同时改 theme 和 line style

如果每次都触发一次重建和重绘，平台层体验会很差。

所以后续建议考虑：

```cpp
class KChartUpdateScope {
 public:
  explicit KChartUpdateScope(KChart* chart);
  ~KChartUpdateScope();
};
```

伪代码：

```cpp
{
  KChartUpdateScope batch(&chart);
  chart.AddIndicator(ma5);
  chart.AddIndicator(ma10);
  chart.AddIndicator(ma20);
  chart.AddIndicator(macd);
  chart.SetTheme(dark_theme);
}
```

## 推荐把样式系统做成统一 `Style`，而不是到处散字段

成熟方案一般不会让每种对象都各写一套完全割裂的样式字段。

更推荐抽一组公共能力：

- line style
- fill style
- text style
- marker style
- axis style
- grid style

再由不同对象组合使用。

这能帮助：

- public API 保持一致
- native bridge 做轻量映射
- 主题系统更容易落地

## 下面这些点也值得提早考虑

这是结合业界方案后，我觉得很值得记下来、你们刚才讨论里暂时还没展开的点。

### 1. Formatter / localization

价格、时间、成交量通常需要：

- price formatter
- volume formatter
- time formatter

如果不作为 public API 明确暴露，后面平台层常常会各自做一套。

### 2. Empty pane 的自动回收

Lightweight Charts 明确提到：

- 如果某条 series 被移出 pane 后，pane 里已经没有 series，那么 pane 可以自动删除

这个行为很值得借鉴。

### 3. Pane resize / reorder

多 pane K 线图不仅要能“创建 pane”，还最好支持：

- 改高度
- 调顺序
- 隐藏 / 展开

### 4. Axis ownership

SciChart 的 `xAxisId` / `yAxisId` 提醒了一个重要点：

- series 和 axis 的绑定关系值得作为明确模型存在

即便当前 `kairo` 先不把 axis 暴露太多，也要给未来留空间。

### 5. Snapshot 比直接回调原始事件更稳

例如 crosshair 变化时，不推荐只回一个坐标点。

更推荐回一个 snapshot：

- 当前 logical index
- 当前时间
- 当前 pane
- 每个 series / indicator 的值

这样业务侧做 tooltip、联动、埋点都会更稳。

## 一份更完整的伪代码示例

下面给一份偏目标态的 public API 使用伪代码。

### 示例 1：价格 + 成交量 + MA + MACD

```cpp
using namespace kairo;

KChart chart;

chart.SetOptions(KChartOptions {
  .content_insets = {12, 12, 12, 12},
  .default_sub_pane_height = 96.0f,
});

chart.SetDataSource(price_data_source);

chart.AddSeries(KSeriesSpec {
  .id = "price",
  .type = KSeriesType::kCandles,
  .placement = {.kind = KPlacementKind::kMainPane},
});

chart.AddSeries(KSeriesSpec {
  .id = "volume",
  .type = KSeriesType::kVolume,
  .placement = {
    .kind = KPlacementKind::kNewPane,
    .pane_id = "volume",
    .pane_height = 96.0f,
  },
});

chart.AddIndicator(KIndicatorSpec {
  .id = "ma5",
  .kind = KIndicatorKind::kMA,
  .source_series_id = "price",
  .placement = {
    .kind = KPlacementKind::kOverlayOnSeries,
    .target_series_id = "price",
  },
  .params = {.values = {5}},
});

chart.AddIndicator(KIndicatorSpec {
  .id = "ma10",
  .kind = KIndicatorKind::kMA,
  .source_series_id = "price",
  .placement = {
    .kind = KPlacementKind::kOverlayOnSeries,
    .target_series_id = "price",
  },
  .params = {.values = {10}},
});

chart.AddIndicator(KIndicatorSpec {
  .id = "macd",
  .kind = KIndicatorKind::kMACD,
  .source_series_id = "price",
  .placement = {
    .kind = KPlacementKind::kNewPane,
    .pane_id = "macd",
    .pane_height = 88.0f,
  },
  .params = {.values = {12, 26, 9}},
});

chart.AddModifier({.id = "pan", .type = KModifierType::kPan});
chart.AddModifier({.id = "zoom", .type = KModifierType::kPinchZoom});
chart.AddModifier({.id = "crosshair", .type = KModifierType::kCrosshair});
chart.AddModifier({.id = "tooltip", .type = KModifierType::kTooltip});
```

### 示例 2：用户自定义指标，复用内建线和点

```cpp
class MySignalProvider final : public KCustomIndicatorProvider {
 public:
  bool Compute(const KIndicatorComputeContext& ctx,
               KIndicatorComputedResult* out) override {
    // 伪代码：
    // 1. 从 source 取价格数据
    // 2. 计算信号线和买卖点
    // 3. 写入 out
    return true;
  }
};

MySignalProvider provider;

chart.AddCustomIndicator(KCustomIndicatorSpec {
  .id = "my-signal",
  .source_series_ids = {"price"},
  .placement = {
    .kind = KPlacementKind::kOverlayOnSeries,
    .target_series_id = "price",
  },
  .channels = {
    {"signal-line", KChannelType::kLine, signal_style},
    {"signal-points", KChannelType::kPoints, point_style},
  },
  .provider = &provider,
});
```

### 示例 3：用户自定义 pane primitive

```cpp
class MyWatermarkPanePrimitive : public KPanePrimitiveProvider {
 public:
  void Draw(const KPanePrimitiveContext& ctx) override {
    // 伪代码：在 pane 背后画 watermark
  }
};

chart.AddPanePrimitive("main", std::make_unique<MyWatermarkPanePrimitive>());
```

## 对 `kairo` 当前设计最重要的一句话

如果把这份文档浓缩成一句话，那就是：

> `kairo` 后续对外 public API 最好围绕 `KChart` 统一收口，把主图、副图、叠加指标、独立 pane 指标、自定义指标、交互、标注都建模成稳定的业务语义，而不是把底层 `Chart / Pane / Series / Layer / Overlay` 直接暴露给 native 或业务层。

## 官方资料参考

以下是本提案主要参考的官方资料：

- TradingView Lightweight Charts `IChartApi`: https://tradingview.github.io/lightweight-charts/docs/api/interfaces/IChartApi
- TradingView Lightweight Charts panes: https://tradingview.github.io/lightweight-charts/tutorials/how_to/panes
- TradingView Lightweight Charts plugins intro: https://tradingview.github.io/lightweight-charts/docs/plugins/intro
- TradingView Lightweight Charts custom series: https://tradingview.github.io/lightweight-charts/docs/plugins/custom_series
- Highcharts Stock technical indicators: https://www.highcharts.com/docs/stock/technical-indicator-series
- Highcharts custom technical indicators: https://www.highcharts.com/docs/stock/custom-technical-indicators
- SciChart iOS `SCIChartSurface`: https://www.scichart.com/documentation/ios/current/Classes/SCIChartSurface.html
- SciChart iOS `ISCIRenderableSeries`: https://www.scichart.com/documentation/ios/current/Protocols/ISCIRenderableSeries.html
- SciChart iOS chart modifier APIs: https://www.scichart.com/documentation/ios/current/Chart%20Modifier%20APIs.html
- SciChart iOS annotations APIs: https://www.scichart.com/documentation/ios/current/Annotations%20APIs.html
