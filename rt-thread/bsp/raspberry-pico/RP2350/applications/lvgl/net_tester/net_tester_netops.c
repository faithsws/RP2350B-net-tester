/*
 * lwIP 侧网络探测（不可与 netdev_ipaddr.h 同编译单元混用）
 */
#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef RT_USING_LWIP

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <lwip/netif.h>
#include <lwip/etharp.h>
#include <lwip/tcpip.h>
#include <lwip/ip4_addr.h>

int net_tester_ops_tcp_probe(const char * host, int port, int timeout_ms, char * out, size_t out_sz)
{
    int sock = -1;
    int ret;
    struct sockaddr_in sa;
    ip4_addr_t addr;
    struct addrinfo hints;
    struct addrinfo * res = RT_NULL;
    int flags;
    fd_set wfds;
    struct timeval tv;
    int so_err = 0;
    socklen_t so_len = sizeof(so_err);

    if(ip4addr_aton(host, &addr) == 0) {
        rt_memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if(lwip_getaddrinfo(host, RT_NULL, &hints, &res) != 0 || res == RT_NULL) {
            rt_snprintf(out, out_sz, "主机解析失败");
            return -2;
        }
        addr.addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
        lwip_freeaddrinfo(res);
    }

    sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        rt_snprintf(out, out_sz, "socket失败");
        return -1;
    }

    flags = lwip_fcntl(sock, F_GETFL, 0);
    lwip_fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    rt_memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)port);
    sa.sin_addr.s_addr = addr.addr;

    ret = lwip_connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    if(ret == 0) {
        lwip_close(sock);
        rt_snprintf(out, out_sz, "TCP开放\n%s:%d", host, port);
        return 0;
    }
    if(errno != EINPROGRESS) {
        lwip_close(sock);
        rt_snprintf(out, out_sz, "TCP关闭/拒绝\n%s:%d", host, port);
        return -3;
    }

    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    ret = lwip_select(sock + 1, RT_NULL, &wfds, RT_NULL, &tv);
    if(ret <= 0) {
        lwip_close(sock);
        rt_snprintf(out, out_sz, "TCP超时\n%s:%d", host, port);
        return -4;
    }
    if(lwip_getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_err, &so_len) < 0 || so_err != 0) {
        lwip_close(sock);
        rt_snprintf(out, out_sz, "TCP关闭/拒绝\n%s:%d", host, port);
        return -3;
    }
    lwip_close(sock);
    rt_snprintf(out, out_sz, "TCP开放\n%s:%d", host, port);
    return 0;
}

int net_tester_ops_udp_probe(const char * host, int port, int timeout_ms, char * out, size_t out_sz)
{
    int sock;
    struct sockaddr_in sa;
    ip4_addr_t addr;
    char payload[4] = {0};
    char rx[64];
    fd_set rfds;
    struct timeval tv;
    int n;

    if(ip4addr_aton(host, &addr) == 0) {
        rt_snprintf(out, out_sz, "请用IP地址\n(不支持域名)");
        return -2;
    }

    sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        rt_snprintf(out, out_sz, "socket失败");
        return -1;
    }

    rt_memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)port);
    sa.sin_addr.s_addr = addr.addr;
    lwip_sendto(sock, payload, sizeof(payload), 0, (struct sockaddr *)&sa, sizeof(sa));

    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    n = lwip_select(sock + 1, &rfds, RT_NULL, RT_NULL, &tv);
    if(n > 0) {
        n = lwip_recv(sock, rx, sizeof(rx), 0);
        lwip_close(sock);
        rt_snprintf(out, out_sz, "UDP有响应\n%s:%d", host, port);
        return (n >= 0) ? 0 : -3;
    }
    lwip_close(sock);
    rt_snprintf(out, out_sz, "UDP无响应\n可能开放或过滤\n%s:%d", host, port);
    return 1;
}

int net_tester_ops_dns_lookup(const char * host, char * out, size_t out_sz)
{
    struct addrinfo hints;
    struct addrinfo * res = RT_NULL;
    struct addrinfo * p;
    int err;
    size_t used = 0;
    int count = 0;

    rt_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    err = lwip_getaddrinfo(host, RT_NULL, &hints, &res);
    if(err != 0 || res == RT_NULL) {
        rt_snprintf(out, out_sz, "DNS失败\n%s", host);
        return -1;
    }

    used = (size_t)rt_snprintf(out, out_sz, "%s\n", host);
    for(p = res; p && used + 20 < out_sz; p = p->ai_next) {
        struct sockaddr_in * sa = (struct sockaddr_in *)p->ai_addr;
        char ipbuf[16];
        ip4_addr_t a;
        a.addr = sa->sin_addr.s_addr;
        ip4addr_ntoa_r(&a, ipbuf, sizeof(ipbuf));
        used += (size_t)rt_snprintf(out + used, out_sz - used, "%s\n", ipbuf);
        count++;
        if(count >= 4) {
            break;
        }
    }
    lwip_freeaddrinfo(res);
    return 0;
}

int net_tester_ops_http_get(const char * host, int port, const char * path,
                            char * out, size_t out_sz)
{
    int sock;
    struct sockaddr_in sa;
    ip4_addr_t addr;
    char req[160];
    char rx[768];
    int n, total = 0;
    int status = 0;
    fd_set rfds;
    struct timeval tv;

    if(path == RT_NULL || path[0] == '\0') {
        path = "/";
    }
    if(ip4addr_aton(host, &addr) == 0) {
        struct addrinfo hints;
        struct addrinfo * res = RT_NULL;
        rt_memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        if(lwip_getaddrinfo(host, RT_NULL, &hints, &res) != 0 || !res) {
            rt_snprintf(out, out_sz, "解析失败");
            return -1;
        }
        addr.addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
        lwip_freeaddrinfo(res);
    }

    sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        rt_snprintf(out, out_sz, "socket失败");
        return -1;
    }

    rt_memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)port);
    sa.sin_addr.s_addr = addr.addr;

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    lwip_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if(lwip_connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        lwip_close(sock);
        rt_snprintf(out, out_sz, "连接失败\n%s:%d", host, port);
        return -1;
    }

    rt_snprintf(req, sizeof(req),
                "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
                path, host);
    lwip_send(sock, req, strlen(req), 0);

    rt_memset(rx, 0, sizeof(rx));
    while(total < (int)sizeof(rx) - 1) {
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        if(lwip_select(sock + 1, &rfds, RT_NULL, RT_NULL, &tv) <= 0) {
            break;
        }
        n = lwip_recv(sock, rx + total, sizeof(rx) - 1 - total, 0);
        if(n <= 0) {
            break;
        }
        total += n;
    }
    lwip_close(sock);
    rx[total] = '\0';

    if(sscanf(rx, "HTTP/%*s %d", &status) == 1) {
        char * body = strstr(rx, "\r\n\r\n");
        rt_snprintf(out, out_sz, "HTTP %d\n%d bytes", status, total);
        if(body && body[4]) {
            size_t used = strlen(out);
            rt_snprintf(out + used, out_sz - used, "\n%.40s", body + 4);
        }
        return 0;
    }
    rt_snprintf(out, out_sz, "无有效响应\n%d bytes", total);
    return -1;
}

struct arp_req_arg {
    struct netif * netif;
    ip4_addr_t ip;
    err_t err;
    struct rt_semaphore done;
};

static void arp_req_cb(void * arg)
{
    struct arp_req_arg * a = (struct arp_req_arg *)arg;
    a->err = etharp_request(a->netif, &a->ip);
    rt_sem_release(&a->done);
}

int net_tester_ops_arp_query(const char * ipstr, char * out, size_t out_sz)
{
    struct netif * nif = netif_default;
    struct arp_req_arg arg;
    const ip4_addr_t * ip_ret;
    struct eth_addr * eth_ret;
    int idx;
    int i;

    if(!nif) {
        rt_snprintf(out, out_sz, "无默认网卡");
        return -1;
    }
    if(!ip4addr_aton(ipstr, &arg.ip)) {
        rt_snprintf(out, out_sz, "IP无效");
        return -1;
    }

    idx = etharp_find_addr(nif, &arg.ip, &eth_ret, &ip_ret);
    if(idx >= 0) {
        rt_snprintf(out, out_sz, "%s\n%02X:%02X:%02X:%02X:%02X:%02X",
                    ipstr,
                    eth_ret->addr[0], eth_ret->addr[1], eth_ret->addr[2],
                    eth_ret->addr[3], eth_ret->addr[4], eth_ret->addr[5]);
        return 0;
    }

    arg.netif = nif;
    if(rt_sem_init(&arg.done, "arpui", 0, RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_snprintf(out, out_sz, "信号量失败");
        return -1;
    }
    if(tcpip_callback(arp_req_cb, &arg) != ERR_OK) {
        rt_sem_detach(&arg.done);
        rt_snprintf(out, out_sz, "请求失败");
        return -1;
    }
    rt_sem_take(&arg.done, rt_tick_from_millisecond(2000));
    rt_sem_detach(&arg.done);

    for(i = 0; i < 20; i++) {
        idx = etharp_find_addr(nif, &arg.ip, &eth_ret, &ip_ret);
        if(idx >= 0) {
            rt_snprintf(out, out_sz, "%s\n%02X:%02X:%02X:%02X:%02X:%02X",
                        ipstr,
                        eth_ret->addr[0], eth_ret->addr[1], eth_ret->addr[2],
                        eth_ret->addr[3], eth_ret->addr[4], eth_ret->addr[5]);
            return 0;
        }
        rt_thread_mdelay(100);
    }
    rt_snprintf(out, out_sz, "ARP超时\n%s", ipstr);
    return -1;
}

#else /* !RT_USING_LWIP */

int net_tester_ops_tcp_probe(const char * host, int port, int timeout_ms, char * out, size_t out_sz)
{
    RT_UNUSED(host); RT_UNUSED(port); RT_UNUSED(timeout_ms);
    rt_snprintf(out, out_sz, "未启用LWIP");
    return -1;
}
int net_tester_ops_udp_probe(const char * host, int port, int timeout_ms, char * out, size_t out_sz)
{
    RT_UNUSED(host); RT_UNUSED(port); RT_UNUSED(timeout_ms);
    rt_snprintf(out, out_sz, "未启用LWIP");
    return -1;
}
int net_tester_ops_dns_lookup(const char * host, char * out, size_t out_sz)
{
    RT_UNUSED(host);
    rt_snprintf(out, out_sz, "未启用LWIP");
    return -1;
}
int net_tester_ops_http_get(const char * host, int port, const char * path,
                            char * out, size_t out_sz)
{
    RT_UNUSED(host); RT_UNUSED(port); RT_UNUSED(path);
    rt_snprintf(out, out_sz, "未启用LWIP");
    return -1;
}
int net_tester_ops_arp_query(const char * ipstr, char * out, size_t out_sz)
{
    RT_UNUSED(ipstr);
    rt_snprintf(out, out_sz, "未启用LWIP");
    return -1;
}

#endif /* RT_USING_LWIP */
