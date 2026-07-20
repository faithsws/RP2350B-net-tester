/*
 * 电池电压 ADC 采集
 *
 * GPIO42 -> ADC2（RP2350B），外部分压后 ADC 端为真实电压的一半。
 */
#ifndef __BATTERY_ADC_H__
#define __BATTERY_ADC_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_ADC_GPIO        42
#define BATTERY_ADC_CHANNEL     2

/* 分压比：Vbat = Vadc * (NUM / DEN)，默认 1/2 分压 => 真实电压为 ADC 端 2 倍 */
#define BATTERY_DIVIDER_NUM     2
#define BATTERY_DIVIDER_DEN     1

#define BATTERY_ADC_VREF_MV     3300
#define BATTERY_ADC_MAX_RAW     4095

void battery_adc_init(void);
rt_uint16_t battery_adc_read_raw(void);
rt_uint32_t battery_adc_read_pin_mv(void);
rt_uint32_t battery_adc_read_mv(void);
float battery_adc_read_voltage(void);

#ifdef __cplusplus
}
#endif

#endif /* __BATTERY_ADC_H__ */
