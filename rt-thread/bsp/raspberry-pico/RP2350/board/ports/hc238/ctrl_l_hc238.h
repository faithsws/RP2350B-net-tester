/*
 * CTRL_L：H 桥下管 NMOS 74HC238
 *
 * A2=GPIO25, A1=GPIO26, A0=GPIO27，EN 与 CTRL_H 共用 GPIO24
 */
#ifndef __CTRL_L_HC238_H__
#define __CTRL_L_HC238_H__

#include "hc238.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CTRL_L_A2_PIN  25
#define CTRL_L_A1_PIN  26
#define CTRL_L_A0_PIN  27

void ctrl_l_hc238_init(void);
rt_err_t ctrl_l_hc238_select(rt_uint8_t channel);
rt_uint8_t ctrl_l_hc238_get_channel(void);

#ifdef __cplusplus
}
#endif

#endif /* __CTRL_L_HC238_H__ */
