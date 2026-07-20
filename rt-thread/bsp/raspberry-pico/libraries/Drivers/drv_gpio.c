/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2021-01-28     flybreak       first version
 * 2026-07-15     cursor         修复 RP2350B 输入上拉/方向；避免与 GPIO 协处理器不同步
 */
#include "drv_gpio.h"
#include <hardware/gpio.h>
#include <hardware/platform_defs.h>

static void pico_pin_mode(struct rt_device *dev, rt_base_t pin, rt_uint8_t mode)
{
    RT_ASSERT((0 <= pin) && (pin < NUM_BANK0_GPIOS));

    /* 切到 SIO，解除 pad ISO，并打开输入使能 */
    gpio_init(pin);
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_input_enabled(pin, true);

    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        gpio_disable_pulls(pin);
        gpio_set_dir(pin, GPIO_OUT);
        break;
    case PIN_MODE_INPUT:
        gpio_disable_pulls(pin);
        gpio_set_dir(pin, GPIO_IN);
        break;
    case PIN_MODE_INPUT_PULLUP:
        /* 原先只调了 gpio_pull_up，未明确设为输入，RP2350B 高位脚会一直读到 0 */
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_down(pin);
        break;
    case PIN_MODE_OUTPUT_OD:
        gpio_set_dir(pin, GPIO_OUT);
        gpio_disable_pulls(pin);
        break;
    default:
        break;
    }
}

static void pico_pin_write(struct rt_device *dev, rt_base_t pin, rt_uint8_t value)
{
    RT_ASSERT((0 <= pin) && (pin < NUM_BANK0_GPIOS));
    gpio_put(pin, value ? 1 : 0);
}

static rt_ssize_t pico_pin_read(struct rt_device *device, rt_base_t pin)
{
    RT_ASSERT((0 <= pin) && (pin < NUM_BANK0_GPIOS));
    return gpio_get(pin) ? PIN_HIGH : PIN_LOW;
}

static const struct rt_pin_ops ops =
{
    pico_pin_mode,
    pico_pin_write,
    pico_pin_read,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
};

int rt_hw_gpio_init(void)
{
    rt_device_pin_register("gpio", &ops, RT_NULL);

    return 0;
}
/* 须早于 INIT_DEVICE 级外设（如 CH390/LCD）注册，否则 rt_pin_mode 时空 ops 会 HardFault */
INIT_BOARD_EXPORT(rt_hw_gpio_init);
