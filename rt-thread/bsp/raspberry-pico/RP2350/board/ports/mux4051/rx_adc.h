/*
 * 接收通路电压 ADC（经 RX CD4051 公共端）
 *
 * GPIO45 -> ADC5
 */
#ifndef __RX_ADC_H__
#define __RX_ADC_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RX_ADC_GPIO           45
#define RX_ADC_CHANNEL        5

#define RX_ADC_VREF_MV        3300
#define RX_ADC_MAX_RAW        4095

void rx_adc_init(void);
rt_uint16_t rx_adc_read_raw(void);
rt_uint32_t rx_adc_read_pin_mv(void);  /* ADC 引脚电压 mV */

#ifdef __cplusplus
}
#endif

#endif /* __RX_ADC_H__ */
