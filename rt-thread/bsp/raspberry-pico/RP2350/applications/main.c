/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2025-08-04     hydevcode       first version
 * 2026-07-10     cursor          GPIO0 LED 点灯，使用 RT-Thread 线程
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <power_hold.h>

#define LED_PIN 38

static void led_thread_entry(void *parameter)
{
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_pin_write(LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }
}

int main(void)
{
    rt_thread_t tid;

    power_hold_on();
    rt_kprintf("Hello, RT-Thread! GPIO%d LED blink, power hold on GPIO%d\n",
               LED_PIN, POWER_HOLD_PIN);

    tid = rt_thread_create("led",
                           led_thread_entry,
                           RT_NULL,
                           512,
                           RT_THREAD_PRIORITY_MAX / 2,
                           10);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("create led thread failed!\n");
    }

    return 0;
}
