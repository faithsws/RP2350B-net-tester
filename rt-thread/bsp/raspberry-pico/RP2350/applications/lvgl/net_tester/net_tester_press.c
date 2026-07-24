#include "net_tester_press.h"

#include <stdio.h>
#include <stdlib.h>

#include <lvgl.h>
#include <rtthread.h>

static net_tester_press_result_cb_t g_cb;
static void *g_user_data;
static bool g_seeded;
static bool g_waiting;

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

void net_tester_press_set_result_cb(net_tester_press_result_cb_t cb, void *user_data)
{
    g_cb = cb;
    g_user_data = user_data;
}

void net_tester_press_start(void)
{
    g_waiting = true;
    rt_kprintf("[PRESS] start async press test (waiting callback)\n");
}

void net_tester_press_stop(void)
{
    if(!g_waiting) {
        return;
    }
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
