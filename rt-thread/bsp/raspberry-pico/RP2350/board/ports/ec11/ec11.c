/*
 * EC11 旋转编码器驱动（GPIO 边沿中断）
 *
 * A: GPIO33, B: GPIO34, KEY: GPIO35
 * 内部上拉；KEY 低有效仍轮询
 */
#include "ec11.h"
#include <rthw.h>
#include <rtdevice.h>
#include <hardware/gpio.h>

/* 上一拍 AB(2bit) | 当前 AB(2bit) → 步长；非法跳变计 0 */
static const int8_t ec11_dir_table[16] =
{
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static volatile uint8_t ec11_last_ab;
static volatile int16_t ec11_diff_acc;

static uint8_t ec11_read_ab(void)
{
    uint8_t a = gpio_get(EC11_PIN_A) ? 1u : 0u;
    uint8_t b = gpio_get(EC11_PIN_B) ? 1u : 0u;

    return (uint8_t)((a << 1) | b);
}

/* A/B 任一沿：解码正交方向并累加（ISR，勿调用 RT-Thread API） */
static void ec11_gpio_isr(uint gpio, uint32_t events)
{
    uint8_t cur_ab;
    int8_t step;

    RT_UNUSED(gpio);
    RT_UNUSED(events);

    cur_ab = ec11_read_ab();
    step = ec11_dir_table[(ec11_last_ab << 2) | cur_ab];
    if (step != 0)
    {
        ec11_diff_acc = (int16_t)(ec11_diff_acc + step);
    }
    ec11_last_ab = cur_ab;
}

void ec11_init(void)
{
    const uint32_t edge = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;

    rt_pin_mode(EC11_PIN_A, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(EC11_PIN_B, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(EC11_PIN_KEY, PIN_MODE_INPUT_PULLUP);

    /* 施密特触发，减轻机械抖动毛刺 */
    gpio_set_input_hysteresis_enabled(EC11_PIN_A, true);
    gpio_set_input_hysteresis_enabled(EC11_PIN_B, true);

    ec11_last_ab = ec11_read_ab();
    ec11_diff_acc = 0;

    /* A/B 共用同一 GPIO IRQ 回调；先设回调再使能第二脚 */
    gpio_set_irq_enabled_with_callback(EC11_PIN_A, edge, true, ec11_gpio_isr);
    gpio_set_irq_enabled(EC11_PIN_B, edge, true);
}

void ec11_poll(void)
{
    /* 边沿已由中断采集，保留空接口兼容旧调用点 */
}

int16_t ec11_take_diff(void)
{
    int16_t diff;
    rt_base_t level = rt_hw_interrupt_disable();

    diff = ec11_diff_acc;
    ec11_diff_acc = 0;
    rt_hw_interrupt_enable(level);

    return diff;
}

rt_bool_t ec11_key_pressed(void)
{
    return rt_pin_read(EC11_PIN_KEY) == PIN_LOW;
}
