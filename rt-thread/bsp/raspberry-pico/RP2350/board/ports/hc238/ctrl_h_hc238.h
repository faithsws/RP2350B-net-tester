/*
 * CTRL_H：H 桥上管 PMOS 74HC238
 *
 * A2=GPIO21, A1=GPIO22, A0=GPIO23，EN 与 CTRL_L 共用 GPIO24
 */
#ifndef __CTRL_H_HC238_H__
#define __CTRL_H_HC238_H__

#include "hc238.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CTRL_H_A2_PIN  21
#define CTRL_H_A1_PIN  22
#define CTRL_H_A0_PIN  23

void ctrl_h_hc238_init(void);
rt_err_t ctrl_h_hc238_select(rt_uint8_t channel);
rt_uint8_t ctrl_h_hc238_get_channel(void);

#ifdef __cplusplus
}
#endif

#endif /* __CTRL_H_HC238_H__ */
