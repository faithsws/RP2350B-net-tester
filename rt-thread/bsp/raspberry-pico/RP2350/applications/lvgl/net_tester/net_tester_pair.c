#include "net_tester_pair.h"

#include "pair_scan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lvgl.h>
#include <rtthread.h>

static net_tester_pair_result_cb_t g_cb;
static void *g_user_data;
static bool g_seeded;
static bool g_waiting;
static bool g_hw_cb_ready;
static bool g_restart_pending;

static void ensure_seed(void)
{
    if(g_seeded) {
        return;
    }
    srand((unsigned)lv_tick_get());
    g_seeded = true;
}

static void log_result(const pair_scan_result_t *r)
{
    static const char *names[] = { "断路", "联通", "短路" };
    int i, j;

    rt_kprintf("[PAIR] ========== 对线结果 ==========\n");
    for(i = 0; i < PAIR_CH_COUNT; i++) {
        uint8_t st = r->ch_state[i];
        if(st > PAIR_CH_SHORT) {
            st = PAIR_CH_OPEN;
        }
        rt_kprintf("[PAIR] 针脚%d: %s\n", i + 1, names[st]);
    }
    for(i = 0; i < PAIR_CH_COUNT; i++) {
        for(j = i + 1; j < PAIR_CH_COUNT; j++) {
            if(r->short_bits[i] & (1u << j)) {
                rt_kprintf("[PAIR] 短路对: %d-%d\n", i + 1, j + 1);
            }
        }
    }
    rt_kprintf("[PAIR] ==============================\n");
}

static void pair_report_async_cb(void * p)
{
    pair_scan_result_t * result = (pair_scan_result_t *)p;

    if(result) {
        net_tester_pair_report_result(result);
        rt_free(result);
    }
    else {
        if(g_waiting) {
            g_waiting = false;
            rt_kprintf("[PAIR] scan aborted, waiting cleared\n");
        }
    }
}

static void pair_scan_done_hw_cb(rt_bool_t aborted, void *user_data)
{
    pair_scan_result_t * copy;
    const pair_volt_cube_t * volt;

    RT_UNUSED(user_data);

    if(aborted) {
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

    volt = pair_scan_get_volt();
    copy = (pair_scan_result_t *)rt_malloc(sizeof(pair_scan_result_t));
    if(!copy || !volt) {
        rt_kprintf("[PAIR] malloc/volt failed\n");
        if(copy) {
            rt_free(copy);
        }
        lv_async_call(pair_report_async_cb, RT_NULL);
        return;
    }

    pair_judge(volt, copy);
    pair_judge_log(copy);
    lv_async_call(pair_report_async_cb, copy);
}

static void ensure_hw_cb(void)
{
    /* 每次 start 都抢占回调，避免与测序互相覆盖后失效 */
    pair_scan_set_done_cb(pair_scan_done_hw_cb, RT_NULL);
    g_hw_cb_ready = true;
}

void net_tester_pair_set_result_cb(net_tester_pair_result_cb_t cb, void *user_data)
{
    g_cb = cb;
    g_user_data = user_data;
}

bool net_tester_pair_start(void)
{
    rt_err_t err;

    ensure_hw_cb();

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

    if(!g_waiting) {
        return;
    }
    g_waiting = false;
    rt_kprintf("[PAIR] stop / cancel waiting\n");
}

bool net_tester_pair_is_waiting(void)
{
    return g_waiting;
}

void net_tester_pair_report_result(const pair_scan_result_t *result)
{
    if(!result) {
        return;
    }

    if(!g_waiting) {
        rt_kprintf("[PAIR] ignore report: not waiting\n");
        return;
    }

    g_waiting = false;
    log_result(result);

    if(g_cb) {
        g_cb(result, g_user_data);
    }
}

void net_tester_pair_simulate_async_done(void)
{
    pair_scan_result_t result;
    int i;

    if(!g_waiting) {
        rt_kprintf("[PAIR] 空格忽略: 尚未开始或已收到结果\n");
        return;
    }

    if(pair_scan_is_running()) {
        rt_kprintf("[PAIR] 空格忽略: pair_scan 进行中\n");
        return;
    }

    ensure_seed();
    memset(&result, 0, sizeof(result));

    for(i = 0; i < PAIR_CH_COUNT; i++) {
        int r = rand() % 100;
        if(r < 15) {
            result.ch_state[i] = PAIR_CH_OPEN;
        }
        else if(r < 75) {
            result.ch_state[i] = PAIR_CH_CONN;
        }
        else {
            result.ch_state[i] = PAIR_CH_SHORT;
        }
    }
    {
        int a = -1, b = -1;
        for(i = 0; i < PAIR_CH_COUNT; i++) {
            if(result.ch_state[i] == PAIR_CH_SHORT) {
                if(a < 0) {
                    a = i;
                }
                else {
                    b = i;
                    break;
                }
            }
        }
        if(a >= 0 && b >= 0) {
            result.short_bits[a] |= (uint8_t)(1u << b);
            result.short_bits[b] |= (uint8_t)(1u << a);
        }
        else if(a >= 0) {
            b = (a + 1) % PAIR_CH_COUNT;
            result.ch_state[b] = PAIR_CH_SHORT;
            result.short_bits[a] |= (uint8_t)(1u << b);
            result.short_bits[b] |= (uint8_t)(1u << a);
        }
    }

    rt_kprintf("[PAIR] 空格模拟：上报结果\n");
    net_tester_pair_report_result(&result);
}
