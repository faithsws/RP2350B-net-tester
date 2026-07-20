/*
 * 以太网链路信息 FinSH：Link / 速率 / 双工（读 CH390 NSR/NCR）
 *
 * 用法: eth_info
 *
 * 说明:
 *   ifconfig 只显示 UP/LINK_UP 和 IP，不含协商速率/双工。
 *   速率/双工请用本命令查看。
 */
#include <rtthread.h>

#if defined(BSP_USING_CH390) && defined(RT_USING_FINSH)

#include <ch390.h>
#include <netdev_ipaddr.h>
#include <netdev.h>
#include <finsh.h>

#ifndef BSP_CH390_NETIF_NAME
#define BSP_CH390_NETIF_NAME "e0"
#endif

static int cmd_eth_info(int argc, char **argv)
{
    struct ch390_link_info info;
    struct netdev *nd;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (ch390_get_link_info(&info) != RT_EOK)
    {
        rt_kprintf("[ETH] CH390 not ready\n");
        return -1;
    }

    rt_kprintf("[ETH] ----- CH390 link -----\n");
    rt_kprintf("[ETH] link   : %s\n", info.link_up ? "UP" : "DOWN");
    if (info.link_up)
    {
        rt_kprintf("[ETH] speed  : %u Mbps\n", (unsigned)info.speed_mbps);
        rt_kprintf("[ETH] duplex : %s\n", info.full_duplex ? "full" : "half");
    }
    else
    {
        rt_kprintf("[ETH] speed  : n/a\n");
        rt_kprintf("[ETH] duplex : n/a\n");
    }
    rt_kprintf("[ETH] PHY    : %s\n", ch390_phy_power_get() ? "on" : "off");
    rt_kprintf("[ETH] NSR=0x%02X NCR=0x%02X\n", info.nsr, info.ncr);

    nd = netdev_get_by_name(BSP_CH390_NETIF_NAME);
    if (nd == RT_NULL)
    {
        nd = netdev_default;
    }
    if (nd)
    {
        rt_kprintf("[ETH] netdev : %s  %s %s\n",
                   nd->name,
                   netdev_is_up(nd) ? "UP" : "DOWN",
                   netdev_is_link_up(nd) ? "LINK_UP" : "LINK_DOWN");
        rt_kprintf("[ETH] mac    : %02X:%02X:%02X:%02X:%02X:%02X\n",
                   nd->hwaddr[0], nd->hwaddr[1], nd->hwaddr[2],
                   nd->hwaddr[3], nd->hwaddr[4], nd->hwaddr[5]);
        rt_kprintf("[ETH] ip     : %s\n", inet_ntoa(nd->ip_addr));
        rt_kprintf("[ETH] gw     : %s\n", inet_ntoa(nd->gw));
    }

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_eth_info, eth_info, show ETH link speed duplex);

#endif /* BSP_USING_CH390 && RT_USING_FINSH */
