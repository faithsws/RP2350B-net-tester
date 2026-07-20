#include "net_tester_pair.h"
#include "pair_scan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lvgl.h>
#include <rtthread.h>

static net_tester_pair_result_cb_t g_cb;
static void * g_user_data;
static bool g_seeded;
static bool g_waiting;
static bool g_hw_cb_ready;
static bool g_restart_pending;

static void ensure_seed(void)
{
    if(g_seeded) return;
    srand((unsigned)lv_tick_get());
    g_seeded = true;
}

static void format_bits(uint8_t v, char out[9])
{
    for(int i = 7; i >= 0; i--) {
        out[7 - i] = (v & (1u << i)) ? '1' : '0';
    }
    out[8] = '\0';
}

static void log_status(const uint8_t status[8])
{
    rt_kprintf("[PAIR] ========== 连通矩阵 ==========\n");
    for(int i = 0; i < 8; i++) {
        char bits[9];
        format_bits(status[i], bits);
        rt_kprintf("[PAIR] byte[%d]=0x%02X (%sb)  线%d ->", i, status[i], bits, i + 1);

        bool any = false;
        for(int j = 0; j < 8; j++) {
            if(status[i] & (1u << j)) {
                rt_kprintf(" %d", j + 1);
                any = true;
            }
        }
        if(!any) rt_kprintf(" (无)");
        rt_kprintf("\n");
    }
    rt_kprintf("[PAIR] ==============================\n");
}

/* LVGL 线程中执行：上报结果并刷新 UI */
static void pair_report_async_cb(void * p)
{
    uint8_t * status = (uint8_t *)p;

    if(status) {
        net_tester_pair_report_status(status);
        rt_free(status);
    }
    else {
        /* 中止：结束等待，不更新矩阵 */
        if(g_waiting) {
            g_waiting = false;
            rt_kprintf("[PAIR] scan aborted, waiting cleared\n");
        }
    }
}

/* 扫描线程回调：投递到 LVGL 异步队列 */
static void pair_scan_done_hw_cb(const uint8_t status[8], rt_bool_t aborted, void * user_data)
{
    uint8_t * copy;

    RT_UNUSED(user_data);

    if(aborted || status == RT_NULL) {
        /* 用户再次点「开始」：上一轮结束后立刻重扫 */
        if(g_restart_pending) {
            g_restart_pending = false;
            if(pair_scan_start() == RT_EOK) {
                rt_kprintf("[PAIR] restarted pair_scan after abort\n");
                return;
            }
            g_waiting = false;
            rt_kprintf("[PAIR] restart failed\n");
        }
        lv_async_call(pair_report_async_cb, RT_NULL);
        return;
    }

    copy = (uint8_t *)rt_malloc(8);
    if(!copy) {
        rt_kprintf("[PAIR] malloc status failed\n");
        lv_async_call(pair_report_async_cb, RT_NULL);
        return;
    }

    memcpy(copy, status, 8);
    lv_async_call(pair_report_async_cb, copy);
}

static void ensure_hw_cb(void)
{
    if(g_hw_cb_ready) return;
    pair_scan_set_done_cb(pair_scan_done_hw_cb, RT_NULL);
    g_hw_cb_ready = true;
}

void net_tester_pair_set_result_cb(net_tester_pair_result_cb_t cb, void * user_data)
{
    g_cb = cb;
    g_user_data = user_data;
}

bool net_tester_pair_start(void)
{
    rt_err_t err;

    ensure_hw_cb();

    /* 扫描进行中再点开始：先停再重开 */
    if(pair_scan_is_running()) {
        g_restart_pending = true;
        g_waiting = true;
        pair_scan_stop();
        rt_kprintf("[PAIR] scan busy, stop then restart\n");
        return true;
    }

    g_restart_pending = false;
    g_waiting = true;
    err = pair_scan_start();
    if(err != RT_EOK) {
        g_waiting = false;
        rt_kprintf("[PAIR] pair_scan_start failed: %d\n", (int)err);
        return false;
    }

    rt_kprintf("[PAIR] start: waiting pair_scan result\n");
    return true;
}

void net_tester_pair_stop(void)
{
    g_restart_pending = false;

    if(pair_scan_is_running()) {
        pair_scan_stop();
    }

    if(!g_waiting) return;
    g_waiting = false;
    rt_kprintf("[PAIR] stop / cancel waiting\n");
}

bool net_tester_pair_is_waiting(void)
{
    return g_waiting;
}

void net_tester_pair_report_status(const uint8_t status[8])
{
    if(!status) return;

    if(!g_waiting) {
        rt_kprintf("[PAIR] ignore report: not waiting (click Start first)\n");
        return;
    }

    g_waiting = false;
    log_status(status);

    if(g_cb) {
        g_cb(status, g_user_data);
    }
}

void net_tester_pair_simulate_async_done(void)
{
    if(!g_waiting) {
        rt_kprintf("[PAIR] 空格忽略: 尚未开始或已收到结果\n");
        return;
    }

    /* 硬件扫描进行中时，空格不打断真实结果 */
    if(pair_scan_is_running()) {
        rt_kprintf("[PAIR] 空格忽略: pair_scan 进行中\n");
        return;
    }

    ensure_seed();

    uint8_t status[8];
    for(int i = 0; i < 8; i++) {
        uint8_t v = 0;
        for(int j = 0; j < 8; j++) {
            if((rand() % 100) < 35) {
                v |= (uint8_t)(1u << j);
            }
        }
        if(v == 0) {
            v = (uint8_t)(1u << (rand() % 8));
        }
        status[i] = v;
    }

    rt_kprintf("[PAIR] 空格模拟：异步任务结束，上报结果\n");
    net_tester_pair_report_status(status);
}
