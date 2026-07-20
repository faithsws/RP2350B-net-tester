/*
 * 对线通路电压 ADC（经 CD4051 公共端）
 *
 * GPIO41 -> ADC1，外部分压后 ADC 端为真实电压的 1/3。
 */
#ifndef __LINK_ADC_H__
#define __LINK_ADC_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LINK_ADC_GPIO           41
#define LINK_ADC_CHANNEL        1

/* 分压比：Vreal = Vadc * (NUM / DEN)，1/3 分压 => 真实电压为 ADC 端 3 倍 */
#define LINK_DIVIDER_NUM        3
#define LINK_DIVIDER_DEN        1

#define LINK_ADC_VREF_MV        3300
#define LINK_ADC_MAX_RAW        4095

void link_adc_init(void);
rt_uint16_t link_adc_read_raw(void);
rt_uint32_t link_adc_read_pin_mv(void);  /* ADC 引脚电压 mV */
rt_uint32_t link_adc_read_mv(void);      /* 还原后真实电压 mV */
float link_adc_read_voltage(void);

#ifdef __cplusplus
}
#endif

#endif /* __LINK_ADC_H__ */
