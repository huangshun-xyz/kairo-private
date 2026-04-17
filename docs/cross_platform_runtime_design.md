# Cross-Platform Runtime Design

这份文档描述 `kairo` 下一阶段推荐采用的跨平台运行时设计。

目标不是修补当前 iOS demo，而是重构出一套更接近 Chromium 思路的运行时边界，让：

- 交互逻辑尽量收敛到内核
- 平台层只做宿主、事件采样和系统能力适配
- 业务层不直接接触动画 tick、物理推进、可视区同步等实现细节
- iOS / Android / 鸿蒙 / Windows / macOS 共享同一套交互语义

## 为什么要单独写这份文档

当前仓库虽然已经有：

- `src/core`
- `src/render`
- `src/public`
- `platform/ios`

但交互运行时还是偏“demo 形态”：

- view 自己持有平台 timer
- view 自己决定何时推进滚动动画
- iOS wrapper 暴露了较多细粒度交互方法
- 平台层需要回读可视区状态来维持动画

这类结构在单平台 demo 阶段能工作，但一旦进入：

- 多平台接入
- 更复杂手势
- 更多动画
- 更严格测试

维护成本会快速上升。

## 这次设计想解决什么

本次推荐重点解决 4 个问题：

1. 不再让业务层和 view 层管理动画生命周期。
2. 不再让平台 view 负责推进 scroll physics。
3. 把“时间、帧驱动、失效请求”做成统一运行时抽象。
4. 让平台差异只存在于 `FrameSource / TaskRunner / SurfaceHost` 这一层。

## Chromium 里值得借鉴的思路

Chromium 不是把所有“定时触发”都抽象成同一种 timer。

它通常会区分三件事：

1. `TickClock`
2. `Timer / TaskRunner`
3. `BeginFrameSource`

这三个概念对 `kairo` 非常重要。

### 1. TickClock: 时间来源独立出来

Chromium 的 `TickClock` 是一个单独接口，用来提供单调递增时间，便于测试替换。

参考：

- `base/time/tick_clock.h`
- `base/test/simple_test_tick_clock.h`

这背后的关键思想是：

- “当前时间”不是全局硬编码
- 逻辑层不直接依赖平台 API
- 测试可以用 fake clock 精确推进动画

对 `kairo` 的含义是：

- scroll physics
- fling decay
- spring settle
- kinetic animation

都不应该直接依赖 `CADisplayLink.timestamp`、`System.nanoTime()`、`std::chrono::steady_clock::now()`。

应该统一从内核注入的 `TickClock` 取时间。

### 2. Timer / TaskRunner: 通用延迟任务

Chromium 的 timer 设计是建立在 task runner 之上的。

参考：

- `base/timer/timer.h`

能借鉴的点：

- timer 是通用调度工具，不是动画专用工具
- timer 依赖 task runner，而不是自己绑死平台 run loop
- timer 的时间基准可以替换成 `TickClock`

对 `kairo` 的含义是：

- “延迟隐藏十字线”
- “hover debounce”
- “数据更新节流”
- “延迟重建布局”

这类需求可以用统一 timer 抽象处理。

但：

- 惯性滚动
- 回弹动画
- vsync 对齐绘制

不应该直接抽象成普通 repeating timer。

### 3. BeginFrameSource: 动画要按帧驱动，不按普通 timer 驱动

Chromium 的 compositor 使用 `BeginFrameSource` 给 observer 派发帧事件。

参考：

- `components/viz/common/frame_sinks/begin_frame_source.h`

最值得借鉴的点：

- 帧源是独立对象
- observer 订阅帧事件
- 没有 observer 时，frame source 应该停掉底层 timer/vsync
- 帧参数包含 `frame_time / deadline / interval / sequence`

这比“view 自己起一个 CADisplayLink 然后每帧手动调 step()”更稳定。

对 `kairo` 的含义是：

- scroll fling / spring animation 应该订阅 frame source
- 内核决定何时开始和停止动画
- 平台层只提供 frame source 的实现

## 核心结论

如果按 Chromium 思路重构，`kairo` 不应该只做一个“跨平台 timer”。

更合理的分层是：

1. `TickClock`
2. `TaskRunner`
3. `FrameSource`
4. `InvalidationSink`
5. `InteractionRuntime`

其中：

- `TickClock` 负责“现在几点了”
- `TaskRunner` 负责“什么时候执行一个普通任务”
- `FrameSource` 负责“什么时候来一帧动画时机”
- `InvalidationSink` 负责“请宿主尽快重绘”
- `InteractionRuntime` 负责“如何根据输入和帧推进修改 chart 状态”

## 推荐的新目录结构

推荐把仓库重构成下面这套分层：

```text
src/
  base/
    tick_clock.h
    task_runner.h
    timer.h
    frame_source.h
    invalidation_sink.h
    time_types.h

  runtime/
    chart_runtime.h
    chart_runtime.cc
    interaction_router.h
    interaction_router.cc
    scroll_animator.h
    scroll_animator.cc
    gesture_state.h
    dirty_flags.h

  core/
    ...

  render/
    ...

  public/
    ...

  platform/
    stub/
      stub_frame_source.cc
      stub_task_runner.cc
    ios/
      display_link_frame_source.h
      display_link_frame_source.mm
      main_run_loop_task_runner.h
      main_run_loop_task_runner.mm
    android/
      choreographer_frame_source.h
      choreographer_frame_source.cc
    harmony/
      vsync_frame_source.h
      vsync_frame_source.cc
    windows/
      composition_frame_source.h
      composition_frame_source.cc
```

这里的关键变化是：

- 平台实现允许进入 `src/`
- 但只允许出现在 `src/platform/*`
- `app/*` 只保留 demo app
- `public` 和业务 wrapper 不再直接持有平台 timer

## 推荐的职责边界

### src/base

只放跨平台基础抽象，不放 chart 业务。

推荐包含：

- `TickClock`
- `TaskRunner`
- `Timer`
- `FrameSource`
- `FrameObserver`
- `InvalidationSink`

它对应 Chromium 的 `//base` + 一部分 `BeginFrame` 思想。

### src/runtime

这是本次最重要的新层。

它负责：

- 接收标准化输入事件
- 驱动 scroll / zoom / crosshair 等交互状态机
- 订阅 frame source 推进动画
- 决定何时请求重绘
- 决定何时停止动画

它不负责：

- 具体 K 线绘制
- Metal / OpenGL / Vulkan / Skia surface
- UIKit / Android View 生命周期

一句话说：

- `runtime` 负责“行为”
- `core/render` 负责“图表对象和绘制”

### src/core + src/render

继续负责：

- chart graph
- pane / series / scale
- auto range
- draw

但不应该继续增长平台动画逻辑。

### src/public

只暴露稳定业务语义，不暴露 runtime 推进细节。

对外应该更像：

- 设置数据
- 设置样式
- 设置交互配置
- 接收高层业务命令

不应该暴露：

- `stepScrollAnimation()`
- `beginScrollGesture()`
- `visibleRange() -> 仅为了平台层同步动画而暴露`
- `stopScrollAnimation() -> 仅为了 view 逻辑兜底而暴露`

### src/platform

只做系统能力适配。

比如：

- iOS: `CADisplayLink`
- Android: `Choreographer`
- Windows: `CompositionTarget` 或宿主渲染循环
- Harmony: 平台 vsync 回调

平台层不应该再自己保存 scroll animation 状态机。

## 推荐的抽象接口

下面是更接近目标形态的一组接口草案。

### 时间和帧

```cpp
namespace kairo {

struct KTimeDelta {
  int64_t micros = 0;
};

struct KTimeTicks {
  int64_t micros = 0;
};

class KTickClock {
 public:
  virtual ~KTickClock() = default;
  virtual KTimeTicks NowTicks() const = 0;
};

struct KFrameArgs {
  KTimeTicks frame_time;
  KTimeTicks deadline;
  KTimeDelta interval;
  uint64_t sequence = 0;
};

class KFrameObserver {
 public:
  virtual ~KFrameObserver() = default;
  virtual void OnFrame(const KFrameArgs& args) = 0;
};

class KFrameSource {
 public:
  virtual ~KFrameSource() = default;
  virtual void AddObserver(KFrameObserver* observer) = 0;
  virtual void RemoveObserver(KFrameObserver* observer) = 0;
};

class KTaskRunner {
 public:
  virtual ~KTaskRunner() = default;
  virtual void PostTask(void (*task)(void*), void* context) = 0;
  virtual void PostDelayedTask(KTimeDelta delay, void (*task)(void*), void* context) = 0;
};

class KInvalidationSink {
 public:
  virtual ~KInvalidationSink() = default;
  virtual void RequestRedraw() = 0;
};

}  // namespace kairo
```

### Runtime Host

```cpp
namespace kairo {

struct KRuntimeHost {
  KTickClock* tick_clock = nullptr;
  KTaskRunner* task_runner = nullptr;
  KFrameSource* frame_source = nullptr;
  KInvalidationSink* invalidation_sink = nullptr;
};

}  // namespace kairo
```

这层是整个设计的关键收口点。

`KChart` 或 `KChartRuntime` 只依赖 `KRuntimeHost`，不依赖 UIKit / Android / Windows API。

### Runtime

```cpp
namespace kairo {

class KChartRuntime : public KFrameObserver {
 public:
  explicit KChartRuntime(KChart* chart);

  void BindHost(const KRuntimeHost& host);
  void UnbindHost();

  void OnPanBegin(float x, float y);
  void OnPanUpdate(float delta_x, float delta_y);
  void OnPanEnd(float velocity_x, float velocity_y);

  void OnPinchBegin(float anchor_x, float anchor_y);
  void OnPinchUpdate(float scale, float anchor_x, float anchor_y);
  void OnPinchEnd();

  void OnLongPressBegin(float x, float y);
  void OnLongPressMove(float x, float y);
  void OnLongPressEnd();

  void OnFrame(const KFrameArgs& args) override;

 private:
  void StartAnimation();
  void StopAnimation();
  void RequestRedraw();
};

}  // namespace kairo
```

这里的关键点是：

- pan begin/update/end 是输入语义
- frame tick 是运行时语义
- redraw 是宿主语义

三者明确分离。

## 推荐的内部对象关系

```text
Business API / View
  ↓
Platform Wrapper
  ↓
KChartRuntime
  ├─ ScrollAnimator
  ├─ ZoomController
  ├─ CrosshairController
  └─ KChart
       ├─ core
       └─ render
```

职责建议如下：

### ScrollAnimator

负责：

- drag 跟手
- fling 启动
- bounce settle
- frame tick 推进

不负责：

- 平台 timer 生命周期
- UIKit 手势状态判断

### ZoomController

负责：

- pinch anchor
- scale factor clamp
- viewport 缩放语义

### CrosshairController

负责：

- active pane
- crosshair visibility
- long press 行为

### KChartRuntime

负责把多个交互控制器协调起来，并决定：

- 哪些状态互斥
- 哪些交互能并发
- 哪些变化需要 redraw

## 为什么业务层不应该看到细粒度动画接口

当前这类接口不适合长久保留在 host/public 里：

- `beginScrollGesture`
- `scrollGestureByPixels`
- `endScrollGestureWithVelocityPixelsPerSecond`
- `stepScrollAnimation`
- `isScrollAnimationActive`
- `stopScrollAnimation`

原因不是它们不能工作，而是它们泄露了内核实现结构：

- 业务层知道了动画有“begin/update/end/tick/stop”这些阶段
- 平台 view 被迫了解 animation lifecycle
- 后续一旦把惯性滚动从 step 改成 scheduler 驱动，接口又要重写

更推荐的方式是：

- 业务层只看到 `KRChartView`
- platform host 只看到高层输入事件
- runtime 自己管理 animation subscription

## iOS / Android / Windows 各自应该做什么

### iOS

推荐实现：

- `DisplayLinkFrameSource`
- `MainRunLoopTaskRunner`

不推荐：

- `KRChartView` 自己持有 `CADisplayLink`
- `KRChartView` 自己推进 scroll animation

### Android

推荐实现：

- `ChoreographerFrameSource`
- `LooperTaskRunner`

### Windows

推荐实现：

- `CompositionFrameSource` 或宿主渲染循环适配器
- `DispatcherTaskRunner`

### Harmony

推荐实现：

- 平台 vsync 回调适配器
- 主线程 task runner 适配器

## 推荐的运行时时序

### 拖拽阶段

```text
UIPanGestureRecognizer / Android GestureDetector
  ↓
Platform Wrapper
  ↓
KChartRuntime::OnPanBegin / OnPanUpdate
  ↓
ScrollAnimator updates viewport immediately
  ↓
KChartRuntime::RequestRedraw()
  ↓
Host redraws chart
```

### 松手后的惯性滚动

```text
Platform Wrapper
  ↓
KChartRuntime::OnPanEnd(velocity)
  ↓
ScrollAnimator starts ballistic state
  ↓
KChartRuntime subscribes to KFrameSource
  ↓
FrameSource emits OnFrame(args)
  ↓
ScrollAnimator advances physics
  ↓
RequestRedraw()
  ↓
Animation settles
  ↓
KChartRuntime unsubscribes from KFrameSource
```

这就是 Chromium 的 `BeginFrameSource + Observer` 思想在 `kairo` 里的对应关系。

## 为什么这比“跨平台 repeating timer”更好

如果只做一个统一 repeating timer，会有几个问题：

- 很难和 vsync 对齐
- 不同平台 timer 精度差异大
- 会产生无谓空转
- 不利于后续渲染节流

而 `FrameSource` 的优点是：

- 动画天然按帧推进
- 没有 observer 时可停机
- 平台层可以接入原生 vsync
- 测试时可替换成 synthetic frame source

所以推荐结论是：

- 要有 timer 抽象
- 但动画主抽象不应该叫 timer
- 动画主抽象应该叫 `FrameSource`

## 这次重构后 public API 应该收成什么样

推荐 `public` 层只保留：

- chart 数据
- 样式 / series / placement
- interaction config
- 宿主绑定
- draw

例如：

```cpp
class KChart {
 public:
  void SetRuntimeHost(const KRuntimeHost& host);
  void SetData(std::vector<KCandleEntry> candles);
  void SetInteractionOptions(const KInteractionOptions& options);

  void HandlePointerEvent(const KPointerEvent& event);
  void HandlePinchEvent(const KPinchEvent& event);
  void HandleLongPressEvent(const KLongPressEvent& event);

  void Draw(KCanvas& canvas);
};
```

其中：

- `HandlePointerEvent(...)` 是输入语义
- `SetRuntimeHost(...)` 是宿主语义
- `Draw(...)` 是渲染语义

而不是：

- begin
- update
- end
- tick
- stop

## 对当前仓库的直接替换建议

### 1. 新增 `src/base`

优先引入下面 5 个头文件：

- `tick_clock.h`
- `task_runner.h`
- `timer.h`
- `frame_source.h`
- `invalidation_sink.h`

### 2. 新增 `src/runtime`

优先引入：

- `chart_runtime.h`
- `scroll_animator.h`
- `interaction_router.h`

### 3. 把当前 scroll animation 生命周期从 `KRChartView` 挪走

当前问题不是物理公式本身，而是：

- animation subscription 在 view 里
- visible range sync 在 view 里
- platform wrapper 知道过多 runtime 细节

这部分应该先迁到 `KChartRuntime`。

### 4. iOS 只保留一个薄适配器

推荐 iOS wrapper 最终只负责：

- 手势识别
- 事件标准化
- 提供 `DisplayLinkFrameSource`
- 提供 `InvalidationSink`
- 提供渲染 surface

### 5. host 对外接口收缩

推荐最终移除这类“仅为当前实现服务”的 host 方法：

- `visibleRange`
- `beginScrollGesture`
- `scrollGestureByPixels`
- `endScrollGestureWithVelocityPixelsPerSecond`
- `stepScrollAnimation`
- `isScrollAnimationActive`
- `stopScrollAnimation`

## 测试策略

Chromium 风格设计的另一个重要价值是更好测。

推荐在 runtime 层引入：

- `FakeTickClock`
- `SyntheticFrameSource`
- `FakeTaskRunner`
- `FakeInvalidationSink`

这样就能写出稳定测试：

- fling 在 120ms 后的 viewport
- overscroll 回弹是否在边界停稳
- 拖拽中断是否正确取消 ballistic animation
- pinch 与 pan 互斥是否正确

而不需要依赖真机 run loop。

## 迁移顺序

推荐按下面 5 步做，不要一次性大爆炸替换。

1. 引入 `src/base` 抽象，但先不替换现有 API。
2. 引入 `src/runtime::KChartRuntime`，把 scroll animation 从 view 搬进去。
3. 在 iOS 上实现 `DisplayLinkFrameSource` 和 `InvalidationSink`。
4. 收缩 host/public 的细粒度动画接口。

## 命名建议

为了避免内部适配层退化成“机械转发 wrapper”，更推荐用 `Host` 而不是 `Bridge` 命名。

例如：

- iOS: `KRIOSChartHost`
- Android: `KRAndroidChartHost`
- Harmony: `KRHarmonyChartHost`

这样命名更符合职责：

- 它是平台宿主适配器
- 它不是对外 API
- 它负责装配 runtime host 能力
5. 再让 Android / Harmony / Windows 接入各自 frame source。

## 最终推荐结论

如果按“Chromium 风格跨平台设计”来做，`kairo` 的正确方向不是：

- 在每个平台 view 里各写一套动画循环

而是：

- 内核定义 `TickClock + TaskRunner + FrameSource + InvalidationSink`
- runtime 统一管理交互和动画
- 平台只提供宿主实现
- business API 只看到稳定 chart/view 语义

这套设计更适合长期演进，也更符合你说的：

- 平台可以封装
- 逻辑收敛到内核
- 业务层不暴露过多实现细节
- 多平台长期可维护
