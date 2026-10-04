# HPM5301_LibXR_Template

HPM5301EVKLite 的 LibXR 模板工程 / LibXR template project for the HPM5301EVKLite

## 1. 板子与平台 / Board and Platform

模板面向 HPMicro HPM5301EVKLite 开发板（HPM5301，RISC-V，板载 1 MB QSPI NOR Flash），无操作系统，外设由 LibXR 的 `hpm` 驱动提供。工程是 HPM SDK 应用工程：`CMakeLists.txt` 把 `HPM_SDK_BASE` 指向仓库内本地化的 SDK（`hpm_sdk_localized_for_hpm5301evklite/`，HPM SDK 1.11.0，板子固定为 `hpm5301evklite`）。`cmake/LibXR.CMake` 以 `LIBXR_SYSTEM None`、`LIBXR_DRIVER hpm` 接入 LibXR，并把 SDK 的编译选项传给 `xr` 目标。LibXR 是 `libxr/` 下的 Git 子模块，地址为 `https://github.com/xrobot-org/libxr.git`，本仓库记录的子模块提交固定所用的 LibXR 版本。

```text
src/main.cpp                           应用代码 main()
CMakeLists.txt                         HPM SDK 应用工程
cmake/LibXR.CMake                      LibXR 接入
CMakePresets.json                      构建预设
hpm_sdk_localized_for_hpm5301evklite/  本地化的 HPM SDK
libxr/                                 LibXR 子模块
```

The template targets the HPMicro HPM5301EVKLite board (HPM5301, RISC-V, 1 MB on-board QSPI NOR flash) without an operating system; the peripherals are provided by the LibXR `hpm` driver. The project is an HPM SDK application: `CMakeLists.txt` points `HPM_SDK_BASE` at the localized SDK in the repository (`hpm_sdk_localized_for_hpm5301evklite/`, HPM SDK 1.11.0, with the board fixed to `hpm5301evklite`). `cmake/LibXR.CMake` brings in LibXR with `LIBXR_SYSTEM None` and `LIBXR_DRIVER hpm` and passes the SDK compile options to the `xr` target. LibXR is the Git submodule `libxr/` at `https://github.com/xrobot-org/libxr.git`, and the submodule commit recorded in this repository pins the LibXR version in use.

## 2. 配置一览 / Configurations

| 配置 | 用途 |
| --- | --- |
| `src/main.cpp` | 按键切换灯效：创建 `HPMTimebase`，由 PWM（`HPMPWM`，GPTMR0 通道 2，PB10，1 kHz）输出灯效波形。用户按键 PA3 配置为中断输入（`HPMGPIO`），中断中置位标志，主循环完成 30 ms 消抖。灯效有两种模式：呼吸（占空比 2% 至 90%，每 20 ms 变化 0.8%，两端各保持 500 ms）和 1 Hz 闪烁，每次按键在两种模式间切换。板子定义的 LED 引脚 PA10 配置为模拟高阻，以免加载外部接到 PB10 的 PWM 信号 |

| Configuration | Purpose |
| --- | --- |
| `src/main.cpp` | Light effect selected by a key: creates `HPMTimebase`, and a PWM (`HPMPWM`, GPTMR0 channel 2, PB10, 1 kHz) outputs the effect waveform. The user key on PA3 is an interrupt input (`HPMGPIO`) whose handler sets a flag, and the main loop debounces it over 30 ms. The effect has two modes, breathing (duty cycle 2% to 90%, changing by 0.8% every 20 ms, held 500 ms at each end) and a 1 Hz blink, and each key press switches between them. The LED pin defined by the board, PA10, is set to analog high impedance so that the external PWM signal connected to PB10 is not loaded |

## 3. 构建 / Build

构建使用 CMake（3.23 或更高）、Ninja 和 HPM RISC-V GCC 工具链，环境变量 `GNURISCV_TOOLCHAIN_PATH` 指向工具链目录。镜像 `ghcr.io/xrobot-org/docker-image-hpm:main` 提供 CMake、Ninja 和工具链，工具链目录记录在环境变量 `XR_HPM_TOOLCHAIN_ROOT` 中。

```bash
git clone --recursive https://github.com/xrobot-org/HPM5301_LibXR_Template.git
cd HPM5301_LibXR_Template
docker run --rm -v "$PWD:/work" -w /work ghcr.io/xrobot-org/docker-image-hpm:main \
  bash -c 'export GNURISCV_TOOLCHAIN_PATH="$XR_HPM_TOOLCHAIN_ROOT" && cmake --preset release-flash-xip && cmake --build --preset release-flash-xip'
```

`CMakePresets.json` 的预设如下，构建目录均为 `build/`，产物为 `build/output/demo.elf` 和 `build/output/demo.bin`。已克隆但未带子模块时，运行 `git submodule update --init` 获取 LibXR。

| 预设 | `CMAKE_BUILD_TYPE` | `HPM_BUILD_TYPE` |
| --- | --- | --- |
| `release-flash-xip` | Release | `flash_xip` |
| `debug-flash-xip` | Debug | `flash_xip` |
| `debug-ram` | Debug | `ram` |

`.github/workflows/build.yml` 在上述镜像中递归检出子模块，构建三个预设。其中的 `libxr-master` 作业每天运行一次，把 `libxr/` 更新到 LibXR 的 `master` 后构建，用于提前发现兼容性变化，作业失败不影响工作流结果。

Building uses CMake (3.23 or newer), Ninja and the HPM RISC-V GCC toolchain, with the environment variable `GNURISCV_TOOLCHAIN_PATH` pointing at the toolchain directory. The image `ghcr.io/xrobot-org/docker-image-hpm:main` provides CMake, Ninja and the toolchain, and records the toolchain directory in the environment variable `XR_HPM_TOOLCHAIN_ROOT`.

The presets in `CMakePresets.json` are listed above; all of them build in `build/` and produce `build/output/demo.elf` and `build/output/demo.bin`. For a clone made without submodules, `git submodule update --init` fetches LibXR.

`.github/workflows/build.yml` checks out the submodules recursively and builds the three presets in the image above. Its `libxr-master` job runs daily, updates `libxr/` to the LibXR `master` branch and builds, which reveals compatibility changes early; a failure of that job does not fail the workflow.

## 4. 烧录与运行 / Flash and Run

`flash_xip` 预设的固件从板载 QSPI NOR Flash 执行，`ram` 预设的固件加载到 RAM 运行。开发板的启动开关置于 OFF 时从 Flash 启动。SDK 在 `hpm_sdk_localized_for_hpm5301evklite/boards/openocd/` 中提供 OpenOCD 配置，`boards/hpm5301evklite/hpm5301evklite.yaml` 指定板子的默认探针为 CMSIS-DAP。使用 HPMicro 版 OpenOCD 时，调试服务按 SDK 配置文件头部的用法启动：

```bash
export HPM_SDK_BASE="$PWD/hpm_sdk_localized_for_hpm5301evklite"
cd "$HPM_SDK_BASE/boards/openocd"
openocd -c "set HPM_SDK_BASE ${HPM_SDK_BASE}; set BOARD hpm5301evklite; set PROBE cmsis_dap;" -f hpm5300_all_in_one.cfg
```

随后用 `riscv32-unknown-elf-gdb` 连接并下载 `build/output/demo.elf`。开发板的接口、跳线和引脚见 SDK 中的 `boards/hpm5301evklite/README_en.rst`。运行后 PB10 输出呼吸灯效的 PWM，按下用户按键切换为 1 Hz 闪烁，再次按下恢复呼吸。

With the `flash_xip` presets the firmware executes from the on-board QSPI NOR flash, and with the `ram` preset it is loaded into RAM. The board boots from flash when its boot switch is OFF. The SDK provides OpenOCD configurations in `hpm_sdk_localized_for_hpm5301evklite/boards/openocd/`, and `boards/hpm5301evklite/hpm5301evklite.yaml` names CMSIS-DAP as the default probe of the board. With the HPMicro build of OpenOCD, the debug server starts as in the commands above, which follow the usage described at the top of the SDK configuration file.

Then `riscv32-unknown-elf-gdb` connects and downloads `build/output/demo.elf`. The board interfaces, jumpers and pins are described in `boards/hpm5301evklite/README_en.rst` in the SDK. At run time PB10 outputs the breathing PWM, a press of the user key switches to a 1 Hz blink, and another press returns to breathing.

本仓库以 Apache-2.0 发布，见 [LICENSE](LICENSE)；`CMakeLists.txt` 和 `src/main.cpp` 的文件头保留 HPMicro 的 BSD-3-Clause 声明，`hpm_sdk_localized_for_hpm5301evklite/` 中的 HPM SDK 保留其自身的许可，见该目录下的 `LICENSE`。

This repository is released under Apache-2.0, see [LICENSE](LICENSE); the file headers of `CMakeLists.txt` and `src/main.cpp` keep the HPMicro BSD-3-Clause notice, and the HPM SDK in `hpm_sdk_localized_for_hpm5301evklite/` keeps its own license, see `LICENSE` in that directory.
