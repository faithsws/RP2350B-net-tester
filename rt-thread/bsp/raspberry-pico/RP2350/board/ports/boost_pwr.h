/*
 * 外部升压电源使能
 *
 * GPIO32：高电平使能升压输出，默认上电关闭。
 */
#ifndef __BOOST_PWR_H__
#define __BOOST_PWR_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOST_PWR_EN_PIN  32

void boost_pwr_init(void);
void boost_pwr_on(void);
void boost_pwr_off(void);
rt_bool_t boost_pwr_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOST_PWR_H__ */
