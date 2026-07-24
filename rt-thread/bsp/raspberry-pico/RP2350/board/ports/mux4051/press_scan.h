/*
 * 压接扫描：TX 4051 发 460kHz 方波，RX 4051 切换采样 GPIO45
 *
 * 约束：TX/RX 不同时选同一通道，也不采样同通道电压。
 */
#ifndef __PRESS_SCAN_H__
#define __PRESS_SCAN_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRESS_SCAN_CH_NUM     8
#define PRESS_SCAN_SETTLE_MS  50
/* 每个 RX 对应 7 个异通道 TX 采样点，再取均值 */
#define PRESS_SCAN_RX_AVG_N   (PRESS_SCAN_CH_NUM - 1)

/**
 * 执行一次压接电压扫描。
 * mv[tx][rx]：ADC 引脚电压 mV；对角（tx==rx）填 0 且不采样。
 * 返回实际采样点数（通常 56 = 8*7）。
 */
int press_scan_run(rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM]);

/**
 * 按 RX 通道汇总：每个 RX 对其余 7 个 TX 采样取均值。
 * avg_mv[rx]：该 RX 的均值 mV。
 */
void press_scan_rx_avg(const rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM],
                       rt_uint32_t avg_mv[PRESS_SCAN_CH_NUM]);

#ifdef __cplusplus
}
#endif

#endif /* __PRESS_SCAN_H__ */
