# 依赖版本锁定（DEPS）

恢复工程时请使用下列**固定版本**，避免 `*-latest` 包漂移导致编译/行为不匹配。

机器可读副本见同目录 [`deps.json`](deps.json)。

## 核心依赖

| 组件 | 版本 | Git Commit | 仓库 |
|------|------|------------|------|
| **RT-Thread** | 5.3.0 | `8a0fe09c659b74ed2543d87a36a12355053b2630` | https://gitee.com/rtthread/rt-thread.git<br>备用：https://github.com/RT-Thread/rt-thread.git |
| **Pico RP2350 SDK 包** | 2.1.1 | `66beae4ce5598b1dd5ba218d528a26fb2eb1e8e1` | https://gitee.com/RT-Thread-Mirror/raspberrypi-pico-rp2350-sdk.git<br>目录名：`raspberrypi-pico-rp2350-sdk-latest` |
| **LVGL** | 8.3 | `f2c103260f3ac5a1a8c50af348b994ef8153796d` | https://gitee.com/RT-Thread-Mirror/lvgl.git<br>备用：https://github.com/lvgl/lvgl.git<br>目录名：`lvgl-v8.3-latest` |

## 工具链（本机已验证）

| 工具 | 要求 |
|------|------|
| Arm GNU Toolchain (`arm-none-eabi-gcc`) | 建议 13+（验证：15.2） |
| CMake | ≥ 3.10 |
| Ninja | 1.12.1（`scripts/setup_env.bat` 可下载） |
| Python 3 + scons | `pip install scons` |

默认工具链路径（可被恢复脚本改写）：

`C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi`

## 一键恢复

在仓库根目录执行：

```bat
scripts\restore_workspace.bat
```

或指定输出目录并编译：

```bat
scripts\restore_workspace.bat D:\work\RP2350B-RT-Thread
```

PowerShell：

```powershell
.\scripts\restore_workspace.ps1 -OutDir ..\RP2350B-RT-Thread -Build
```

脚本将：

1. 按 `deps.json` 检出 RT-Thread / Pico SDK 包 / LVGL  
2. 建立 SDK junction，覆盖本仓库 BSP 增量  
3. 复制 `assets/HZK12`、`assets/HZK16` 到工程根  
4. （默认）运行 `setup_env` + `build.bat` 完成编译  

编译成功后产物：

`\<OutDir>\rt-thread\bsp\raspberry-pico\RP2350\build-ninja\rtthread.elf`
