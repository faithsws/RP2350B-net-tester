/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2021-01-28     flybreak       first version
 */

#ifndef __BOARD_H__
#define __BOARD_H__

/*
 * RP2350 默认启用 GPIO 协处理器：gpio_set_dir/gpio_put 走 gpioc，
 * 而 gpio_get() 仍读 SIO。RT-Thread pin 驱动混用后，高位脚(32+)输入
 * 常出现恒为低。关闭协处理器，统一走 SIO（与 gpio_get 一致）。
 */
#ifndef PICO_USE_GPIO_COPROCESSOR
#define PICO_USE_GPIO_COPROCESSOR 0
#endif

/* 本板为 RP2350B：48 路 GPIO。勿按 Pico2/RP2350A（30 路）编译。 */
#ifndef PICO_RP2350A
#define PICO_RP2350A 0
#endif

#define PICO_SRAM_SIZE         520
#define PICO_SRAM_END          (0x20000000 + PICO_SRAM_SIZE * 1024)

extern int _sstack;
#define HEAP_BEGIN      (&_sstack)
#define HEAP_END        ((void *)PICO_SRAM_END)

#endif
