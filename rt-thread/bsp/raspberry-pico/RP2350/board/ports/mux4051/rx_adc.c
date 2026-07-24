/*
 * 接收通路电压 ADC（GPIO45 / ADC5）
 */
#include "rx_adc.h"
#include <hardware/adc.h>

#define RX_ADC_SAMPLE_COUNT  8

static rt_bool_t rx_adc_ready = RT_FALSE;

static rt_uint32_t rx_adc_raw_to_pin_mv(rt_uint32_t raw)
{
    return (rt_uint32_t)((raw * RX_ADC_VREF_MV) / RX_ADC_MAX_RAW);
}

void rx_adc_init(void)
{
    adc_init();
    adc_gpio_init(RX_ADC_GPIO);
    adc_select_input(RX_ADC_CHANNEL);
    rx_adc_ready = RT_TRUE;
}

rt_uint16_t rx_adc_read_raw(void)
{
    rt_uint32_t sum = 0;
    rt_uint32_t i;

    if (!rx_adc_ready)
    {
        rx_adc_init();
    }

    adc_select_input(RX_ADC_CHANNEL);

    /* 丢弃首样，降低通道切换误差 */
    (void)adc_read();

    for (i = 0; i < RX_ADC_SAMPLE_COUNT; i++)
    {
        sum += adc_read();
    }

    return (rt_uint16_t)(sum / RX_ADC_SAMPLE_COUNT);
}

rt_uint32_t rx_adc_read_pin_mv(void)
{
    return rx_adc_raw_to_pin_mv(rx_adc_read_raw());
}

static int rx_adc_auto_init(void)
{
    rx_adc_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rx_adc_auto_init);

#ifdef RT_USING_FINSH
#include <finsh.h>

static int cmd_rx_adc(int argc, char **argv)
{
    rt_uint16_t raw = rx_adc_read_raw();

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("GPIO%d ADC%d raw=%u\n", RX_ADC_GPIO, RX_ADC_CHANNEL, raw);
    rt_kprintf("ADC pin: %u mV\n", rx_adc_read_pin_mv());
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_rx_adc, rx_adc, read RX path ADC voltage GPIO45);
#endif /* RT_USING_FINSH */
