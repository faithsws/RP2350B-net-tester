#include "net_tester_trace.h"

#include "sc_link_pwm.h"

#include <rtthread.h>

static bool g_trace_running;

void net_tester_trace_start(void)
{
    if(g_trace_running) {
        return;
    }

    /* 寻线载波：LINK_PWM GPIO19 @ 460kHz */
    if(link_pwm_start(CARRIER_PWM_FREQ_460KHZ) != RT_EOK) {
        rt_kprintf("[TRACE] LINK_PWM 460kHz start failed\n");
        return;
    }

    g_trace_running = true;
    rt_kprintf("[TRACE] start, LINK_PWM 460kHz on\n");
}

void net_tester_trace_stop(void)
{
    if(!g_trace_running) {
        /* 仍尝试关断，避免异常路径残留输出 */
        link_pwm_stop();
        return;
    }

    g_trace_running = false;
    link_pwm_stop();
    rt_kprintf("[TRACE] stop, LINK_PWM off\n");
}

bool net_tester_trace_is_running(void)
{
    return g_trace_running;
}
