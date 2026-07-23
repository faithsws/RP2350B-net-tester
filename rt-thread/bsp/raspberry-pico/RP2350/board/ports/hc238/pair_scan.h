/*
 * 扫描电压立方体公共定义 + H 桥扫描 API
 *
 * 扫描只负责填充 V[H][L][MUX]（mV）；判定由 pair_judge / seq_judge 完成。
 */
#ifndef __PAIR_SCAN_H__
#define __PAIR_SCAN_H__

#include <rtthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAIR_CH_COUNT           8
#define PAIR_SCAN_INVALID_MV    0xFFFFu

typedef uint16_t pair_volt_cube_t[PAIR_CH_COUNT][PAIR_CH_COUNT][PAIR_CH_COUNT];

/**
 * 扫描完成回调（扫描线程上下文，勿直接操作 LVGL）。
 * @param aborted   RT_TRUE 表示被 stop/abort
 * @param user_data 注册时传入
 */
typedef void (*pair_scan_done_cb_t)(rt_bool_t aborted, void *user_data);

void pair_scan_set_done_cb(pair_scan_done_cb_t cb, void *user_data);

rt_err_t pair_scan_start(void);
void pair_scan_stop(void);
rt_bool_t pair_scan_is_running(void);

/**
 * 最近一次完整扫描的电压立方体（扫描中/中止后内容不保证有效）。
 * V[h][l][m] 单位 mV；h==l 为 PAIR_SCAN_INVALID_MV。
 */
const pair_volt_cube_t *pair_scan_get_volt(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAIR_SCAN_H__ */
