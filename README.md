# HPM5301 LibXR Template

这是一个面向 HPM5301EVKLite 的 HPM SDK + LibXR 模板工程。仓库内包含本地化的
HPM SDK 板级文件、CMake 构建入口和 LibXR 接入配置，可作为
[XRobot HPM 环境配置](https://xrobot-org.github.io/docs/env_setup/env-setup-hpm)的起始工程。

## 工程结构

- `hpm_sdk_localized_for_hpm5301evklite/`：本地化的 HPM SDK 与板级文件。
- `hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/pinmux.hpmpc`：
  HPM Pinmux Tool 的引脚配置源文件。
- `cmake/LibXR.CMake`：LibXR 路径、平台和驱动选择。
- `src/main.cpp`：应用入口。
- `CMakePresets.json`：Flash XIP、RAM 等构建预设。

## 环境要求

需要安装 HPM SDK Env 使用的 RISC-V GCC、CMake 和 Ninja，并设置工具链路径。例如：

```powershell
$env:GNURISCV_TOOLCHAIN_PATH = "C:/HPM/sdk_env/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win"
```

根目录 `CMakeLists.txt` 已将 `HPM_SDK_BASE` 指向仓库内的本地化 SDK。LibXR
源码路径请通过 CMake 参数显式指定，不要依赖 `cmake/LibXR.CMake` 中的本机默认值：

```powershell
cmake --preset debug-flash-xip -DLIBXR_DIR="D:/path/to/libxr"
cmake --build --preset debug-flash-xip
```

其他可用预设见 `CMakePresets.json`。

## HPM 配置分为两层

### 1. 引脚复用

引脚分配仍由先楫官方
[HPM Pinmux Tool](https://marketplace.visualstudio.com/items?itemName=HPMicro.hpm-pinmux-tool)
负责。在 VS Code 中打开：

```text
hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/pinmux.hpmpc
```

用图形界面配置 UART、I2C、SPI、CAN 等外设使用的 Pad 和 Pinmux 函数，然后保存
`.hpmpc`。仓库中的 `.hpmpc` 是引脚配置的唯一源文件，应该纳入版本控制。

### 2. 外设行为与 LibXR 代码生成

新的
[XRobot HPM Peripheral Config](https://github.com/xrobot-org/hpm-peripheral-config)
VS Code 插件提供类似 STM32CubeMX 外设配置页的图形界面，可用于：

- 选择当前工程实际调用的 Pinmux 函数；
- 配置 UART 波特率与缓冲区；
- 配置 I2C 总线速率；
- 配置 SPI 时钟、模式、硬件/GPIO 片选；
- 配置 CAN/CAN FD 位速率、采样点和队列；
- 校验时钟、时序、DMA 和 Pinmux 组合；
- 生成或更新 LibXR 配置、应用入口和板级代码。

> 预览状态：插件的 CLI 后端重构及最新 SPI/GPIO-CS 功能仍在
> [Draft PR #1](https://github.com/xrobot-org/hpm-peripheral-config/pull/1)，尚未合并到
> 插件仓库的 `main`。想提前体验可以从 GitHub 拉取该 PR 使用的
> [`codex/hpm-cli-backend-refactor`](https://github.com/xrobot-org/hpm-peripheral-config/tree/codex/hpm-cli-backend-refactor)
> 分支并自行打包安装。插件 `main` 和当前发布版 `0.2.19` 尚不包含下文所述的完整
> `0.3.0` 工作流。

## 安装预览版 GUI

插件要求 VS Code 1.108.0 或更高版本。

### 1. 安装 HPM 代码生成后端

GUI 需要
[LibXR_CppCodeGenerator >= 5.3.0](https://github.com/CaFeZn/LibXR_CppCodeGenerator/tree/feat/hpm-config-generator)
提供的 `xr_hpm_cfg`：

```powershell
python -m pip install --upgrade "git+https://github.com/CaFeZn/LibXR_CppCodeGenerator.git@feat/hpm-config-generator"
xr_hpm_cfg --help
```

如果使用 `pipx`，也可以将同一个 Git 地址安装到独立环境，但必须保证
`xr_hpm_cfg` 位于 `PATH` 中。也可以在 VS Code 设置中通过
`hpmPeripheral.cliPath` 指定可执行文件的绝对路径。

### 2. 获取并安装插件

```powershell
git clone https://github.com/xrobot-org/hpm-peripheral-config.git
cd hpm-peripheral-config
git switch codex/hpm-cli-backend-refactor
npm ci
npm test
npm run package -- --out hpm-peripheral-config-0.3.0.vsix
code --install-extension ./hpm-peripheral-config-0.3.0.vsix --force
```

安装后在 VS Code 中执行 `Developer: Reload Window`。

如果需要从插件打开官方 HPM Project Generator，还要安装 HPM SDK Env，并通过
`hpmPeripheral.sdkEnvPath` 指向 SDK Env 根目录。

### 3. 使用匹配的 LibXR HPM 分支

生成的 UART 和 GPIO-CS SPI 代码目前还依赖对应的 LibXR HPM 外设实现：

- [feat/hpm-uart-peripheral-config](https://github.com/CaFeZn/libxr/tree/feat/hpm-uart-peripheral-config)

在这些改动合并 LibXR 主线前，请将 `LIBXR_DIR` 指向该分支的本地检出目录。

## GUI 配置流程

`.hpmpc` 只描述 Pad、信号和 Pinmux 函数；`hpm_peripherals.yaml` 保存当前选择的
Pinmux 函数以及外设速率、时钟、DMA、缓冲区和队列等行为。不要用手工修改
`pinmux.c` 来代替 `.hpmpc` 中的引脚配置。

1. 在 VS Code 中打开本模板工程根目录。
2. 本模板将 `.hpmpc` 放在本地化 SDK 目录中。为了避免递归扫描整套 SDK，生成器默认
   跳过 `hpm_sdk_localized_for_*`，因此需要在 VS Code 工作区设置中明确指定：

   ```json
   {
     "hpmPeripheral.hpmpcPath": "hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/pinmux.hpmpc"
   }
   ```

3. 打开活动栏中的 **XRobot HPM Peripherals**。
4. 点击 **Pinmux**，使用官方 HPM Pinmux Tool 配置并保存 `.hpmpc`。
5. 插件检测到 `.hpmpc` 变化后会自动重新解析，无需手动刷新。
6. 在 **Pinmux functions** 中选择应用需要的初始化函数。可以同时保留
   `init_all_pins` 和外设专用函数；GPIO 片选函数必须在基础函数之后执行，GUI 会在
   切换 GPIO CS 时自动处理顺序。
7. 配置 UART、I2C、SPI、CAN/CAN FD 等外设行为并处理页面中的校验错误。
8. 点击 **Save YAML** 只保存行为配置；点击 **Save + Generate** 则校验并一次性更新
   所有受管理文件。

正常工作流会涉及以下文件：

```text
hpm_peripherals.yaml
.config.yaml
User/libxr_config.yaml
User/app_main.cpp
User/app_main.h
hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/board.c
hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/board.h
hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/pinmux.c
hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/pinmux.h
```

第一次生成前建议先提交当前工程，生成后通过 `git diff` 审查板级文件和应用代码。
生成器会以事务方式更新这些文件，并保留 `User/libxr_config.yaml` 中不受管理的节点和
C/C++ 文件中受管标记以外的用户代码。

不要提交插件为 HPM Pinmux Tool 准备的 `.xrobot-local` 签名工作副本或任何本地凭据。
提交仓库 `.hpmpc` 前也要确认其中不含账号、令牌或签名客户端凭据；如果真实凭据曾经
进入 Git 历史，应立即轮换并清理历史。

## 当前模板的接入边界

当前模板 `master` 的 `CMakeLists.txt` 只编译 `src/main.cpp`，应用入口也尚未调用
`app_main()`。因此预览版 GUI 目前可以在本模板上完成 `.hpmpc` 解析、行为配置、校验
和文件生成，但生成的 `User/app_main.cpp` 不会自动参与现有固件构建。

在模板主线补齐 `User/app_main.cpp/.h` 的 CMake 接入和 `app_main()` 调用前，请将
**Save + Generate** 视为配置与生成预览，并通过 `git diff` 审查结果，不要误认为生成
代码已经在固件中运行。

## 不使用 GUI

GUI 只是 `xr_hpm_cfg` 的 VS Code 前端。自动化、CI 或无界面环境仍可直接使用命令行：

```powershell
$hpmpc = "hpm_sdk_localized_for_hpm5301evklite/boards/hpm5301evklite/pinmux.hpmpc"
xr_hpm_cfg inspect -d . -i $hpmpc
xr_hpm_cfg validate -d . -i $hpmpc --peripheral-config hpm_peripherals.yaml
xr_hpm_cfg generate -d . -i $hpmpc --peripheral-config hpm_peripherals.yaml
```

命令行与 GUI 使用同一套工程解析、规范化、校验和代码生成逻辑。
