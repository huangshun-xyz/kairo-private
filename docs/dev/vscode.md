# VSCode Setup

这份文档只解释 VSCode 的工程索引和跳转配置。

注意：

- 真正会被 VSCode 和 CMake 自动识别的配置文件，必须留在仓库根目录
- 文档说明放在 `docs/`，方便后续扩展到 Android / Windows / macOS

当前相关配置文件：

- [CMakePresets.json](/Users/bytedance/Desktop/lark/kairo-private/CMakePresets.json)
- [.vscode/settings.json](/Users/bytedance/Desktop/lark/kairo-private/.vscode/settings.json)
- [.vscode/extensions.json](/Users/bytedance/Desktop/lark/kairo-private/.vscode/extensions.json)

## 推荐扩展

- `llvm-vs-code-extensions.vscode-clangd`
- `ms-vscode.cmake-tools`

当前默认依赖 `clangd` 提供：

- 语义高亮
- 跳转到定义
- 查找引用
- 补全

不建议把主要语义能力交给 `ms-vscode.cpptools`，因为这个项目同时包含 `.cc` 和 `.mm`，`clangd` 的结果更稳定。

## 首次初始化

先保证 Step 1 的 iOS 工程已经能构建：

```bash
./scripts/bootstrap.sh
```

然后生成给 VSCode 使用的编译数据库：

```bash
cmake --preset vscode-index-ios-sim
```

这条命令会生成：

```bash
build/vscode-index/compile_commands.json
```

生成完成后，在 VSCode 里执行一次 `Developer: Reload Window`。

## 工作原理

当前 preset 会做这些事情：

- 使用 `Ninja`
- 输出到 `build/vscode-index`
- 打开 `CMAKE_EXPORT_COMPILE_COMMANDS`
- 按 iOS Simulator 的编译参数生成索引数据库

`clangd` 会直接读取：

```bash
build/vscode-index/compile_commands.json
```

因此 `.cc`、`.mm` 以及相关头文件的语义信息会和真实构建参数保持一致。

## 何时重新生成

遇到下面这些情况时，重新执行一次：

```bash
cmake --preset vscode-index-ios-sim
```

- 修改了 `CMakeLists.txt`
- 新增了源码文件
- 调整了 include 路径
- 修改了 iOS 相关编译参数

## 后续扩展

后面如果增加 Android / Windows / macOS：

- 继续把平台无关的 VSCode 基础配置放根目录
- 在 `CMakePresets.json` 里新增对应平台的 index preset
- 把说明文档继续放到 `docs/` 下
