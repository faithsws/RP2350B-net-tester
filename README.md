# RP2350B 网络巡线对线仪固件（BSP 增量）

基于 **RT-Thread + RP2350B** 的网络巡线 / 压接 / 对线 / 网络调试固件。  
本仓库保存相对上游 RT-Thread 的**产品侧依赖代码**（BSP、板级驱动、CH390、LVGL 业务），需叠加到完整 `rt-thread` + Pico SDK 工程中编译。

## 功能概览

| 模块 | 说明 |
|------|------|
| 寻线 | 开始后输出 `LINK_PWM` 460kHz（GPIO19），停止/退出关闭 |
| 对线 | H 桥 `ctrl_h`/`ctrl_l` 扫描 + LINK4051 0~7 ADC 采样（连通判定待定） |
| 压接 | UI + 结果回调（硬件对接中） |
| 网络调试 | IP/DHCP、Ping、TCP/UDP、DNS、HTTP、ARP、链路闪灯 |
| 电源/电池 | 升压使能、电池 ADC、充电检测、顶栏电池图标 |
| 以太网 | CH390 SPI 网卡 + lwIP |

## 仓库结构

```text
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
| `hc238` | CTRL_H / CTRL_L 74HC238 + `pair_scan` 对线扫描 |
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
| `net_tester_pair.*` | 对线 → `pair_scan` |
| `net_tester_crimp.*` | 压接 |
| `net_tester_netdbg.*` / `netops.*` | 网络协议调试页 |
| `lv_port_indev` / `lv_port_disp` | 按键/编码器、显示端口 |

## 对线扫描当前行为（`hc238_port.c`）

1. 升压开启  
2. 56 步：每步设定一对 H/L（PMOS/NMOS，H≠L）  
3. H/L 稳定等待后，CD4051 依次切通道 **0~7**，每通道等待后 ADC 采样并打印  
4. **暂不判定连通性**（矩阵上报全 0），待实测规律后再补算法  

## 编译提示

宿主工程路径示例：`…/RP2350B-RT-Thread/rt-thread/bsp/raspberry-pico/RP2350`  

1. 将本仓库对应文件覆盖/合并进完整 RT-Thread 树  
2. `scons --target=cmake` 后用 Ninja 构建，或沿用工程内 `build-ninja`  
3. UF2：`picotool uf2 convert rtthread.elf rtthread.uf2 --family rp2350-arm-s --abs-block`  
4. 烧录：DebugControl **COM16** BOOT/RUN；FinSH **COM14**

## 调试串口常用命令

```text
boost_pwr on|off
ctrl_h <0-7> / ctrl_l <0-7> / hc238_off / hc238_info
link_mux <0-7|off> / link_adc
pair_scan / pair_scan_stop
link_pwm 460|off / pwm_info
```
