# HPM MCU：SEGGER Embedded Studio + VSCode + Codex 开发工作流

本文档用于约定 HPMicro HPM 系列 MCU 项目的推荐开发方式：

- **SEGGER Embedded Studio for RISC-V（SES）**：作为主 IDE，负责工程打开、构建、下载、原生 J-Link 调试。
- **VSCode**：作为辅助编辑器，负责代码阅读、批量修改、CMake/clangd 索引。
- **Codex**：作为代码助手，负责理解工程、重构、补全、生成代码、辅助排错。
- **CMake**：作为工程真实来源。SES 工程文件由 CMake 生成，不作为长期维护的主工程文件。

核心原则：

> CMake 是工程真相；SES 是硬件调试权威；VSCode + Codex 是智能编辑层。

---

## 1. 推荐目录结构

建议将 HPM SDK、用户工程、构建目录分离：

```text
D:/hpm/
  sdk_env/
    hpm_sdk/
    start_cmd.cmd

  workspace/
    my_app/
      CMakeLists.txt
      app.yaml

      src/
        main.c
        app.c

      include/
        app.h

      boards/
        my_board/

      scripts/
        build_debug.bat

      build/
        compile_commands.json
        segger_embedded_studio/
          xxx.emProject

      hpm_sdk_localized/

      AGENTS.md

      .vscode/
        settings.json

      my_app.code-workspace
```

建议：

- 不要把用户 App 直接放进 HPM SDK 目录内部。
- 不要长期手动维护 `build/segger_embedded_studio/*.emProject`。
- 不要手动拼大量 include path。
- 不要让 Codex 直接大面积修改 SDK 文件。
- 推荐将当前工程实际依赖的 SDK 子集本地化到 `hpm_sdk_localized/`。

---

## 2. 分工边界

### 2.1 SES 负责

SES 负责和硬件强相关的事情：

- 打开 HPM CMake 生成的 `.emProject`
- 编译工程
- 下载程序
- 使用 SEGGER 原生 J-Link 调试
- 单步、断点、寄存器、反汇编、Memory Watch
- 必要时查看 Linker、Section、Map、符号

### 2.2 VSCode 负责

VSCode 作为辅助编辑器使用：

- 代码阅读
- 全局搜索
- clangd / IntelliSense 跳转
- CMake 配置查看
- 多文件编辑
- Git 操作
- 调用构建脚本
- 配合 Codex 进行代码修改

### 2.3 Codex 负责

Codex 负责软件工程层面的工作：

- 理解现有代码
- 生成模块代码
- 重构
- 查找潜在 bug
- 解释 HPM SDK 调用关系
- 辅助修改 `CMakeLists.txt`
- 辅助修改 board 配置
- 辅助分析编译报错

Codex 不应该默认接管：

- J-Link 调试器配置
- SES 工程生成物
- OpenOCD / CMSIS-DAP 配置
- 大面积 SDK 源码修改

---

## 3. HPM CMake 工程基础

用户 App 的 `CMakeLists.txt` 建议保持简洁：

```cmake
cmake_minimum_required(VERSION 3.13)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(my_hpm_app)

sdk_app_src(src/main.c)
sdk_app_src(src/app.c)

sdk_app_inc(include)
sdk_app_inc(src)

generate_ide_projects()
```

如果有自定义 board：

```cmake
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_LIST_DIR}/boards)
```

示例：

```cmake
cmake_minimum_required(VERSION 3.13)

set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_LIST_DIR}/boards)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(my_hpm_app)

sdk_app_src(src/main.c)
sdk_app_src(src/app.c)

sdk_app_inc(include)
sdk_app_inc(src)

generate_ide_projects()
```

---

## 4. 生成工程

建议从 HPM SDK_ENV 的环境命令行启动。

Windows 示例：

```bat
D:\hpm\sdk_env\start_cmd.cmd
```

进入项目目录：

```bat
cd D:\hpm\workspace\my_app
```

配置工程：

```bat
cmake -S . -B build -G Ninja ^
  -DBOARD=hpm6750evkmini ^
  -DHPM_BUILD_TYPE=flash_xip ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

构建工程：

```bat
cmake --build build
```

生成物中通常会包含：

```text
build/
  compile_commands.json
  segger_embedded_studio/
    xxx.emProject
```

其中：

- `compile_commands.json` 给 clangd、VSCode、Codex 理解编译参数。
- `.emProject` 给 SES 打开。
- `.emProject` 是 CMake 生成物，不建议当作唯一真实工程文件维护。

---

## 5. 使用 SES 原生 J-Link 调试

用 SES 打开：

```text
build/segger_embedded_studio/xxx.emProject
```

在 SES 内部检查或修改调试连接方式：

```text
Project Options
  Debug
    Target Connection = J-Link
```

目标是使用：

```text
SES → Native J-Link Support → J-Link Probe → HPM MCU
```

而不是：

```text
SES → OpenOCD → CMSIS-DAP
```

也不是：

```text
VSCode → DAPLink
```

### 5.1 J-Link 注意事项

HPM 系列通常是 RISC-V MCU，调试接口一般走 JTAG。

如果调试连接失败，优先检查：

- J-Link 软件版本是否足够新。
- SEGGER 是否支持当前具体 HPM 型号。
- Device 名称是否选对。
- JTAG 线序是否正确。
- 板载调试器是否与外接 J-Link 冲突。
- TCK、TMS、TDI、TDO、TRST、nRESET 是否连接正确。
- Boot 模式是否允许调试和下载。
- 工程链接地址是否和启动方式匹配，例如 `flash_xip` / `ram`。

### 5.2 SES 工程被覆盖问题

`build/segger_embedded_studio/*.emProject` 是生成物。重新 CMake Configure 后，SES 里的手动配置可能会被覆盖。

可选处理方式：

#### 方案 A：接受 `.emProject` 是临时生成物

适合早期开发：

- CMake 是主工程。
- SES 工程每次从 CMake 生成。
- SES 只负责打开、构建、调试。
- 如果重新生成后 J-Link 配置丢失，手动设置一次。

#### 方案 B：将关键 SES 选项固化进 CMake

流程：

1. CMake 生成 `.emProject`。
2. SES 打开工程。
3. 手动改成原生 J-Link 调试。
4. 保存 SES 工程。
5. 对比 `.emProject` 修改前后的差异。
6. 将关键 key-value 迁移到 `CMakeLists.txt` 中的 `sdk_ses_options()`。

示意：

```cmake
sdk_ses_options("LIBRARY_IO_TYPE=SEGGER")
# sdk_ses_options("某个SES选项key=某个SES选项value")
```

注意：

- SES 选项 key 需要从实际 `.emProject` 中反查。
- 不要凭空猜 key 名。
- 不同 SES 版本和 HPM SDK 版本可能存在差异。

---

## 6. VSCode 配置

### 6.1 推荐扩展

建议安装：

```text
CMake Tools
clangd
Codex
```

可选：

```text
Microsoft C/C++
GitLens
```

如果使用 clangd，建议避免 Microsoft C/C++ IntelliSense 和 clangd 同时抢同一套 C/C++ 语义分析。

### 6.2 VSCode settings.json

项目内 `.vscode/settings.json` 示例：

```json
{
  "cmake.generator": "Ninja",
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.configureArgs": [
    "-DBOARD=hpm6750evkmini",
    "-DHPM_BUILD_TYPE=flash_xip",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
  ],

  "clangd.arguments": [
    "--background-index",
    "--compile-commands-dir=${workspaceFolder}/build"
  ],

  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

如果使用 VSCode 多根工作区，路径建议写在 `.code-workspace` 里。

---

## 7. SDK 本地化

强烈推荐为 Codex 生成当前项目所需的 SDK 子集：

```bat
cmake --build build --target localize_sdk
```

生成后项目内会出现：

```text
hpm_sdk_localized/
  boards/
  cmake/
  components/
  drivers/
  middleware/
  soc/
  samples/
```

实际目录会根据工程依赖不同而不同。

### 7.1 为什么推荐 localize_sdk

不推荐直接让 Codex 索引整个 HPM SDK，原因是：

- SDK 太大。
- sample 很多，容易干扰检索。
- middleware 内容复杂，可能让 LLM 误判当前工程依赖。
- 全量索引速度慢。
- Codex 更容易改到不该改的 SDK 文件。

推荐让 Codex 看到：

```text
my_app/
hpm_sdk_localized/
build/compile_commands.json
```

这样它能理解当前工程，又不会被整个 SDK 淹没。

---

## 8. VSCode 多根工作区

创建 `my_app.code-workspace`：

```json
{
  "folders": [
    {
      "name": "app",
      "path": "."
    },
    {
      "name": "localized_sdk",
      "path": "./hpm_sdk_localized"
    }
  ],
  "settings": {
    "C_Cpp.default.compileCommands": "${workspaceFolder:app}/build/compile_commands.json",
    "clangd.arguments": [
      "--background-index",
      "--compile-commands-dir=${workspaceFolder:app}/build"
    ],
    "cmake.buildDirectory": "${workspaceFolder:app}/build"
  }
}
```

使用方式：

```bat
code my_app.code-workspace
```

注意：

- VSCode 工作区里应该包含 App 和 localized SDK。
- 不建议默认把完整 `D:/hpm/sdk_env/hpm_sdk` 加进工作区。
- 除非确实要查完整 SDK，否则优先使用 `hpm_sdk_localized/`。

---

## 9. 给 Codex 的 AGENTS.md

项目根目录建议放置 `AGENTS.md`，用于约束 Codex 的行为。

示例：

```md
# HPM MCU Project Guide

This is an HPMicro RISC-V MCU firmware project.

## Primary IDE

- SEGGER Embedded Studio for RISC-V is the primary IDE.
- Native J-Link debugging is used in SES.
- Do not replace debugging with OpenOCD, CMSIS-DAP, or DAPLink unless explicitly requested.

## Build system

- HPM SDK CMake is the source of truth.
- SES project files are generated under `build/segger_embedded_studio/`.
- Do not manually edit generated `.emProject` files unless the task explicitly asks for SES option investigation.
- If SES options must be changed permanently, inspect the generated `.emProject` and migrate stable key-value options back into `CMakeLists.txt`.

## Build commands

Use the HPM SDK_ENV command prompt first, then run:

```bat
cmake -S . -B build -G Ninja ^
  -DBOARD=hpm6750evkmini ^
  -DHPM_BUILD_TYPE=flash_xip ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build
```

## SDK access

- Prefer `hpm_sdk_localized/` for SDK source references.
- Use `build/compile_commands.json` to understand macros and include directories.
- Do not invent include paths manually.
- Do not modify SDK source unless explicitly requested.

## Project layout

- Application code belongs in `src/` and `include/`.
- Board-specific configuration belongs in `boards/`.
- Build output belongs in `build/`.
- Scripts belong in `scripts/`.

## Coding rules

- Keep changes minimal and reviewable.
- Avoid broad, unrelated refactors.
- Preserve HPM SDK coding style where existing code already follows it.
- When changing initialization code, verify clock, pinmux, interrupt, DMA, cache, and linker assumptions.
- When changing CMake, keep HPM SDK macros such as `sdk_app_src()` and `sdk_app_inc()` rather than replacing them with generic CMake unless necessary.

## Validation

To validate changes, run:

```bat
scripts/build_debug.bat
```

If build fails, report the first meaningful compiler or linker error and propose the smallest fix.
```

---

## 10. 构建脚本

建议添加：

```text
scripts/build_debug.bat
```

内容：

```bat
@echo off
setlocal

cmake -S . -B build -G Ninja ^
  -DBOARD=hpm6750evkmini ^
  -DHPM_BUILD_TYPE=flash_xip ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if errorlevel 1 exit /b 1

cmake --build build

exit /b %errorlevel%
```

如果使用自定义板卡，将：

```bat
-DBOARD=hpm6750evkmini
```

改为你的 board 名称。

例如：

```bat
-DBOARD=my_board
```

---

## 11. 可选：SES 命令行构建

如果希望验证 SES 工程本身，可以使用 SEGGER 的 `emBuild.exe`。

示例：

```bat
"C:\Program Files\SEGGER\SEGGER Embedded Studio for RISC-V\bin\emBuild.exe" ^
  -config "Debug" ^
  build\segger_embedded_studio\xxx.emProject
```

注意：

- SES 安装路径按实际情况修改。
- `xxx.emProject` 按实际文件名修改。
- `Debug` 配置名以 SES 工程中的实际配置名为准。
- 这只是可选验证方式，不建议替代 CMake/Ninja 主构建链路。

---

## 12. 推荐日常工作流

### 12.1 第一次创建工程

```text
1. 从 HPM SDK_ENV 启动命令行。
2. 创建或复制用户 App 工程。
3. 编写 CMakeLists.txt。
4. 执行 CMake Configure。
5. 执行 CMake Build。
6. 生成 compile_commands.json。
7. 生成 SES 工程。
8. 用 SES 打开 .emProject。
9. 在 SES 中配置原生 J-Link。
10. 连接硬件并验证下载、断点、单步。
11. 生成 hpm_sdk_localized。
12. 用 VSCode 打开 my_app.code-workspace。
13. 添加 AGENTS.md。
14. 让 Codex 辅助阅读和修改代码。
```

### 12.2 日常开发

```text
1. 用 VSCode + Codex 改代码。
2. 用 scripts/build_debug.bat 验证编译。
3. 编译通过后切到 SES。
4. SES 下载和调试。
5. 如果需要查 SDK，优先查 hpm_sdk_localized。
6. 如果需要修改 board，修改 boards/ 下的自定义 board。
7. 如果需要新增源文件，修改 CMakeLists.txt。
8. 如果 CMake 重新生成导致 SES 配置变化，重新检查 J-Link 配置。
```

### 12.3 SDK 升级

```text
1. 不要直接覆盖用户工程。
2. 更新 HPM SDK_ENV 或 HPM SDK。
3. 清理 build/。
4. 重新 CMake Configure。
5. 重新 Build。
6. 重新生成 hpm_sdk_localized。
7. 检查 SES 工程是否仍能原生 J-Link 调试。
8. 重点检查 linker、startup、clock、pinmux、board 文件变化。
9. 用 Git 查看用户工程的实际变化。
```

---

## 13. 常见问题

### 13.1 Codex 为什么看不到 HPM SDK 的函数定义？

常见原因：

- VSCode 工作区没有包含 SDK 或 `hpm_sdk_localized/`。
- 没有生成 `compile_commands.json`。
- `clangd` 没有指向正确的 build 目录。
- 从普通命令行启动 VSCode，没有继承 HPM SDK_ENV 环境变量。
- CMake configure 失败，导致编译数据库不完整。

处理：

```bat
cmake -S . -B build -G Ninja ^
  -DBOARD=hpm6750evkmini ^
  -DHPM_BUILD_TYPE=flash_xip ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build --target localize_sdk
```

然后用 VSCode 打开：

```bat
code my_app.code-workspace
```

### 13.2 为什么不建议直接用 VSCode 调试？

可以用，但不建议作为主链路。

原因：

- SES 对 J-Link 原生支持更完整。
- HPM 是 RISC-V，很多 VSCode Cortex-M 工作流不完全适配。
- 用 VSCode 调试往往会引入 GDB Server、OpenOCD、DAP、launch.json 等额外变量。
- 当前目标是稳定开发，而不是把 VSCode 配成完整 IDE。

推荐：

```text
调试：SES
编辑：VSCode
代码助手：Codex
工程真相：CMake
```

### 13.3 SES 里修改的配置丢了怎么办？

因为 `.emProject` 是生成物。

解决策略：

- 短期：重新在 SES 里手动设置。
- 长期：对比 `.emProject`，把稳定选项迁移到 `CMakeLists.txt` 的 `sdk_ses_options()`。
- 不要把 generated `.emProject` 当成唯一主工程长期维护。

### 13.4 Codex 要修改 SDK 文件怎么办？

默认拒绝大面积修改 SDK 文件。

优先策略：

- 在用户工程中封装。
- 在 `boards/` 中做板级适配。
- 在 `src/` 中做应用层修正。
- 只有确认是 SDK bug 或必须定制时，才修改 `hpm_sdk_localized/` 或维护 patch。

### 13.5 是否应该把完整 HPM SDK 加入 Git？

通常不建议。

推荐：

- 用户 App 进 Git。
- 自定义 board 进 Git。
- `AGENTS.md` 进 Git。
- `.vscode/settings.json` 可按团队情况进 Git。
- `scripts/` 进 Git。
- `build/` 不进 Git。
- `hpm_sdk_localized/` 是否进 Git，取决于团队是否希望工程自包含。

如果团队非常重视可复现性，可以考虑将 `hpm_sdk_localized/` 进 Git；否则记录 HPM SDK 版本即可。

---

## 14. .gitignore 建议

```gitignore
# Build output
build/
out/

# SES generated/session files
*.emSession
*.emWorkspace
*.jlink
*.map
*.lst

# VSCode local state
.vscode/.browse.c_cpp.db
.vscode/ipch/

# Optional
# hpm_sdk_localized/
```

如果决定不提交 localized SDK，取消注释：

```gitignore
hpm_sdk_localized/
```

如果决定提交 localized SDK，就不要忽略它。

---

## 15. 最终推荐组合

```text
工程真相：
  CMakeLists.txt
  app.yaml
  boards/
  src/
  include/

构建：
  HPM SDK CMake + Ninja

SES：
  打开 build/segger_embedded_studio/*.emProject
  使用原生 J-Link 调试

VSCode：
  打开 my_app.code-workspace
  使用 clangd + compile_commands.json

Codex：
  读取 AGENTS.md
  修改应用代码、board 配置、CMake
  使用 scripts/build_debug.bat 验证

SDK：
  优先使用 hpm_sdk_localized 给 Codex 索引
```

一句话总结：

> 不要让 VSCode、SES、Codex 三者争夺“主工程”地位。CMake 负责定义工程，SES 负责硬件调试，VSCode + Codex 负责高效率编辑和代码理解。
