/*
 * 电源保持引脚驱动
 *
 * 直接使用 pico-sdk GPIO，可在 PIN 框架初始化前完成上电锁存。
 */
#include "power_hold.h"
#include <rthw.h>
#include <hardware/gpio.h>

static rt_bool_t power_hold_ready = RT_FALSE;

void power_hold_init(void)
{
    gpio_init(POWER_HOLD_PIN);
    gpio_set_dir(POWER_HOLD_PIN, GPIO_OUT);
    gpio_put(POWER_HOLD_PIN, 1);
    power_hold_ready = RT_TRUE;
}

void power_hold_on(void)
{
    if (!power_hold_ready)
    {
        power_hold_init();
        return;
    }

    gpio_put(POWER_HOLD_PIN, 1);
}

void power_hold_off(void)
{
    if (!power_hold_ready)
    {
        gpio_init(POWER_HOLD_PIN);
        gpio_set_dir(POWER_HOLD_PIN, GPIO_OUT);
        power_hold_ready = RT_TRUE;
    }

    gpio_put(POWER_HOLD_PIN, 0);
}

void rt_hw_cpu_shutdown(void)
{
    power_hold_off();
    rt_hw_interrupt_disable();

    while (1)
    {
        /* 等待外部电源电路切断供电 */
    }
}
