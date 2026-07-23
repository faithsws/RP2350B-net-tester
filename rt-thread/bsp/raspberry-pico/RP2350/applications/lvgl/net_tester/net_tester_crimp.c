#include "net_tester_crimp.h"

#include "pair_scan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lvgl.h>
#include <rtthread.h>

static net_tester_crimp_result_cb_t g_cb;
static void *g_user_data;
static bool g_seeded;
static bool g_waiting;
static bool g_restart_pending;

static void ensure_seed(void)
{
    if(g_seeded) {
        return;
    }
    srand((unsigned)lv_tick_get());
    g_seeded = true;
}

static void log_result(const seq_scan_result_t *r)
{
    int i;

    rt_kprintf("[SEQ] ========== 测序结果 ==========\n");
    for(i = 0; i < PAIR_CH_COUNT; i++) {
        int8_t mid = r->map_to[i];
        if(mid < 0) {
            rt_kprintf("[SEQ] 本端%d -> ?\n", i + 1);
        }
        else {
            rt_kprintf("[SEQ] 本端%d -> 对端%d %s\n",
                       i + 1, mid + 1,
                       (mid == i) ? "(直通)" : "(错序)");
        }
    }
    rt_kprintf("[SEQ] ==============================\n");
}

static void seq_report_async_cb(void * p)
{
    seq_scan_result_t * result = (seq_scan_result_t *)p;

    if(result) {
        net_tester_crimp_report_result(result);
        rt_free(result);
    }
    else {
        if(g_waiting) {
            g_waiting = false;
            rt_kprintf("[SEQ] scan aborted, waiting cleared\n");
        }
    }
}

static void seq_scan_done_hw_cb(rt_bool_t aborted, void *user_data)
{
    seq_scan_result_t * copy;
    const pair_volt_cube_t * volt;

    RT_UNUSED(user_data);

    if(aborted) {
        if(g_restart_pending) {
            g_restart_pending = false;
            if(pair_scan_start() == RT_EOK) {
                rt_kprintf("[SEQ] restarted pair_scan after abort\n");
                return;
            }
            g_waiting = false;
            rt_kprintf("[SEQ] restart failed\n");
        }
        lv_async_call(seq_report_async_cb, RT_NULL);
        return;
    }

    volt = pair_scan_get_volt();
    copy = (seq_scan_result_t *)rt_malloc(sizeof(seq_scan_result_t));
    if(!copy || !volt) {
        rt_kprintf("[SEQ] malloc/volt failed\n");
        if(copy) {
            rt_free(copy);
        }
        lv_async_call(seq_report_async_cb, RT_NULL);
        return;
    }

    seq_judge(volt, copy);
    seq_judge_log(copy);
    lv_async_call(seq_report_async_cb, copy);
}

static void ensure_hw_cb(void)
{
    pair_scan_set_done_cb(seq_scan_done_hw_cb, RT_NULL);
}

void net_tester_crimp_set_result_cb(net_tester_crimp_result_cb_t cb, void *user_data)
{
    g_cb = cb;
    g_user_data = user_data;
}

bool net_tester_crimp_start(void)
{
    rt_err_t err;

    ensure_hw_cb();

    if(pair_scan_is_running()) {
        g_restart_pending = true;
        g_waiting = true;
        pair_scan_stop();
        rt_kprintf("[SEQ] scan busy, stop then restart\n");
        return true;
    }

    g_restart_pending = false;
    g_waiting = true;
    err = pair_scan_start();
    if(err != RT_EOK) {
        g_waiting = false;
        rt_kprintf("[SEQ] pair_scan_start failed: %d\n", (int)err);
        return false;
    }

    rt_kprintf("[SEQ] start: waiting sequence result\n");
    return true;
}

void net_tester_crimp_stop(void)
{
    g_restart_pending = false;

    if(pair_scan_is_running()) {
        pair_scan_stop();
    }

    if(!g_waiting) {
        return;
    }
    g_waiting = false;
    rt_kprintf("[SEQ] stop / cancel waiting\n");
}

bool net_tester_crimp_is_waiting(void)
{
    return g_waiting;
}

void net_tester_crimp_report_result(const seq_scan_result_t *result)
{
    if(!result) {
        return;
    }

    if(!g_waiting) {
        rt_kprintf("[SEQ] ignore report: not waiting\n");
        return;
    }

    g_waiting = false;
    log_result(result);

    if(g_cb) {
        g_cb(result, g_user_data);
    }
}

void net_tester_crimp_simulate_async_done(void)
{
    seq_scan_result_t result;
    int i;

    if(!g_waiting) {
        rt_kprintf("[SEQ] 空格忽略: 尚未开始或已收到结果\n");
        return;
    }

    if(pair_scan_is_running()) {
        rt_kprintf("[SEQ] 空格忽略: pair_scan 进行中\n");
        return;
    }

    ensure_seed();
    memset(&result, 0, sizeof(result));

    /* 模拟：多数直通，偶有交叉或未匹配 */
    for(i = 0; i < PAIR_CH_COUNT; i++) {
        int r = rand() % 100;
        if(r < 10) {
            result.map_to[i] = SEQ_MAP_NONE;
            result.mean_r_mk[i] = 0;
        }
        else if(r < 80) {
            result.map_to[i] = (int8_t)i;
            result.mean_r_mk[i] = 500 + i * 500;
        }
        else {
            result.map_to[i] = (int8_t)((i + 1 + (rand() % 6)) % PAIR_CH_COUNT);
            result.mean_r_mk[i] = 500 + result.map_to[i] * 500;
        }
    }

    rt_kprintf("[SEQ] 空格模拟：上报结果\n");
    net_tester_crimp_report_result(&result);
}
