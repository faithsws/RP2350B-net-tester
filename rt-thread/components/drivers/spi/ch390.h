/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date Author Notes
 * 2026-07-07 Liu Changjie add WCH CH390 SPI Ethernet driver
 */

#ifndef __CH390_H__
#define __CH390_H__

#include <rtthread.h>
#include <rtdevice.h>
#ifdef __cplusplus
extern "C" {
#endif

struct ch390_config
{
 const char *spi_device_name;
 const char *netif_name;
 rt_base_t rst_pin;
 rt_base_t int_pin;
 const rt_uint8_t *mac;
 rt_uint32_t spi_max_hz;
 rt_uint16_t poll_interval_ms;
};

rt_err_t ch390_attach(const struct ch390_config *config);
void ch390_isr(void);

/* PHY 电源：RT_FALSE=掉电断链（交换机口灯灭），RT_TRUE=上电协商 */
rt_err_t ch390_phy_power_set(rt_bool_t enable);
rt_bool_t ch390_phy_power_get(void);
rt_bool_t ch390_link_is_up(void);

/* 链路状态：来自 NSR(LINKST/SPEED) + NCR(FDX) */
struct ch390_link_info
{
    rt_bool_t link_up;
    rt_uint16_t speed_mbps;   /* 10 或 100；未链接时为 0 */
    rt_bool_t full_duplex;
    rt_uint8_t nsr;
    rt_uint8_t ncr;
};
rt_err_t ch390_get_link_info(struct ch390_link_info *info);

#ifdef __cplusplus
}
#endif

#endif /* __CH390_H__ */
