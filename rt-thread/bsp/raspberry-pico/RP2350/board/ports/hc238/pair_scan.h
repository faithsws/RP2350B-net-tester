/*
 * 对线 H 桥扫描（pair_scan）
 *
 * 扫描完成后通过回调上报 8 字节连通矩阵（与 net_tester_pair 一致）。
 */
#ifndef __PAIR_SCAN_H__
#define __PAIR_SCAN_H__

#include <rtthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 扫描完成回调（在扫描线程上下文调用，勿直接操作 LVGL）。
 * @param status  8 字节连通矩阵；中止时为 NULL
 * @param aborted RT_TRUE 表示被 stop/abort
 * @param user_data 注册时传入
 */
typedef void (*pair_scan_done_cb_t)(const uint8_t status[8],
                                    rt_bool_t aborted,
                                    void *user_data);

void pair_scan_set_done_cb(pair_scan_done_cb_t cb, void *user_data);

/* 启动异步扫描；已在运行则返回 -RT_EBUSY */
rt_err_t pair_scan_start(void);

/* 请求停止（异步，线程结束后回调 aborted=RT_TRUE） */
void pair_scan_stop(void);

rt_bool_t pair_scan_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAIR_SCAN_H__ */
