/*
 * 通用网络工具 FinSH 命令：TCP/UDP/WS/HTTP 探测、DNS 解析
 *
 * 用法:
 *   tcp_probe [主机] [端口] [超时ms]   默认 192.168.91.99 22 3000
 *   udp_probe [主机] [端口] [超时ms]   默认 192.168.91.99 53 3000
 *   ws_probe  [主机] [端口] [路径] [超时ms]  WebSocket 握手探测，默认 80 /
 *   http_get  [主机] [端口] [路径] [超时ms] [body_max]  HTTP GET，默认 80 / 5000 512
 *   dns_lookup [域名]                  默认 baidu.com（解析 A 记录）
 */
#include <rtthread.h>

#ifdef RT_USING_LWIP
#ifdef RT_USING_FINSH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>

#include <finsh.h>

#define NET_TCP_DEFAULT_HOST   "192.168.91.99"
#define NET_TCP_DEFAULT_PORT   22
#define NET_TCP_DEFAULT_TO_MS  3000
#define NET_UDP_DEFAULT_HOST   "192.168.91.99"
#define NET_UDP_DEFAULT_PORT   53
#define NET_UDP_DEFAULT_TO_MS  3000
#define NET_WS_DEFAULT_HOST    "192.168.91.99"
#define NET_WS_DEFAULT_PORT    80
#define NET_WS_DEFAULT_PATH    "/"
#define NET_WS_DEFAULT_TO_MS   3000
#define NET_WS_RX_BUF          768
/* RFC6455 示例 Key，仅用于探测握手 */
#define NET_WS_PROBE_KEY       "dGhlIHNhbXBsZSBub25jZQ=="
#define NET_HTTP_DEFAULT_HOST  "192.168.91.99"
#define NET_HTTP_DEFAULT_PORT  80
#define NET_HTTP_DEFAULT_PATH  "/"
#define NET_HTTP_DEFAULT_TO_MS 5000
#define NET_HTTP_DEFAULT_BODY  512
/* FinSH 栈仅 2048，大缓冲必须用静态区，避免 stack overflow 死循环 */
#define NET_HTTP_RX_BUF        1536
#define NET_DNS_DEFAULT_HOST   "baidu.com"

/* 不区分大小写子串查找 */
static const char *net_strcasestr(const char *hay, const char *needle)
{
    size_t nlen;
    const char *p;

    if (hay == RT_NULL || needle == RT_NULL || needle[0] == '\0')
    {
        return hay;
    }

    nlen = strlen(needle);
    for (p = hay; *p; p++)
    {
        size_t i;
        for (i = 0; i < nlen; i++)
        {
            char a = (char)tolower((unsigned char)p[i]);
            char b = (char)tolower((unsigned char)needle[i]);
            if (a != b)
            {
                break;
            }
        }
        if (i == nlen)
        {
            return p;
        }
        if (p[i] == '\0')
        {
            break;
        }
    }
    return RT_NULL;
}

/* 解析主机名为 IPv4，成功返回 0 并写入 addr */
static int net_resolve_ipv4(const char *host, ip4_addr_t *addr)
{
    struct addrinfo hints;
    struct addrinfo *res = RT_NULL;
    struct sockaddr_in *sa;
    int err;

    if (host == RT_NULL || addr == RT_NULL)
    {
        return -1;
    }

    /* 先按点分十进制解析 */
    if (ip4addr_aton(host, addr))
    {
        return 0;
    }

    rt_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    err = lwip_getaddrinfo(host, RT_NULL, &hints, &res);
    if (err != 0 || res == RT_NULL)
    {
        return -1;
    }

    sa = (struct sockaddr_in *)res->ai_addr;
    addr->addr = sa->sin_addr.s_addr;
    lwip_freeaddrinfo(res);
    return 0;
}

/* 绝对截止时间（毫秒预算） */
static rt_tick_t net_deadline_ms(int timeout_ms)
{
    return rt_tick_get() + rt_tick_from_millisecond(timeout_ms);
}

static int net_remain_ms(rt_tick_t deadline)
{
    rt_tick_t now = rt_tick_get();

    if (now >= deadline)
    {
        return 0;
    }
    return (int)((deadline - now) * 1000UL / RT_TICK_PER_SECOND);
}

static void net_ms_to_timeval(int ms, struct timeval *tv)
{
    if (ms <= 0)
    {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
        return;
    }
    tv->tv_sec = ms / 1000;
    tv->tv_usec = (ms % 1000) * 1000;
}

/*
 * TCP 端口探测：connect 成功视为开放
 * @return 0 开放, 1 关闭/拒绝, -1 错误/超时
 */
static int net_tcp_probe_once(const char *host, int port, int timeout_ms)
{
    int s = -1;
    int ret;
    int result = -1;
    ip4_addr_t ip4;
    struct sockaddr_in to;
    u32_t nonblock = 1;
    u32_t block = 0;
    fd_set wset;
    struct timeval tv;
    int so_err = 0;
    socklen_t so_len = sizeof(so_err);
    rt_tick_t t0, elapsed_ms;

    if (port <= 0 || port > 65535)
    {
        rt_kprintf("[TCP] invalid port %d\n", port);
        return -1;
    }
    if (timeout_ms <= 0)
    {
        timeout_ms = NET_TCP_DEFAULT_TO_MS;
    }

    if (net_resolve_ipv4(host, &ip4) != 0)
    {
        rt_kprintf("[TCP] resolve failed: %s\n", host);
        return -1;
    }

    rt_kprintf("[TCP] probe %s (%s):%d timeout=%dms\n",
               host, ip4addr_ntoa(&ip4), port, timeout_ms);

    s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        rt_kprintf("[TCP] socket create failed\n");
        return -1;
    }

    /* 非阻塞 connect + select 实现超时 */
    if (lwip_ioctl(s, FIONBIO, &nonblock) != 0)
    {
        rt_kprintf("[TCP] set nonblock failed\n");
        lwip_close(s);
        return -1;
    }

    rt_memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_port = htons((u16_t)port);
    to.sin_addr.s_addr = ip4.addr;

    t0 = rt_tick_get();
    ret = lwip_connect(s, (struct sockaddr *)&to, sizeof(to));
    if (ret == 0)
    {
        result = 0; /* 立即连上 */
    }
    else if (errno != EINPROGRESS && errno != EALREADY && errno != EWOULDBLOCK)
    {
        rt_kprintf("[TCP] connect error: errno=%d\n", errno);
        result = 1;
    }
    else
    {
        /* 进行中：等可写 */
        FD_ZERO(&wset);
        FD_SET(s, &wset);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        ret = lwip_select(s + 1, RT_NULL, &wset, RT_NULL, &tv);
        if (ret == 0)
        {
            result = -1; /* 超时 */
        }
        else if (ret < 0)
        {
            rt_kprintf("[TCP] select error\n");
            result = -1;
        }
        else if (lwip_getsockopt(s, SOL_SOCKET, SO_ERROR, &so_err, &so_len) < 0)
        {
            rt_kprintf("[TCP] getsockopt SO_ERROR failed\n");
            result = -1;
        }
        else if (so_err == 0)
        {
            result = 0;
        }
        else
        {
            result = 1; /* 拒绝等 */
        }
    }

    elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
    if (result == 0)
    {
        rt_kprintf("[TCP] OPEN  %s:%d  (%ums)\n",
                   ip4addr_ntoa(&ip4), port, (unsigned)elapsed_ms);
    }
    else if (result == 1)
    {
        rt_kprintf("[TCP] CLOSED %s:%d  so_err=%d (%ums)\n",
                   ip4addr_ntoa(&ip4), port, so_err, (unsigned)elapsed_ms);
    }
    else
    {
        rt_kprintf("[TCP] CLOSED/FILTERED (timeout %ums)\n", (unsigned)elapsed_ms);
    }

    lwip_ioctl(s, FIONBIO, &block);
    lwip_close(s);
    return result;
}

static int cmd_tcp_probe(int argc, char **argv)
{
    const char *host = NET_TCP_DEFAULT_HOST;
    int port = NET_TCP_DEFAULT_PORT;
    int timeout_ms = NET_TCP_DEFAULT_TO_MS;

    if (argc >= 2)
    {
        host = argv[1];
    }
    if (argc >= 3)
    {
        port = atoi(argv[2]);
    }
    if (argc >= 4)
    {
        timeout_ms = atoi(argv[3]);
    }

    if (argc == 1)
    {
        rt_kprintf("Usage: tcp_probe [host] [port] [timeout_ms]\n");
        rt_kprintf("Default: %s %d %d\n",
                   NET_TCP_DEFAULT_HOST, NET_TCP_DEFAULT_PORT, NET_TCP_DEFAULT_TO_MS);
    }

    return net_tcp_probe_once(host, port, timeout_ms);
}

/*
 * UDP 端口探测：
 *  connect + send，再 recv：
 *    有数据 → OPEN
 *    ICMP 不可达 → CLOSED
 *    超时 → OPEN|FILTERED
 * @return 0 OPEN, 1 CLOSED, 2 OPEN|FILTERED, -1 错误
 */
static int net_udp_probe_once(const char *host, int port, int timeout_ms)
{
    int s = -1;
    int ret;
    int result = -1;
    ip4_addr_t ip4;
    struct sockaddr_in to;
    struct timeval tv;
    char payload[] = "PROBE";
    char rbuf[64];
    rt_tick_t t0, elapsed_ms;

    if (port <= 0 || port > 65535)
    {
        rt_kprintf("[UDP] invalid port %d\n", port);
        return -1;
    }
    if (timeout_ms <= 0)
    {
        timeout_ms = NET_UDP_DEFAULT_TO_MS;
    }

    if (net_resolve_ipv4(host, &ip4) != 0)
    {
        rt_kprintf("[UDP] resolve failed: %s\n", host);
        return -1;
    }

    rt_kprintf("[UDP] probe %s (%s):%d timeout=%dms\n",
               host, ip4addr_ntoa(&ip4), port, timeout_ms);

    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        rt_kprintf("[UDP] socket create failed\n");
        return -1;
    }

    rt_memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_port = htons((u16_t)port);
    to.sin_addr.s_addr = ip4.addr;

    /* connect 后 ICMP Port Unreachable 可关联到本 socket */
    if (lwip_connect(s, (struct sockaddr *)&to, sizeof(to)) < 0)
    {
        rt_kprintf("[UDP] connect failed errno=%d\n", errno);
        lwip_close(s);
        return -1;
    }

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    t0 = rt_tick_get();
    ret = lwip_send(s, payload, sizeof(payload) - 1, 0);
    if (ret < 0)
    {
        /* 部分栈在 send 时就能反映 ICMP 拒绝 */
        if (errno == ECONNREFUSED || errno == EHOSTUNREACH)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[UDP] CLOSED %s:%d  errno=%d (%ums)\n",
                       ip4addr_ntoa(&ip4), port, errno, (unsigned)elapsed_ms);
            lwip_close(s);
            return 1;
        }
        rt_kprintf("[UDP] send failed errno=%d\n", errno);
        lwip_close(s);
        return -1;
    }

    ret = lwip_recv(s, rbuf, sizeof(rbuf), 0);
    elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;

    if (ret > 0)
    {
        rt_kprintf("[UDP] OPEN  %s:%d  (got %d bytes, %ums)\n",
                   ip4addr_ntoa(&ip4), port, ret, (unsigned)elapsed_ms);
        result = 0;
    }
    else if (ret < 0 && (errno == ECONNREFUSED || errno == EHOSTUNREACH))
    {
        rt_kprintf("[UDP] CLOSED %s:%d  errno=%d (%ums)\n",
                   ip4addr_ntoa(&ip4), port, errno, (unsigned)elapsed_ms);
        result = 1;
    }
    else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT))
    {
        /* 无 ICMP、无 UDP 应答：开放无回包或中间过滤 */
        rt_kprintf("[UDP] OPEN|FILTERED %s:%d  (no reply %ums)\n",
                   ip4addr_ntoa(&ip4), port, (unsigned)elapsed_ms);
        result = 2;
    }
    else
    {
        rt_kprintf("[UDP] unknown result ret=%d errno=%d (%ums)\n",
                   ret, errno, (unsigned)elapsed_ms);
        result = -1;
    }

    lwip_close(s);
    return result;
}

static int cmd_udp_probe(int argc, char **argv)
{
    const char *host = NET_UDP_DEFAULT_HOST;
    int port = NET_UDP_DEFAULT_PORT;
    int timeout_ms = NET_UDP_DEFAULT_TO_MS;

    if (argc >= 2)
    {
        host = argv[1];
    }
    if (argc >= 3)
    {
        port = atoi(argv[2]);
    }
    if (argc >= 4)
    {
        timeout_ms = atoi(argv[3]);
    }

    if (argc == 1)
    {
        rt_kprintf("Usage: udp_probe [host] [port] [timeout_ms]\n");
        rt_kprintf("Default: %s %d %d\n",
                   NET_UDP_DEFAULT_HOST, NET_UDP_DEFAULT_PORT, NET_UDP_DEFAULT_TO_MS);
        rt_kprintf("Note: no reply => OPEN|FILTERED (UDP normal)\n");
    }

    return net_udp_probe_once(host, port, timeout_ms);
}

/*
 * WebSocket 服务探测：TCP 连接后发 HTTP Upgrade 握手，检查 101 + websocket
 * @return 0=WS_OK, 1=HTTP非WS, 2=TCP拒绝, -1=超时/错误
 */
static int net_ws_probe_once(const char *host, int port, const char *path, int timeout_ms)
{
    int s = -1;
    int ret;
    int result = -1;
    ip4_addr_t ip4;
    struct sockaddr_in to;
    u32_t nonblock = 1;
    u32_t block = 0;
    fd_set wset, rset;
    struct timeval tv;
    int so_err = 0;
    socklen_t so_len = sizeof(so_err);
    /* 静态缓冲：FinSH 栈仅 2048，避免与 HTTP 同类溢出卡死 */
    static char req[320];
    static char rbuf[NET_WS_RX_BUF];
    int req_len;
    int total = 0;
    rt_tick_t t0, elapsed_ms;
    const char *status_line;
    int status_code = 0;

    if (port <= 0 || port > 65535)
    {
        rt_kprintf("[WS] invalid port %d\n", port);
        return -1;
    }
    if (path == RT_NULL || path[0] == '\0')
    {
        path = NET_WS_DEFAULT_PATH;
    }
    if (path[0] != '/')
    {
        rt_kprintf("[WS] path must start with /\n");
        return -1;
    }
    if (timeout_ms <= 0)
    {
        timeout_ms = NET_WS_DEFAULT_TO_MS;
    }

    if (net_resolve_ipv4(host, &ip4) != 0)
    {
        rt_kprintf("[WS] resolve failed: %s\n", host);
        return -1;
    }

    rt_kprintf("[WS] probe ws://%s:%d%s timeout=%dms\n",
               ip4addr_ntoa(&ip4), port, path, timeout_ms);

    s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        rt_kprintf("[WS] socket create failed\n");
        return -1;
    }

    if (lwip_ioctl(s, FIONBIO, &nonblock) != 0)
    {
        rt_kprintf("[WS] set nonblock failed\n");
        lwip_close(s);
        return -1;
    }

    rt_memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_port = htons((u16_t)port);
    to.sin_addr.s_addr = ip4.addr;

    t0 = rt_tick_get();
    ret = lwip_connect(s, (struct sockaddr *)&to, sizeof(to));
    if (ret == 0)
    {
        /* connected */
    }
    else if (errno != EINPROGRESS && errno != EALREADY && errno != EWOULDBLOCK)
    {
        elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
        rt_kprintf("[WS] CLOSED %s:%d errno=%d (%ums)\n",
                   ip4addr_ntoa(&ip4), port, errno, (unsigned)elapsed_ms);
        lwip_close(s);
        return 2;
    }
    else
    {
        FD_ZERO(&wset);
        FD_SET(s, &wset);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        ret = lwip_select(s + 1, RT_NULL, &wset, RT_NULL, &tv);
        if (ret <= 0)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[WS] connect timeout (%ums)\n", (unsigned)elapsed_ms);
            lwip_close(s);
            return -1;
        }
        if (lwip_getsockopt(s, SOL_SOCKET, SO_ERROR, &so_err, &so_len) < 0 || so_err != 0)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[WS] CLOSED %s:%d so_err=%d (%ums)\n",
                       ip4addr_ntoa(&ip4), port, so_err, (unsigned)elapsed_ms);
            lwip_close(s);
            return 2;
        }
    }

    lwip_ioctl(s, FIONBIO, &block);

    req_len = rt_snprintf(req, sizeof(req),
                          "GET %s HTTP/1.1\r\n"
                          "Host: %s:%d\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Key: %s\r\n"
                          "Sec-WebSocket-Version: 13\r\n"
                          "\r\n",
                          path, ip4addr_ntoa(&ip4), port, NET_WS_PROBE_KEY);
    if (req_len <= 0 || req_len >= (int)sizeof(req))
    {
        rt_kprintf("[WS] request build failed\n");
        lwip_close(s);
        return -1;
    }

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (lwip_send(s, req, (size_t)req_len, 0) != req_len)
    {
        rt_kprintf("[WS] send handshake failed errno=%d\n", errno);
        lwip_close(s);
        return -1;
    }

    /* 收满响应头（\r\n\r\n）或超时 */
    while (total < (int)sizeof(rbuf) - 1)
    {
        ret = lwip_recv(s, rbuf + total, (size_t)((int)sizeof(rbuf) - 1 - total), 0);
        if (ret > 0)
        {
            total += ret;
            rbuf[total] = '\0';
            if (strstr(rbuf, "\r\n\r\n") != RT_NULL)
            {
                break;
            }
        }
        else if (ret == 0)
        {
            break;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT)
        {
            break;
        }
        else
        {
            rt_kprintf("[WS] recv failed errno=%d\n", errno);
            lwip_close(s);
            return -1;
        }
    }

    elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
    rbuf[total] = '\0';

    if (total == 0)
    {
        rt_kprintf("[WS] no response (%ums)\n", (unsigned)elapsed_ms);
        lwip_close(s);
        return -1;
    }

    status_line = rbuf;
    if (strncmp(status_line, "HTTP/1.", 7) == 0)
    {
        const char *sp = strchr(status_line + 7, ' ');
        if (sp)
        {
            status_code = atoi(sp + 1);
        }
    }

    rt_kprintf("[WS] response (%d bytes, %ums):\n", total, (unsigned)elapsed_ms);
    /* 只打印首行 + 关键头 */
    {
        char *line = rbuf;
        char *next;
        int lines = 0;
        while (line && *line && lines < 8)
        {
            next = strstr(line, "\r\n");
            if (next)
            {
                *next = '\0';
            }
            rt_kprintf("[WS] %s\n", line);
            lines++;
            if (!next || next[2] == '\0')
            {
                break;
            }
            line = next + 2;
        }
    }

    if (status_code == 101 &&
        net_strcasestr(rbuf, "Upgrade:") != RT_NULL &&
        net_strcasestr(rbuf, "websocket") != RT_NULL)
    {
        rt_kprintf("[WS] OK WebSocket service on %s:%d%s\n",
                   ip4addr_ntoa(&ip4), port, path);
        result = 0;
    }
    else if (status_code > 0)
    {
        rt_kprintf("[WS] HTTP %d (not WebSocket upgrade)\n", status_code);
        result = 1;
    }
    else
    {
        rt_kprintf("[WS] invalid HTTP response\n");
        result = -1;
    }

    lwip_close(s);
    return result;
}

static int cmd_ws_probe(int argc, char **argv)
{
    const char *host = NET_WS_DEFAULT_HOST;
    int port = NET_WS_DEFAULT_PORT;
    const char *path = NET_WS_DEFAULT_PATH;
    int timeout_ms = NET_WS_DEFAULT_TO_MS;

    if (argc >= 2)
    {
        host = argv[1];
    }
    if (argc >= 3)
    {
        port = atoi(argv[2]);
    }
    if (argc >= 4)
    {
        if (argv[3][0] == '/')
        {
            path = argv[3];
        }
        else
        {
            timeout_ms = atoi(argv[3]);
        }
    }
    if (argc >= 5)
    {
        timeout_ms = atoi(argv[4]);
    }

    if (argc == 1)
    {
        rt_kprintf("Usage: ws_probe [host] [port] [path] [timeout_ms]\n");
        rt_kprintf("Default: %s %d %s %d\n",
                   NET_WS_DEFAULT_HOST, NET_WS_DEFAULT_PORT,
                   NET_WS_DEFAULT_PATH, NET_WS_DEFAULT_TO_MS);
        rt_kprintf("OK: HTTP 101 + Upgrade: websocket\n");
    }

    return net_ws_probe_once(host, port, path, timeout_ms);
}

/* 从响应头解析 HTTP 状态码 */
static int net_http_parse_status(const char *buf)
{
    if (buf == RT_NULL || strncmp(buf, "HTTP/1.", 7) != 0)
    {
        return -1;
    }
    {
        const char *sp = strchr(buf + 7, ' ');
        if (sp == RT_NULL)
        {
            return -1;
        }
        return atoi(sp + 1);
    }
}

/* 从响应头解析 Content-Length，无则返回 -1 */
static int net_http_parse_content_length(const char *hdr)
{
    const char *p = net_strcasestr(hdr, "Content-Length:");
    if (p == RT_NULL)
    {
        return -1;
    }
    p += 15;
    while (*p == ' ' || *p == '\t')
    {
        p++;
    }
    return atoi(p);
}

/* 打印响应头各行（不修改缓冲区） */
static void net_http_print_headers(const char *hdr, int max_lines)
{
    const char *line = hdr;
    int lines = 0;

    if (hdr == RT_NULL || max_lines <= 0)
    {
        return;
    }

    while (line && *line && lines < max_lines)
    {
        const char *next = strstr(line, "\r\n");
        int linelen = next ? (int)(next - line) : (int)strlen(line);

        rt_kprintf("[HTTP] %.*s\n", linelen, line);
        lines++;
        if (!next || next[2] == '\0')
        {
            break;
        }
        line = next + 2;
    }
}

/* 打印 body 预览，非可打印字符显示为 '.' */
static void net_http_print_body(const char *body, int len, int max_print)
{
    int i;
    int n = len;

    if (body == RT_NULL || len <= 0 || max_print <= 0)
    {
        return;
    }
    if (n > max_print)
    {
        n = max_print;
    }

    rt_kprintf("[HTTP] body (%d bytes", len);
    if (len > max_print)
    {
        rt_kprintf(", show first %d", max_print);
    }
    rt_kprintf("):\n");

    for (i = 0; i < n; i++)
    {
        unsigned char c = (unsigned char)body[i];
        if (c == '\r' || c == '\n')
        {
            rt_kprintf("%c", (char)c);
        }
        else if (c >= 32 && c < 127)
        {
            rt_kprintf("%c", (char)c);
        }
        else
        {
            rt_kprintf(".");
        }
        if ((i + 1) % 64 == 0)
        {
            rt_kprintf("\n");
        }
    }
    if (n % 64 != 0)
    {
        rt_kprintf("\n");
    }
}

/*
 * HTTP GET：TCP 连接后发 GET 请求，解析状态码与响应体
 * @return 0=2xx, 1=其他HTTP状态, 2=TCP拒绝, -1=超时/错误
 */
static int net_http_get_once(const char *host, int port, const char *path,
                             int timeout_ms, int body_max)
{
    int s = -1;
    int ret;
    int result = -1;
    ip4_addr_t ip4;
    struct sockaddr_in to;
    u32_t nonblock = 1;
    fd_set fset;
    struct timeval tv;
    int so_err = 0;
    socklen_t so_len = sizeof(so_err);
    /* 静态缓冲：避免在 FinSH 小栈上分配导致溢出卡死 */
    static char req[384];
    static char rbuf[NET_HTTP_RX_BUF];
    int req_len;
    int sent = 0;
    int total = 0;
    int status_code = -1;
    int content_length = -1;
    int body_got = 0;
    char *hdr_end = RT_NULL;
    rt_tick_t t0, deadline, elapsed_ms;
    int remain_ms;

    if (port <= 0 || port > 65535)
    {
        rt_kprintf("[HTTP] invalid port %d\n", port);
        return -1;
    }
    if (path == RT_NULL || path[0] == '\0')
    {
        path = NET_HTTP_DEFAULT_PATH;
    }
    if (path[0] != '/')
    {
        rt_kprintf("[HTTP] path must start with /\n");
        return -1;
    }
    if (timeout_ms <= 0)
    {
        timeout_ms = NET_HTTP_DEFAULT_TO_MS;
    }
    if (body_max <= 0)
    {
        body_max = NET_HTTP_DEFAULT_BODY;
    }
    if (body_max > (int)sizeof(rbuf) - 1)
    {
        body_max = (int)sizeof(rbuf) - 1;
    }

    if (net_resolve_ipv4(host, &ip4) != 0)
    {
        rt_kprintf("[HTTP] resolve failed: %s\n", host);
        return -1;
    }

    rt_kprintf("[HTTP] GET http://%s:%d%s timeout=%dms body_max=%d\n",
               ip4addr_ntoa(&ip4), port, path, timeout_ms, body_max);

    s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        rt_kprintf("[HTTP] socket create failed\n");
        return -1;
    }

    if (lwip_ioctl(s, FIONBIO, &nonblock) != 0)
    {
        rt_kprintf("[HTTP] set nonblock failed\n");
        lwip_close(s);
        return -1;
    }

    rt_memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_port = htons((u16_t)port);
    to.sin_addr.s_addr = ip4.addr;

    t0 = rt_tick_get();
    deadline = net_deadline_ms(timeout_ms);

    ret = lwip_connect(s, (struct sockaddr *)&to, sizeof(to));
    if (ret != 0 &&
        errno != EINPROGRESS && errno != EALREADY && errno != EWOULDBLOCK)
    {
        elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
        rt_kprintf("[HTTP] CLOSED %s:%d errno=%d (%ums)\n",
                   ip4addr_ntoa(&ip4), port, errno, (unsigned)elapsed_ms);
        lwip_close(s);
        return 2;
    }
    if (ret != 0)
    {
        remain_ms = net_remain_ms(deadline);
        if (remain_ms <= 0)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[HTTP] connect timeout (%ums)\n", (unsigned)elapsed_ms);
            lwip_close(s);
            return -1;
        }

        FD_ZERO(&fset);
        FD_SET(s, &fset);
        net_ms_to_timeval(remain_ms, &tv);
        ret = lwip_select(s + 1, RT_NULL, &fset, RT_NULL, &tv);
        if (ret <= 0)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[HTTP] connect timeout (%ums)\n", (unsigned)elapsed_ms);
            lwip_close(s);
            return -1;
        }
        if (lwip_getsockopt(s, SOL_SOCKET, SO_ERROR, &so_err, &so_len) < 0 || so_err != 0)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[HTTP] CLOSED %s:%d so_err=%d (%ums)\n",
                       ip4addr_ntoa(&ip4), port, so_err, (unsigned)elapsed_ms);
            lwip_close(s);
            return 2;
        }
    }

    if (port == 80)
    {
        req_len = rt_snprintf(req, sizeof(req),
                              "GET %s HTTP/1.1\r\n"
                              "Host: %s\r\n"
                              "Connection: close\r\n"
                              "Accept: */*\r\n"
                              "\r\n",
                              path, host);
    }
    else
    {
        req_len = rt_snprintf(req, sizeof(req),
                              "GET %s HTTP/1.1\r\n"
                              "Host: %s:%d\r\n"
                              "Connection: close\r\n"
                              "Accept: */*\r\n"
                              "\r\n",
                              path, host, port);
    }
    if (req_len <= 0 || req_len >= (int)sizeof(req))
    {
        rt_kprintf("[HTTP] request build failed\n");
        lwip_close(s);
        return -1;
    }

    while (sent < req_len)
    {
        remain_ms = net_remain_ms(deadline);
        if (remain_ms <= 0)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[HTTP] send timeout (%ums)\n", (unsigned)elapsed_ms);
            lwip_close(s);
            return -1;
        }

        ret = lwip_send(s, req + sent, (size_t)(req_len - sent), 0);
        if (ret > 0)
        {
            sent += ret;
            continue;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            rt_kprintf("[HTTP] send failed errno=%d\n", errno);
            lwip_close(s);
            return -1;
        }

        FD_ZERO(&fset);
        FD_SET(s, &fset);
        net_ms_to_timeval(remain_ms, &tv);
        ret = lwip_select(s + 1, RT_NULL, &fset, RT_NULL, &tv);
        if (ret <= 0)
        {
            elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
            rt_kprintf("[HTTP] send timeout (%ums)\n", (unsigned)elapsed_ms);
            lwip_close(s);
            return -1;
        }
    }

    /* 全程非阻塞 + select，总耗时不超过 timeout_ms */
    while (total < (int)sizeof(rbuf) - 1)
    {
        int room = (int)sizeof(rbuf) - 1 - total;
        int want = room;

        if (hdr_end != RT_NULL && body_got >= body_max)
        {
            break;
        }
        if (hdr_end != RT_NULL && content_length >= 0)
        {
            int hdr_body = total - (int)(hdr_end - rbuf);
            int body_remain = content_length - hdr_body;
            if (body_remain <= 0)
            {
                break;
            }
            if (want > body_remain)
            {
                want = body_remain;
            }
        }

        remain_ms = net_remain_ms(deadline);
        if (remain_ms <= 0)
        {
            break;
        }

        FD_ZERO(&fset);
        FD_SET(s, &fset);
        net_ms_to_timeval(remain_ms, &tv);
        ret = lwip_select(s + 1, &fset, RT_NULL, RT_NULL, &tv);
        if (ret <= 0)
        {
            break;
        }

        ret = lwip_recv(s, rbuf + total, (size_t)want, 0);
        if (ret > 0)
        {
            total += ret;
            rbuf[total] = '\0';
            if (hdr_end == RT_NULL)
            {
                hdr_end = strstr(rbuf, "\r\n\r\n");
                if (hdr_end != RT_NULL)
                {
                    hdr_end += 4;
                    status_code = net_http_parse_status(rbuf);
                    content_length = net_http_parse_content_length(rbuf);
                    body_got = total - (int)(hdr_end - rbuf);
                }
            }
            else
            {
                body_got = total - (int)(hdr_end - rbuf);
            }
        }
        else if (ret == 0)
        {
            break;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            continue;
        }
        else
        {
            rt_kprintf("[HTTP] recv failed errno=%d\n", errno);
            lwip_close(s);
            return -1;
        }
    }

    elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;
    rbuf[total] = '\0';

    if (total == 0)
    {
        rt_kprintf("[HTTP] no response (%ums)\n", (unsigned)elapsed_ms);
        lwip_close(s);
        return -1;
    }

    if (hdr_end == RT_NULL)
    {
        hdr_end = strstr(rbuf, "\r\n\r\n");
        if (hdr_end != RT_NULL)
        {
            hdr_end += 4;
            status_code = net_http_parse_status(rbuf);
            content_length = net_http_parse_content_length(rbuf);
            body_got = total - (int)(hdr_end - rbuf);
        }
    }

    rt_kprintf("[HTTP] response %d bytes, %ums\n", total, (unsigned)elapsed_ms);
    if (status_code > 0)
    {
        rt_kprintf("[HTTP] status %d\n", status_code);
    }
    else
    {
        rt_kprintf("[HTTP] invalid status line\n");
        lwip_close(s);
        return -1;
    }

    net_http_print_headers(rbuf, 16);

    if (hdr_end != RT_NULL && body_got > 0)
    {
        net_http_print_body(hdr_end, body_got, body_max);
    }
    else if (content_length == 0)
    {
        rt_kprintf("[HTTP] body empty\n");
    }
    else
    {
        rt_kprintf("[HTTP] no body received\n");
    }

    if (status_code >= 200 && status_code < 300)
    {
        rt_kprintf("[HTTP] OK %d\n", status_code);
        result = 0;
    }
    else
    {
        rt_kprintf("[HTTP] status %d\n", status_code);
        result = 1;
    }

    lwip_close(s);
    return result;
}

static int cmd_http_get(int argc, char **argv)
{
    const char *host = NET_HTTP_DEFAULT_HOST;
    int port = NET_HTTP_DEFAULT_PORT;
    const char *path = NET_HTTP_DEFAULT_PATH;
    int timeout_ms = NET_HTTP_DEFAULT_TO_MS;
    int body_max = NET_HTTP_DEFAULT_BODY;

    if (argc >= 2)
    {
        host = argv[1];
    }
    if (argc >= 3)
    {
        port = atoi(argv[2]);
    }
    if (argc >= 4)
    {
        if (argv[3][0] == '/')
        {
            path = argv[3];
            if (argc >= 5)
            {
                timeout_ms = atoi(argv[4]);
            }
            if (argc >= 6)
            {
                body_max = atoi(argv[5]);
            }
        }
        else
        {
            timeout_ms = atoi(argv[3]);
            if (argc >= 5)
            {
                body_max = atoi(argv[4]);
            }
        }
    }

    if (argc == 1)
    {
        rt_kprintf("Usage: http_get [host] [port] [path] [timeout_ms] [body_max]\n");
        rt_kprintf("Default: %s %d %s %d %d\n",
                   NET_HTTP_DEFAULT_HOST, NET_HTTP_DEFAULT_PORT,
                   NET_HTTP_DEFAULT_PATH, NET_HTTP_DEFAULT_TO_MS,
                   NET_HTTP_DEFAULT_BODY);
        rt_kprintf("OK: HTTP 2xx, prints headers and body preview\n");
    }

    return net_http_get_once(host, port, path, timeout_ms, body_max);
}

static int cmd_dns_lookup(int argc, char **argv)
{
    const char *name = NET_DNS_DEFAULT_HOST;
    struct addrinfo hints;
    struct addrinfo *res = RT_NULL;
    struct addrinfo *rp;
    int err;
    int count = 0;
    rt_tick_t t0, elapsed_ms;

    if (argc >= 2)
    {
        name = argv[1];
    }
    else
    {
        rt_kprintf("Usage: dns_lookup [hostname]\n");
        rt_kprintf("Default: %s\n", NET_DNS_DEFAULT_HOST);
    }

    rt_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    rt_kprintf("[DNS] resolve %s ...\n", name);
    t0 = rt_tick_get();
    err = lwip_getaddrinfo(name, RT_NULL, &hints, &res);
    elapsed_ms = (rt_tick_get() - t0) * 1000UL / RT_TICK_PER_SECOND;

    if (err != 0 || res == RT_NULL)
    {
        rt_kprintf("[DNS] FAIL %s (err=%d, %ums)\n", name, err, (unsigned)elapsed_ms);
        return -1;
    }

    for (rp = res; rp != RT_NULL; rp = rp->ai_next)
    {
        struct sockaddr_in *sa = (struct sockaddr_in *)rp->ai_addr;
        if (sa == RT_NULL)
        {
            continue;
        }
        count++;
        rt_kprintf("[DNS] %s -> %s\n", name, inet_ntoa(sa->sin_addr));
    }

    lwip_freeaddrinfo(res);
    rt_kprintf("[DNS] OK, %d address(es), %ums\n", count, (unsigned)elapsed_ms);
    return (count > 0) ? 0 : -1;
}

MSH_CMD_EXPORT_ALIAS(cmd_tcp_probe, tcp_probe, TCP port probe host:port);
MSH_CMD_EXPORT_ALIAS(cmd_udp_probe, udp_probe, UDP port probe host:port);
MSH_CMD_EXPORT_ALIAS(cmd_ws_probe, ws_probe, WebSocket handshake probe);
MSH_CMD_EXPORT_ALIAS(cmd_http_get, http_get, HTTP GET request);
MSH_CMD_EXPORT_ALIAS(cmd_dns_lookup, dns_lookup, DNS resolve hostname to IP);

#endif /* RT_USING_FINSH */
#endif /* RT_USING_LWIP */
