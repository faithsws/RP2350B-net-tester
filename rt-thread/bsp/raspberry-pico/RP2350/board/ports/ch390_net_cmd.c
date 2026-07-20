/*
 * CH390 网络 FinSH 测试命令
 *
 * 用法:
 *   ch390_info                 查看网卡/链路/IP
 *   ch390_dhcp [超时秒]        重启 DHCP 并等待拿到 IP（默认 30s）
 *   ch390_ping [目标] [次数]   ping（默认 192.168.91.1，4 次）
 *   ch390_test [目标] [超时秒] DHCP + ping 一键自检
 */
#include <rtthread.h>
#include <rtdevice.h>

#ifdef BSP_USING_CH390

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdev_ipaddr.h>
#include <netdev.h>

#ifndef BSP_CH390_NETIF_NAME
#define BSP_CH390_NETIF_NAME "e0"
#endif

#define CH390_TEST_DEFAULT_HOST   "192.168.91.1"
#define CH390_DHCP_DEFAULT_SEC    30
#define CH390_PING_DEFAULT_TIMES  4

#ifdef RT_USING_FINSH
#include <finsh.h>

/* netdev.c 中已导出，头文件未声明 */
extern int netdev_cmd_ping(char *target_name, char *netdev_name,
                           rt_uint32_t times, rt_size_t size);

static struct netdev *ch390_get_netdev(void)
{
    struct netdev *netdev;

    netdev = netdev_get_by_name(BSP_CH390_NETIF_NAME);
    if (netdev == RT_NULL)
    {
        /* 兼容偶发命名差异：取默认网卡 */
        netdev = netdev_default;
    }
    return netdev;
}

static void ch390_print_info(struct netdev *netdev)
{
    if (netdev == RT_NULL)
    {
        rt_kprintf("[CH390] netdev \"%s\" not found\n", BSP_CH390_NETIF_NAME);
        return;
    }

    rt_kprintf("[CH390] name=%s\n", netdev->name);
    rt_kprintf("[CH390] flags: %s %s %s\n",
               netdev_is_up(netdev) ? "UP" : "DOWN",
               netdev_is_link_up(netdev) ? "LINK_UP" : "LINK_DOWN",
               netdev_is_dhcp_enabled(netdev) ? "DHCP_ON" : "DHCP_OFF");
    rt_kprintf("[CH390] mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
               netdev->hwaddr[0], netdev->hwaddr[1], netdev->hwaddr[2],
               netdev->hwaddr[3], netdev->hwaddr[4], netdev->hwaddr[5]);
    rt_kprintf("[CH390] ip : %s\n", inet_ntoa(netdev->ip_addr));
    rt_kprintf("[CH390] gw : %s\n", inet_ntoa(netdev->gw));
    rt_kprintf("[CH390] mask: %s\n", inet_ntoa(netdev->netmask));
}

/* 等待链路 UP */
static rt_err_t ch390_wait_link(struct netdev *netdev, int timeout_sec)
{
    int i;

    for (i = 0; i < timeout_sec * 2; i++)
    {
        if (netdev_is_up(netdev) && netdev_is_link_up(netdev))
        {
            return RT_EOK;
        }
        rt_thread_mdelay(500);
    }
    return -RT_ETIMEOUT;
}

/*
 * 强制重启 DHCP（若已是 DHCP_ON，直接 open 不会再次 dhcp_start）
 * 成功拿到非 0.0.0.0 地址返回 RT_EOK
 */
static rt_err_t ch390_dhcp_restart_wait(struct netdev *netdev, int timeout_sec)
{
    int i;
    rt_err_t err;

    if (netdev == RT_NULL)
    {
        return -RT_ERROR;
    }

    rt_kprintf("[CH390] wait link (timeout %ds)...\n", timeout_sec);
    if (ch390_wait_link(netdev, timeout_sec) != RT_EOK)
    {
        rt_kprintf("[CH390] link down, abort DHCP\n");
        return -RT_ERROR;
    }
    rt_kprintf("[CH390] link up\n");

    /* 先关再开，确保真正重新 dhcp_start */
    err = netdev_dhcp_enabled(netdev, RT_FALSE);
    if (err != RT_EOK)
    {
        rt_kprintf("[CH390] dhcp disable failed: %d\n", (int)err);
    }
    rt_thread_mdelay(100);

    err = netdev_dhcp_enabled(netdev, RT_TRUE);
    if (err != RT_EOK)
    {
        rt_kprintf("[CH390] dhcp enable failed: %d\n", (int)err);
        return err;
    }

    rt_kprintf("[CH390] DHCP started, waiting IP...\n");
    for (i = 0; i < timeout_sec * 2; i++)
    {
        if (!ip_addr_isany(&netdev->ip_addr))
        {
            rt_kprintf("[CH390] DHCP OK: ip=%s gw=%s mask=%s\n",
                       inet_ntoa(netdev->ip_addr),
                       inet_ntoa(netdev->gw),
                       inet_ntoa(netdev->netmask));
            return RT_EOK;
        }
        rt_thread_mdelay(500);
    }

    rt_kprintf("[CH390] DHCP timeout (%ds), ip still 0.0.0.0\n", timeout_sec);
    return -RT_ETIMEOUT;
}

static int cmd_ch390_info(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    ch390_print_info(ch390_get_netdev());
    return 0;
}

static int cmd_ch390_dhcp(int argc, char **argv)
{
    struct netdev *netdev;
    int timeout_sec = CH390_DHCP_DEFAULT_SEC;

    if (argc >= 2)
    {
        timeout_sec = atoi(argv[1]);
        if (timeout_sec <= 0)
        {
            timeout_sec = CH390_DHCP_DEFAULT_SEC;
        }
    }

    netdev = ch390_get_netdev();
    if (netdev == RT_NULL)
    {
        rt_kprintf("[CH390] netdev not found\n");
        return -1;
    }

    return (ch390_dhcp_restart_wait(netdev, timeout_sec) == RT_EOK) ? 0 : -1;
}

static int cmd_ch390_ping(int argc, char **argv)
{
    const char *host = CH390_TEST_DEFAULT_HOST;
    rt_uint32_t times = CH390_PING_DEFAULT_TIMES;
    struct netdev *netdev;

    if (argc >= 2)
    {
        host = argv[1];
    }
    if (argc >= 3)
    {
        times = (rt_uint32_t)atoi(argv[2]);
        if (times == 0)
        {
            times = CH390_PING_DEFAULT_TIMES;
        }
    }

    netdev = ch390_get_netdev();
    if (netdev == RT_NULL)
    {
        rt_kprintf("[CH390] netdev not found\n");
        return -1;
    }

    if (ip_addr_isany(&netdev->ip_addr))
    {
        rt_kprintf("[CH390] no IP yet, run ch390_dhcp first\n");
        return -1;
    }

    rt_kprintf("[CH390] ping %s via %s, times=%u\n",
               host, netdev->name, times);
    return netdev_cmd_ping((char *)host, netdev->name, times, 0);
}

/*
 * 一键自检：等链路 → DHCP → ping
 * ch390_test [目标IP] [超时秒]
 */
static int cmd_ch390_test(int argc, char **argv)
{
    const char *host = CH390_TEST_DEFAULT_HOST;
    int timeout_sec = CH390_DHCP_DEFAULT_SEC;
    struct netdev *netdev;

    if (argc >= 2)
    {
        host = argv[1];
    }
    if (argc >= 3)
    {
        timeout_sec = atoi(argv[2]);
        if (timeout_sec <= 0)
        {
            timeout_sec = CH390_DHCP_DEFAULT_SEC;
        }
    }

    netdev = ch390_get_netdev();
    if (netdev == RT_NULL)
    {
        rt_kprintf("[CH390] FAIL: netdev \"%s\" not found (驱动未起来?)\n",
                   BSP_CH390_NETIF_NAME);
        return -1;
    }

    rt_kprintf("[CH390] ===== CH390 self-test start =====\n");
    ch390_print_info(netdev);

    if (ch390_dhcp_restart_wait(netdev, timeout_sec) != RT_EOK)
    {
        rt_kprintf("[CH390] FAIL: DHCP\n");
        return -1;
    }

    rt_kprintf("[CH390] ping %s ...\n", host);
    netdev_cmd_ping((char *)host, netdev->name, CH390_PING_DEFAULT_TIMES, 0);

    rt_kprintf("[CH390] ===== CH390 self-test done =====\n");
    ch390_print_info(netdev);
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_ch390_info, ch390_info, show CH390 netif status);
MSH_CMD_EXPORT_ALIAS(cmd_ch390_dhcp, ch390_dhcp, restart DHCP and wait IP);
MSH_CMD_EXPORT_ALIAS(cmd_ch390_ping, ch390_ping, ping host via CH390);
MSH_CMD_EXPORT_ALIAS(cmd_ch390_test, ch390_test, DHCP then ping 192.168.91.1);

#endif /* RT_USING_FINSH */

#endif /* BSP_USING_CH390 */
