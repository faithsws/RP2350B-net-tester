/*
 * SC_PWM / LINK_PWM 载波输出
 *
 * GPIO17 (SC_PWM)、GPIO19 (LINK_PWM) 输出 455kHz 或 460kHz、50% 占空比方波。
 */
#ifndef __SC_LINK_PWM_H__
#define __SC_LINK_PWM_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SC_PWM_PIN      17
#define LINK_PWM_PIN    19

typedef enum
{
    CARRIER_PWM_FREQ_455KHZ = 455,
    CARRIER_PWM_FREQ_460KHZ = 460,
} carrier_pwm_freq_t;

void sc_link_pwm_init(void);

rt_err_t sc_pwm_start(carrier_pwm_freq_t freq);
rt_err_t sc_pwm_stop(void);
rt_bool_t sc_pwm_is_running(void);
carrier_pwm_freq_t sc_pwm_get_freq(void);

rt_err_t link_pwm_start(carrier_pwm_freq_t freq);
rt_err_t link_pwm_stop(void);
rt_bool_t link_pwm_is_running(void);
carrier_pwm_freq_t link_pwm_get_freq(void);

#ifdef __cplusplus
}
#endif

#endif /* __SC_LINK_PWM_H__ */
