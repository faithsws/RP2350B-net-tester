/*
 * ARP FinSH：按 IP 查询 MAC，或列出 ARP 表
 *
 * 用法:
 *   arp                 列出 ARP 缓存
 *   arp <IP>            查询该 IP 的 MAC（发 ARP 请求并等待应答）
 *   arp <IP> [超时ms]   默认超时 2000ms
 *
 * 注意：勿与 netdev_ipaddr.h 混用 lwIP 头，避免 ip4_addr_t 重定义。
 */
#include <rtthread.h>

#if defined(RT_USING_LWIP) && defined(RT_USING_FINSH)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lwip/netif.h>
#include <lwip/etharp.h>
#include <lwip/tcpip.h>
#include <lwip/ip4_addr.h>
#include <netif/ethernet.h>

#include <finsh.h>

#define ARP_DEFAULT_TIMEOUT_MS  2000

struct arp_req_arg
{
    struct netif *netif;
    ip4_addr_t ip;
    err_t err;
    struct rt_semaphore done;
};

static struct netif *arp_get_netif(void)
{
    struct netif *nif;

    NETIF_FOREACH(nif)
    {
        if (nif->name[0] == 'e' && nif->name[1] == '0')
        {
            return nif;
        }
    }
    return netif_default;
}

static void arp_request_cb(void *arg)
{
    struct arp_req_arg *a = (struct arp_req_arg *)arg;
    a->err = etharp_request(a->netif, &a->ip);
    rt_sem_release(&a->done);
}

static int arp_list_table(void)
{
    size_t i;
    int count = 0;

    rt_kprintf("[ARP] ----- cache -----\n");
    for (i = 0; i < ARP_TABLE_SIZE; i++)
    {
        ip4_addr_t *ip = RT_NULL;
        struct netif *nif = RT_NULL;
        struct eth_addr *eth = RT_NULL;

        if (etharp_get_entry(i, &ip, &nif, &eth))
        {
            count++;
            rt_kprintf("[ARP] %s at %02X:%02X:%02X:%02X:%02X:%02X  (%c%c)\n",
                       ip4addr_ntoa(ip),
                       eth->addr[0], eth->addr[1], eth->addr[2],
                       eth->addr[3], eth->addr[4], eth->addr[5],
                       nif->name[0], nif->name[1]);
        }
    }
    if (count == 0)
    {
        rt_kprintf("[ARP] (empty)\n");
    }
    else
    {
        rt_kprintf("[ARP] %d entr%s\n", count, count > 1 ? "ies" : "y");
    }
    return 0;
}

static int arp_query_ip(const char *ipstr, int timeout_ms)
{
    struct netif *netif;
    struct arp_req_arg arg;
    struct eth_addr *eth_ret = RT_NULL;
    const ip4_addr_t *ip_ret = RT_NULL;
    rt_tick_t start;
    ssize_t idx;
    err_t cerr;

    netif = arp_get_netif();
    if (netif == RT_NULL)
    {
        rt_kprintf("[ARP] netif not found\n");
        return -1;
    }

    if (!ip4addr_aton(ipstr, &arg.ip))
    {
        rt_kprintf("[ARP] bad IP: %s\n", ipstr);
        return -1;
    }

    if (!netif_is_up(netif) || !netif_is_link_up(netif))
    {
        rt_kprintf("[ARP] netif link down\n");
        return -1;
    }

    idx = etharp_find_addr(netif, &arg.ip, &eth_ret, &ip_ret);
    if (idx >= 0 && eth_ret != RT_NULL)
    {
        rt_kprintf("[ARP] %s is at %02X:%02X:%02X:%02X:%02X:%02X (cached)\n",
                   ipstr,
                   eth_ret->addr[0], eth_ret->addr[1], eth_ret->addr[2],
                   eth_ret->addr[3], eth_ret->addr[4], eth_ret->addr[5]);
        return 0;
    }

    arg.netif = netif;
    arg.err = ERR_VAL;
    if (rt_sem_init(&arg.done, "arp_rq", 0, RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[ARP] sem init failed\n");
        return -1;
    }
    cerr = tcpip_callback(arp_request_cb, &arg);
    if (cerr != ERR_OK)
    {
        rt_sem_detach(&arg.done);
        rt_kprintf("[ARP] tcpip_callback failed %d\n", (int)cerr);
        return -1;
    }
    if (rt_sem_take(&arg.done, rt_tick_from_millisecond(1000)) != RT_EOK)
    {
        rt_sem_detach(&arg.done);
        rt_kprintf("[ARP] request callback timeout\n");
        return -1;
    }
    rt_sem_detach(&arg.done);
    if (arg.err != ERR_OK)
    {
        rt_kprintf("[ARP] request send failed %d\n", (int)arg.err);
        return -1;
    }

    rt_kprintf("[ARP] who-has %s ? waiting %dms...\n", ipstr, timeout_ms);
    start = rt_tick_get();
    while ((rt_tick_get() - start) < rt_tick_from_millisecond(timeout_ms))
    {
        idx = etharp_find_addr(netif, &arg.ip, &eth_ret, &ip_ret);
        if (idx >= 0 && eth_ret != RT_NULL)
        {
            rt_kprintf("[ARP] %s is at %02X:%02X:%02X:%02X:%02X:%02X\n",
                       ipstr,
                       eth_ret->addr[0], eth_ret->addr[1], eth_ret->addr[2],
                       eth_ret->addr[3], eth_ret->addr[4], eth_ret->addr[5]);
            return 0;
        }
        rt_thread_mdelay(50);
    }

    rt_kprintf("[ARP] %s no reply (timeout)\n", ipstr);
    return -1;
}

static int cmd_arp(int argc, char **argv)
{
    int timeout_ms = ARP_DEFAULT_TIMEOUT_MS;

    if (argc == 1)
    {
        return arp_list_table();
    }

    if (argc >= 3)
    {
        timeout_ms = atoi(argv[2]);
        if (timeout_ms <= 0)
        {
            timeout_ms = ARP_DEFAULT_TIMEOUT_MS;
        }
    }

    if (argc >= 2 && argv[1][0] != '\0')
    {
        if (strcmp(argv[1], "-a") == 0)
        {
            return arp_list_table();
        }
        return arp_query_ip(argv[1], timeout_ms);
    }

    rt_kprintf("Usage:\n");
    rt_kprintf("  arp              list ARP table\n");
    rt_kprintf("  arp <IP> [ms]    resolve IP to MAC\n");
    return -1;
}

MSH_CMD_EXPORT_ALIAS(cmd_arp, arp, ARP resolve IP to MAC / list table);

#endif /* RT_USING_LWIP && RT_USING_FINSH */
