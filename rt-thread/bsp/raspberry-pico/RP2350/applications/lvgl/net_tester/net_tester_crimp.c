#include "net_tester_crimp.h"

#include <stdio.h>
#include <stdlib.h>


#include <lvgl.h>
#include <rtthread.h>

static net_tester_crimp_result_cb_t g_cb;
static void * g_user_data;
static bool g_seeded;
static bool g_waiting;

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

static void log_status(uint8_t status)
{
    char bits[9];
    format_bits(status, bits);

    rt_kprintf("[CRIMP] status=0x%02X (%sb)\n", status, bits);
    rt_kprintf("[CRIMP] 牢固线: ");
    bool any = false;
    for(int i = 0; i < 8; i++) {
        if(status & (1u << i)) {
            rt_kprintf("%d ", i + 1);
            any = true;
        }
    }
    if(!any) rt_kprintf("(无)");
    rt_kprintf("\n");
}

void net_tester_crimp_set_result_cb(net_tester_crimp_result_cb_t cb, void * user_data)
{
    g_cb = cb;
    g_user_data = user_data;
}

void net_tester_crimp_start(void)
{
    g_waiting = true;
    rt_kprintf("[CRIMP] start async crimp test (waiting callback)\n");
}

void net_tester_crimp_stop(void)
{
    if(!g_waiting) return;
    g_waiting = false;
    rt_kprintf("[CRIMP] stop / cancel waiting\n");
}

bool net_tester_crimp_is_waiting(void)
{
    return g_waiting;
}

void net_tester_crimp_report_status(uint8_t status)
{
    if(!g_waiting) {
        rt_kprintf("[CRIMP] ignore report: not waiting (click Start first)\n");
        
        return;
    }

    g_waiting = false;
    log_status(status);

    if(g_cb) {
        g_cb(status, g_user_data);
    }
}

void net_tester_crimp_simulate_async_done(void)
{
    if(!g_waiting) {
        rt_kprintf("[CRIMP] 空格忽略: 尚未开始或已收到结果\n");
        
        return;
    }

    ensure_seed();
    uint8_t status = (uint8_t)(rand() & 0xFF);
    /* 避免全灰过于单调，提高“牢固”概率 */
    for(int i = 0; i < 8; i++) {
        if((rand() % 100) < 55) {
            status |= (uint8_t)(1u << i);
        }
    }

    rt_kprintf("[CRIMP] 空格模拟：异步任务结束，上报结果\n");
    net_tester_crimp_report_status(status);
}
