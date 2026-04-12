# Step 1 iOS Demo

本阶段只覆盖最小闭环：

- 用 `scripts/fetch_skia.sh` 自动拉取 Skia 源码。
- 用 `scripts/build_skia_ios_sim.sh` 生成 iOS Simulator 版 `libskia.a`。
- 用根目录 `CMakeLists.txt` 统一组织 `core`、`render` 和 `app/ios`。
- 用 `scripts/bootstrap.sh` 一次性拉起 Demo 工程并构建 `kairo_ios_demo.app`。

## 前置条件

- macOS + Xcode
- CMake 3.29+
- Python 3

默认会使用 Skia 自带的 `bin/fetch-gn` / `bin/fetch-ninja` 自动下载匹配版本的构建工具，不依赖本地 `depot_tools`。

## 一键启动

```bash
./scripts/bootstrap.sh
```

默认行为：

- 拉取 `src/third_party/skia`
- 构建 `src/third_party/skia/out/ios_sim/libskia.a`
- SDK 构建入口位于仓库根目录
- iOS Demo 构建入口位于 `app/ios`
- 生成 `build/ios_sim/kairo.xcodeproj`
- 构建 `build/ios_sim/app/ios/Debug-iphonesimulator/kairo_ios_demo.app`

如果你明确需要同步 Skia 的完整开发依赖，可以额外执行：

```bash
SKIA_SYNC_DEPS=1 ./scripts/fetch_skia.sh
```

如果你已经拉过 Skia，但想强制更新到新的 ref：

```bash
SKIA_UPDATE=1 SKIA_GIT_REF=main ./scripts/fetch_skia.sh
```

## 单独执行

```bash
./scripts/fetch_skia.sh
./scripts/build_skia_ios_sim.sh
cmake -S . -B build/ios_sim -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0
cmake --build build/ios_sim --config Debug --target kairo_ios_demo
```

## 运行方式

`bootstrap.sh` 会生成可运行的 Simulator App。后续可用两种方式打开：

- 直接用 Xcode 打开 `build/ios_sim/kairo.xcodeproj`，运行 `kairo_ios_demo`
- 用 `xcodebuild` / `simctl` 继续做自动安装与启动

## 约束

- 当前只构建宿主机架构对应的 Simulator 版本
- 当前 Demo 只使用 Skia CPU Raster，不包含 Metal / 手势 / 图层系统
- 当前默认不跑 `tools/git-sync-deps`，以避免最小闭环被上游限流拖住
