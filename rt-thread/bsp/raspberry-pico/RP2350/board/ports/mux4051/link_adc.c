/*
 * 对线通路电压 ADC（GPIO41 / ADC1）
 */
#include "link_adc.h"
#include <hardware/adc.h>

#define LINK_ADC_SAMPLE_COUNT  8

static rt_bool_t link_adc_ready = RT_FALSE;

static rt_uint32_t link_adc_raw_to_pin_mv(rt_uint32_t raw)
{
    return (rt_uint32_t)((raw * LINK_ADC_VREF_MV) / LINK_ADC_MAX_RAW);
}

void link_adc_init(void)
{
    adc_init();
    adc_gpio_init(LINK_ADC_GPIO);
    adc_select_input(LINK_ADC_CHANNEL);
    link_adc_ready = RT_TRUE;
}

rt_uint16_t link_adc_read_raw(void)
{
    rt_uint32_t sum = 0;
    rt_uint32_t i;

    if (!link_adc_ready)
    {
        link_adc_init();
    }

    adc_select_input(LINK_ADC_CHANNEL);

    /* 丢弃首样，降低通道切换误差 */
    (void)adc_read();

    for (i = 0; i < LINK_ADC_SAMPLE_COUNT; i++)
    {
        sum += adc_read();
    }

    return (rt_uint16_t)(sum / LINK_ADC_SAMPLE_COUNT);
}

rt_uint32_t link_adc_read_pin_mv(void)
{
    return link_adc_raw_to_pin_mv(link_adc_read_raw());
}

rt_uint32_t link_adc_read_mv(void)
{
    rt_uint32_t pin_mv = link_adc_read_pin_mv();

    return (pin_mv * LINK_DIVIDER_NUM) / LINK_DIVIDER_DEN;
}

float link_adc_read_voltage(void)
{
    return (float)link_adc_read_mv() / 1000.0f;
}

static int link_adc_auto_init(void)
{
    link_adc_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(link_adc_auto_init);

#ifdef RT_USING_FINSH
#include <finsh.h>

static int cmd_link_adc(int argc, char **argv)
{
    rt_uint16_t raw = link_adc_read_raw();

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("GPIO%d ADC%d raw=%u\n", LINK_ADC_GPIO, LINK_ADC_CHANNEL, raw);
    rt_kprintf("ADC pin: %u mV\n", link_adc_read_pin_mv());
    rt_kprintf("Real (x%d): %u mV (%.3f V)\n",
               LINK_DIVIDER_NUM, link_adc_read_mv(), link_adc_read_voltage());
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_link_adc, link_adc, read link path ADC voltage);
#endif /* RT_USING_FINSH */
