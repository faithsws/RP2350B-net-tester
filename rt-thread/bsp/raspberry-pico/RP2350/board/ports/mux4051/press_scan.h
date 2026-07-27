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
/* 每个 RX/TX 对应 7 个异通道采样点，再取均值 */
#define PRESS_SCAN_RX_AVG_N   (PRESS_SCAN_CH_NUM - 1)
#define PRESS_SCAN_TX_AVG_N   (PRESS_SCAN_CH_NUM - 1)
/* TX/RX 均值均低于该阈值 → 该通道压接不牢（默认值；运行时见 press_judge_thr_get） */
#define PRESS_JUDGE_THR_MV_DEFAULT  30
#define PRESS_JUDGE_THR_MV          PRESS_JUDGE_THR_MV_DEFAULT

typedef struct
{
    rt_uint32_t tx_avg_mv[PRESS_SCAN_CH_NUM];
    rt_uint32_t rx_avg_mv[PRESS_SCAN_CH_NUM];
    /* bit(i)=1：通道 i 压接牢固；bit(i)=0：不牢固 */
    rt_uint8_t status;
} press_judge_result_t;

/** 压接判定阈值（mV），可由 Flash 参数覆盖 */
rt_uint32_t press_judge_thr_get(void);
void press_judge_thr_set(rt_uint32_t mv);

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

/**
 * 按 TX 通道汇总：每个 TX 对其余 7 个 RX 采样取均值。
 * avg_mv[tx]：该 TX 的均值 mV。
 */
void press_scan_tx_avg(const rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM],
                       rt_uint32_t avg_mv[PRESS_SCAN_CH_NUM]);

/**
 * 压接牢固判定：TX_avg[i]<30mV 且 RX_avg[i]<30mV → 通道 i 不牢（bit=0）。
 */
void press_scan_judge(const rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM],
                      press_judge_result_t *out);

/** 扫描 + 均值 + 判定；成功返回 RT_EOK */
rt_err_t press_scan_run_judge(press_judge_result_t *out);

void press_scan_judge_log(const press_judge_result_t *r);

#ifdef __cplusplus
}
#endif

#endif /* __PRESS_SCAN_H__ */
