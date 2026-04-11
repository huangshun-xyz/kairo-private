# kairo-private

`kairo-private` 是一个最小跨平台图表渲染项目骨架。

当前阶段只实现 Step 1 的最小闭环：

- 用 CMake 统一组织工程
- 自动拉取并编译 Skia
- 产出一个可运行的 iOS Simulator Demo

当前还没有开始做 Android / Windows / macOS Demo、手势系统、多层图层系统和复杂 SDK 封装。

## 当前状态

已完成：

- `core/` 中的最小图表数据结构
- `render/` 中基于 Skia 的最小柱状图渲染
- `app/ios/` 中的 iOS Demo App
- `scripts/bootstrap.sh` 一键拉起 Skia、生成工程并构建 Demo

当前 Demo 使用 Skia CPU Raster 路径，目标是先验证“数据 -> Skia -> iOS App”这条链路。

## 目录结构

```text
├── CMakeLists.txt
├── cmake/
│   └── skia.cmake
├── scripts/
│   ├── fetch_skia.sh
│   ├── build_skia_ios_sim.sh
│   └── bootstrap.sh
├── third_party/
│   └── skia/
├── core/
│   ├── CMakeLists.txt
│   ├── chart.h
│   └── chart.cc
├── render/
│   ├── CMakeLists.txt
│   ├── skia_renderer.h
│   └── skia_renderer.cc
├── app/
│   └── ios/
│       ├── CMakeLists.txt
│       ├── AppDelegate.mm
│       ├── AppDelegate.h
│       ├── ViewController.mm
│       ├── ViewController.h
│       ├── MainView.mm
│       ├── MainView.h
│       ├── main.mm
│       └── Info.plist
└── docs/
    └── step1_ios_demo.md
```

## 依赖要求

- macOS
- Xcode
- CMake 3.29+
- Python 3

默认会使用 Skia 自带的 `bin/fetch-gn` 和 `bin/fetch-ninja` 自动下载匹配版本工具，不依赖本地 `depot_tools`。

## 快速开始

一键构建：

```bash
./scripts/bootstrap.sh
```

默认会完成这些事情：

- 拉取 `third_party/skia`
- 编译 `third_party/skia/out/ios_sim/libskia.a`
- 生成 `build/ios_sim/kairo.xcodeproj`
- 构建 `build/ios_sim/app/ios/Debug-iphonesimulator/kairo_ios_demo.app`

如果只想单独执行 Skia 拉取或编译：

```bash
./scripts/fetch_skia.sh
./scripts/build_skia_ios_sim.sh
```

如果你已经拉过 Skia，但想强制更新：

```bash
SKIA_UPDATE=1 SKIA_GIT_REF=main ./scripts/fetch_skia.sh
```

如果你明确需要同步 Skia 的完整开发依赖：

```bash
SKIA_SYNC_DEPS=1 ./scripts/fetch_skia.sh
```

## 运行 iOS Demo

先构建：

```bash
./scripts/bootstrap.sh
```

再启动模拟器并安装运行：

```bash
open -a Simulator
xcrun simctl boot "iPhone 16 Pro"
xcrun simctl install booted /Users/bytedance/Desktop/lark/kairo-private/build/ios_sim/app/ios/Debug-iphonesimulator/kairo_ios_demo.app
xcrun simctl launch booted com.kairo.demo
```

也可以直接用 Xcode 打开：

```bash
open build/ios_sim/kairo.xcodeproj
```

## 当前限制

- 当前只构建宿主机架构对应的 iOS Simulator 版本
- 当前 Demo 只验证最小绘制闭环，不代表最终架构
- 当前默认不跑 `tools/git-sync-deps`，避免最小闭环被上游限流拖住

## 进一步说明

Step 1 的详细记录见 [docs/step1_ios_demo.md](/Users/bytedance/Desktop/lark/kairo-private/docs/step1_ios_demo.md)。

开发环境和 VSCode 相关说明见 [docs/dev/vscode.md](/Users/bytedance/Desktop/lark/kairo-private/docs/dev/vscode.md)。
