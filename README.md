# kairo-private

`kairo-private` 是一个最小跨平台图表 SDK 项目骨架。

当前阶段只实现 Step 1 的最小闭环：

- SDK 与 iOS Demo 分离构建
- 自动拉取并编译 Skia
- 产出一个可运行的 iOS Simulator Demo
- 在 `src/public` 提供面向业务的 `KChart` facade
- 在 `platform/ios` 提供原生 Swift `KRChartView`

当前还没有开始做 Android / Windows / macOS Demo、手势系统、多层图层系统和复杂 SDK 封装。

## 当前状态

已完成：

- `src/` 作为 SDK 本体，单独编译
- `app/ios/` 作为独立 Demo 工程，单独集成 SDK
- `src/render/` 中基于 Skia 的最小 K 线渲染
- `scripts/bootstrap.sh` 一键拉起 Skia、生成 iOS Demo 工程并构建 Demo

当前 Demo 使用 `CAMetalLayer + Skia Metal (Ganesh)` 路径，目标是先验证“SDK -> Skia -> iOS App”这条链路，并避免每帧导出 `UIImage` 的额外开销。

## 目录结构

```text
├── CMakeLists.txt
├── scripts/
│   ├── fetch_skia.sh
│   ├── build_skia_ios_sim.sh
│   └── bootstrap.sh
├── src/
│   ├── CMakeLists.txt
│   ├── cmake/
│   │   └── skia.cmake
│   ├── core/
│   ├── public/
│   ├── render/
│   └── third_party/
│       └── skia/
├── platform/
│   └── ios/
│       ├── KRChartBridge.h
│       ├── KRChartBridge.mm
│       └── KRChartView.swift
├── app/
│   └── ios/
│       ├── CMakeLists.txt
│       ├── AppDelegate.mm
│       ├── AppDelegate.h
│       ├── MainView.swift
│       ├── ViewController.mm
│       ├── ViewController.h
│       ├── kairo_ios_demo-Bridging-Header.h
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

## 构建 SDK

```bash
cmake --preset vscode-index-ios-sim --fresh
cmake --build build/vscode-index -j4
```

## 构建 iOS Demo

一键构建：

```bash
./scripts/bootstrap.sh
```

默认会完成这些事情：

- 拉取 `src/third_party/skia`
- 编译 `src/third_party/skia/out/ios_sim/libskia.a`
- 生成 `build/ios_sim/kairo_ios_demo.xcodeproj`
- 构建 `build/ios_sim` 下的 `kairo_ios_demo.app`

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
open build/ios_sim/kairo_ios_demo.xcodeproj
```

## 当前限制

- 当前只构建宿主机架构对应的 iOS Simulator 版本
- 当前 Demo 只验证最小绘制闭环，不代表最终架构
- 当前默认不跑 `tools/git-sync-deps`，避免最小闭环被上游限流拖住

## 进一步说明

Step 1 的详细记录见 [docs/step1_ios_demo.md](/Users/bytedance/Desktop/lark/kairo-private/docs/step1_ios_demo.md)。

开发环境和 VSCode 相关说明见 [docs/dev/vscode.md](/Users/bytedance/Desktop/lark/kairo-private/docs/dev/vscode.md)。

Public API 设计说明见 [docs/public_api_design.md](/Users/bytedance/Desktop/lark/kairo-private/docs/public_api_design.md)。

Public API 的业界参考提案见 [docs/public_api_industry_proposal.md](/Users/bytedance/Desktop/lark/kairo-private/docs/public_api_industry_proposal.md)。

iOS 对外接口说明见 [docs/ios_public_api.md](/Users/bytedance/Desktop/lark/kairo-private/docs/ios_public_api.md)。

iOS `CAMetalLayer` 渲染说明见 [docs/ios_metal_rendering.md](/Users/bytedance/Desktop/lark/kairo-private/docs/ios_metal_rendering.md)。
