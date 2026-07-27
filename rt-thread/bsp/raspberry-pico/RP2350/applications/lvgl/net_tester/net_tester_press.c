#include "net_tester_press.h"

#include "press_scan.h"

#include <stdio.h>
#include <stdlib.h>

#include <lvgl.h>
#include <rtthread.h>

#define PRESS_SCAN_STACK   3072
#define PRESS_SCAN_PRIO    (RT_THREAD_PRIORITY_MAX / 2 + 2)

static net_tester_press_result_cb_t g_cb;
static void *g_user_data;
static bool g_seeded;
static volatile bool g_waiting;
static volatile bool g_abort;
static rt_thread_t g_tid;

static void ensure_seed(void)
{
    if(g_seeded) {
        return;
    }
    srand((unsigned)lv_tick_get());
    g_seeded = true;
}

static void log_status(uint8_t status)
{
    rt_kprintf("[PRESS] status=0x%02X\n", status);
    rt_kprintf("[PRESS] 牢固线: ");
    bool any = false;
    for(int i = 0; i < 8; i++) {
        if(status & (1u << i)) {
            rt_kprintf("%d ", i + 1);
            any = true;
        }
    }
    if(!any) {
        rt_kprintf("(无)");
    }
    rt_kprintf("\n");
}

static void press_report_async_cb(void *p)
{
    uint8_t status = (uint8_t)(uintptr_t)p;
    net_tester_press_report_status(status);
}

static void press_scan_thread_entry(void *parameter)
{
    press_judge_result_t judge;
    rt_err_t err;

    RT_UNUSED(parameter);

    rt_kprintf("[PRESS] hardware scan start\n");
    err = press_scan_run_judge(&judge);
    g_tid = RT_NULL;

    if(g_abort || !g_waiting) {
        rt_kprintf("[PRESS] scan discarded (abort/cancel)\n");
        return;
    }

    if(err != RT_EOK) {
        rt_kprintf("[PRESS] scan failed\n");
        lv_async_call(press_report_async_cb, (void *)(uintptr_t)0);
        return;
    }

    press_scan_judge_log(&judge);
    lv_async_call(press_report_async_cb, (void *)(uintptr_t)judge.status);
}

void net_tester_press_set_result_cb(net_tester_press_result_cb_t cb, void *user_data)
{
    g_cb = cb;
    g_user_data = user_data;
}

void net_tester_press_start(void)
{
    if(g_tid != RT_NULL) {
        /* 已在扫描：请求中止后由新一轮接管较复杂，这里直接忽略重复开始 */
        rt_kprintf("[PRESS] already running, ignore start\n");
        return;
    }

    g_abort = false;
    g_waiting = true;
    rt_kprintf("[PRESS] start press crimp test\n");

    g_tid = rt_thread_create("press_scan",
                             press_scan_thread_entry,
                             RT_NULL,
                             PRESS_SCAN_STACK,
                             PRESS_SCAN_PRIO,
                             20);
    if(g_tid == RT_NULL) {
        g_waiting = false;
        rt_kprintf("[PRESS] create thread failed\n");
        return;
    }
    rt_thread_startup(g_tid);
}

void net_tester_press_stop(void)
{
    if(!g_waiting && g_tid == RT_NULL) {
        return;
    }

    g_abort = true;
    g_waiting = false;
    rt_kprintf("[PRESS] stop / cancel waiting\n");
}

bool net_tester_press_is_waiting(void)
{
    return g_waiting;
}

void net_tester_press_report_status(uint8_t status)
{
    if(!g_waiting) {
        rt_kprintf("[PRESS] ignore report: not waiting\n");
        return;
    }

    g_waiting = false;
    log_status(status);

    if(g_cb) {
        g_cb(status, g_user_data);
    }
}

void net_tester_press_simulate_async_done(void)
{
    uint8_t status;
    int i;

    if(!g_waiting) {
        rt_kprintf("[PRESS] 空格忽略: 尚未开始或已收到结果\n");
        return;
    }

    /* 硬件扫描进行中时不允许空格模拟打断 */
    if(g_tid != RT_NULL) {
        rt_kprintf("[PRESS] 空格忽略: 硬件扫描中\n");
        return;
    }

    ensure_seed();
    status = (uint8_t)(rand() & 0xFF);
    for(i = 0; i < 8; i++) {
        if((rand() % 100) < 55) {
            status |= (uint8_t)(1u << i);
        }
    }

    rt_kprintf("[PRESS] 空格模拟：上报结果\n");
    net_tester_press_report_status(status);
}
