# RP2350B 网络巡线对线仪固件（BSP 增量）

基于 **RT-Thread + RP2350B** 的网络巡线 / 测序 / 对线 / 网络调试固件。  
本仓库保存相对上游 RT-Thread 的**产品侧依赖代码**（BSP、板级驱动、CH390、LVGL 业务），需叠加到完整 `rt-thread` + Pico SDK 工程中编译。

## 依赖版本（已锁定）

详见 [`VERSIONS.md`](VERSIONS.md) / [`deps.json`](deps.json)：

| 组件 | 版本 | Commit |
|------|------|--------|
| RT-Thread | 5.3.0 | `8a0fe09c` |
| Pico RP2350 SDK 包 | 2.1.1 | `66beae4` |
| LVGL | 8.3 | `f2c1032` |

**不固定版本直接拉 `*-latest` 会导致编译或行为不匹配。** 请使用下方恢复脚本。

## 一键恢复可编译工程

```bat
scripts\restore_workspace.bat
```

默认输出到同级目录 `..\RP2350B-RT-Thread`，并自动编译。指定目录：

```bat
scripts\restore_workspace.bat D:\work\RP2350B-RT-Thread
```

仅恢复、不编译：

```powershell
.\scripts\restore_workspace.ps1 -OutDir ..\RP2350B-RT-Thread -SkipBuild
```

成功后产物：

`\<OutDir>\rt-thread\bsp\raspberry-pico\RP2350\build-ninja\rtthread.elf`

前置：已安装 `arm-none-eabi-gcc`、CMake、Git、Python3。Ninja/scons 可由 `setup_env.bat` 自动准备。

## 功能概览

| 模块 | 说明 |
|------|------|
| 寻线 | 开始后输出 `LINK_PWM` 460kHz（GPIO19），停止/退出关闭 |
| 对线 | 共用 H 桥扫描；`pair_judge` 判定断路/联通/短路；单接头 UI |
| 测序 | 共用扫描电压立方体；`seq_judge` 按标识电阻映射本端→对端线序 |
| 网络调试 | IP/DHCP、Ping、TCP/UDP、DNS、HTTP、ARP、链路闪灯 |
| 电源/电池 | 升压使能、电池 ADC、充电检测、顶栏电池图标 |
| 以太网 | CH390 SPI 网卡 + lwIP |

## 仓库结构

```text
assets/                     # HZK12 / HZK16 字库
deps.json / VERSIONS.md     # 依赖版本锁定
scripts/
  restore_workspace.*       # 一键恢复+编译
  setup_env.bat / build_bs2.bat
  pair_scan_analyze.py / seq_scan_analyze.py
build.bat                   # 宿主工程编译入口（恢复后复制）
rt-thread/
  bsp/raspberry-pico/
    RP2350/                 # 产品 BSP（applications / board / ports / LVGL）
    libraries/Drivers/      # 改过的 drv_gpio / drv_spi / drv_uart
  components/drivers/spi/   # CH390 驱动 + Kconfig/SConscript
```

## 板级驱动依赖（`board/ports`）

| 路径 | 作用 |
|------|------|
| `boost_pwr` | 外部升压使能 GPIO32 |
| `pwm/sc_link_pwm` | SC_PWM(GPIO17) / LINK_PWM(GPIO19) 455/460kHz |
| `hc238` | CTRL_H / CTRL_L 74HC238 + `pair_scan` 扫描 |
| `hc238/pair_judge` | 对线判定（断路/联通/短路） |
| `hc238/seq_judge` | 测序判定（`R=7.4/Vmux-2`，标识电阻 ±0.2kΩ） |
| `mux4051` | TX/RX TMUX4051、LINK4051、对线 ADC(GPIO41) |
| `battery` / `charger` | 电池电压、适配器/充电状态 |
| `ec11` | 旋转编码器 |
| `lcd` | ST7789 显示 |
| `hzk` | 汉字字库 |
| `ch390_port` + `ch390` | 以太网 |
| `power_hold` | 电源保持 |

## UI / 业务（`applications/lvgl`）

| 路径 | 作用 |
|------|------|
| `net_tester/net_tester_ui.c` | 主菜单与四功能页、电池图标 |
| `net_tester_trace.*` | 寻线 → `link_pwm_start/stop(460kHz)` |
| `net_tester_pair.*` | 对线 → `pair_scan` + `pair_judge` |
| `net_tester_crimp.*` | 测序 → `pair_scan` + `seq_judge` |
| `net_tester_netdbg.*` / `netops.*` | 网络协议调试页 |
| `lv_port_indev` / `lv_port_disp` | 按键/编码器、显示端口 |

## 扫描与判定

1. **扫描**（`hc238_port.c` / `pair_scan`）：升压 → 56 步 H/L → 每步 MUX0~7 采样 → 填充 `V[H][L][MUX]`（mV）
2. **对线**（`pair_judge`）：按电压典型点判定通道断路/联通/短路
3. **测序**（`seq_judge`）：仅用 `MUX==L` 采样，`R=7.4/V-2`，均值匹配对端标识电阻通道

离线验证：

```text
python scripts/pair_scan_analyze.py --log test-logs/xxx.log
python scripts/seq_scan_analyze.py --log test-logs/测序1.log
```

## 烧录

- UF2：`picotool uf2 convert rtthread.elf rtthread.uf2 --family rp2350-arm-s --abs-block`
- DebugControl **COM16** BOOT/RUN；FinSH **COM14**
- 或 CMSIS-DAP：`pyocd load -t rp2350 rtthread.elf`

## 调试串口常用命令

```text
boost_pwr on|off
ctrl_h <0-7> / ctrl_l <0-7> / hc238_off / hc238_info
link_mux <0-7|off> / link_adc
pair_scan / pair_scan_stop
link_pwm 460|off / pwm_info
```
