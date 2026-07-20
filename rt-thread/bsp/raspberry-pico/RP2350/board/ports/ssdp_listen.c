/*
 * SSDP 组播监听（239.255.255.250:1900）
 *
 * USN 过滤规则：
 *   - 不输入关键词：不过滤，显示所有带 USN 的报文
 *   - 输入关键词：仅当 USN 包含该字符串时显示（不区分大小写）
 *
 * 用法:
 *   ssdp_start [关键词] [秒]   开始监听；秒=0 一直听直到 stop（默认 30s）
 *   ssdp_filter [关键词]       运行中改过滤（无参=取消过滤）
 *   ssdp_stop                  停止监听
 *   ssdp_search [ST]           发一次 M-SEARCH（默认 ssdp:all）
 *
 * 例: ssdp_start innercomdevice 30
 */
#include <rtthread.h>

#if defined(RT_USING_LWIP) && defined(RT_USING_FINSH)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <lwip/sockets.h>
#include <lwip/inet.h>

#include <finsh.h>

#define SSDP_MCAST_ADDR     "239.255.255.250"
#define SSDP_PORT           1900
#define SSDP_RX_BUF         1600
#define SSDP_STACK          2048
#define SSDP_PRIO           (RT_THREAD_PRIORITY_MAX / 2 + 3)
#define SSDP_FILTER_MAX     96
#define SSDP_DEFAULT_SEC    30

static volatile rt_bool_t ssdp_running = RT_FALSE;
static volatile rt_bool_t ssdp_abort = RT_FALSE;
static rt_thread_t ssdp_tid = RT_NULL;
static int ssdp_sock = -1;
static char ssdp_filter[SSDP_FILTER_MAX];
static rt_mutex_t ssdp_lock = RT_NULL;

static void ssdp_lock_init(void)
{
    if (ssdp_lock == RT_NULL)
    {
        ssdp_lock = rt_mutex_create("ssdp", RT_IPC_FLAG_PRIO);
    }
}

static void ssdp_set_filter(const char *kw)
{
    ssdp_lock_init();
    rt_mutex_take(ssdp_lock, RT_WAITING_FOREVER);
    if (kw == RT_NULL || kw[0] == '\0')
    {
        ssdp_filter[0] = '\0';
    }
    else
    {
        rt_strncpy(ssdp_filter, kw, SSDP_FILTER_MAX - 1);
        ssdp_filter[SSDP_FILTER_MAX - 1] = '\0';
    }
    rt_mutex_release(ssdp_lock);
}

static void ssdp_get_filter(char *out, rt_size_t out_sz)
{
    ssdp_lock_init();
    rt_mutex_take(ssdp_lock, RT_WAITING_FOREVER);
    rt_strncpy(out, ssdp_filter, out_sz - 1);
    out[out_sz - 1] = '\0';
    rt_mutex_release(ssdp_lock);
}

/* 不区分大小写查找子串 */
static const char *ssdp_strcasestr(const char *hay, const char *needle)
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

/* 从 SSDP 报文中取 header 值（如 USN / LOCATION），写入 out */
static rt_bool_t ssdp_get_header(const char *msg, const char *key,
                                 char *out, rt_size_t out_sz)
{
    const char *p;
    const char *line;
    size_t key_len;
    size_t i;

    if (msg == RT_NULL || key == RT_NULL || out == RT_NULL || out_sz == 0)
    {
        return RT_FALSE;
    }

    out[0] = '\0';
    key_len = strlen(key);
    p = msg;

    while (*p)
    {
        line = p;
        /* 找行尾 */
        while (*p && *p != '\r' && *p != '\n')
        {
            p++;
        }

        /* 空行结束头部 */
        if (p == line)
        {
            break;
        }

        if ((rt_size_t)(p - line) > key_len &&
            ssdp_strcasestr(line, key) == line &&
            line[key_len] == ':')
        {
            const char *v = line + key_len + 1;
            while (*v == ' ' || *v == '\t')
            {
                v++;
            }
            i = 0;
            while (v < p && i + 1 < out_sz)
            {
                out[i++] = *v++;
            }
            out[i] = '\0';
            /* 去掉尾部空白 */
            while (i > 0 && (out[i - 1] == ' ' || out[i - 1] == '\t'))
            {
                out[--i] = '\0';
            }
            return (i > 0) ? RT_TRUE : RT_FALSE;
        }

        if (*p == '\r')
        {
            p++;
        }
        if (*p == '\n')
        {
            p++;
        }
    }

    return RT_FALSE;
}

static int ssdp_open_socket(void)
{
    int s;
    int on = 1;
    struct sockaddr_in bind_addr;
    struct ip_mreq mreq;
    struct timeval tv;

    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        rt_kprintf("[SSDP] socket failed\n");
        return -1;
    }

    if (lwip_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        rt_kprintf("[SSDP] SO_REUSEADDR warn\n");
    }

    rt_memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_len = sizeof(bind_addr);
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(SSDP_PORT);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (lwip_bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
    {
        rt_kprintf("[SSDP] bind :%d failed errno=%d\n", SSDP_PORT, errno);
        lwip_close(s);
        return -1;
    }

    rt_memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(SSDP_MCAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (lwip_setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        rt_kprintf("[SSDP] IGMP join %s failed errno=%d\n", SSDP_MCAST_ADDR, errno);
        lwip_close(s);
        return -1;
    }

    /* recv 超时，便于周期检查 abort */
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return s;
}

static void ssdp_close_socket(void)
{
    if (ssdp_sock >= 0)
    {
        struct ip_mreq mreq;

        rt_memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(SSDP_MCAST_ADDR);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        lwip_setsockopt(ssdp_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        lwip_close(ssdp_sock);
        ssdp_sock = -1;
    }
}

static void ssdp_handle_packet(const char *buf, int len,
                               const struct sockaddr_in *from)
{
    char usn[256];
    char loc[192];
    char nt[128];
    char filter[SSDP_FILTER_MAX];
    char first_line[80];
    int i;

    if (len <= 0 || buf == RT_NULL)
    {
        return;
    }

    /* 首行（NOTIFY / HTTP/1.1 200 / M-SEARCH） */
    i = 0;
    while (i < len && i < (int)sizeof(first_line) - 1 &&
           buf[i] != '\r' && buf[i] != '\n')
    {
        first_line[i] = buf[i];
        i++;
    }
    first_line[i] = '\0';

    if (!ssdp_get_header(buf, "USN", usn, sizeof(usn)))
    {
        /* 无 USN 的报文（如纯 M-SEARCH）默认不显示，除非未设过滤且想看搜索 */
        return;
    }

    ssdp_get_filter(filter, sizeof(filter));
    if (filter[0] != '\0' && ssdp_strcasestr(usn, filter) == RT_NULL)
    {
        return; /* USN 不含关键词 */
    }

    ssdp_get_header(buf, "LOCATION", loc, sizeof(loc));
    if (!ssdp_get_header(buf, "NT", nt, sizeof(nt)))
    {
        ssdp_get_header(buf, "ST", nt, sizeof(nt));
    }

    rt_kprintf("[SSDP] ---- from %s ----\n", inet_ntoa(from->sin_addr));
    rt_kprintf("[SSDP] %s\n", first_line);
    rt_kprintf("[SSDP] USN: %s\n", usn);
    if (nt[0])
    {
        rt_kprintf("[SSDP] NT/ST: %s\n", nt);
    }
    if (loc[0])
    {
        rt_kprintf("[SSDP] LOCATION: %s\n", loc);
    }
}

static void ssdp_thread_entry(void *parameter)
{
    int seconds = (int)(rt_ubase_t)parameter;
    char *buf;
    rt_tick_t start_tick;
    rt_tick_t duration_ticks = 0;
    int count = 0;

    buf = (char *)rt_malloc(SSDP_RX_BUF);
    if (buf == RT_NULL)
    {
        rt_kprintf("[SSDP] malloc rx buf failed\n");
        ssdp_running = RT_FALSE;
        ssdp_tid = RT_NULL;
        return;
    }

    ssdp_sock = ssdp_open_socket();
    if (ssdp_sock < 0)
    {
        rt_free(buf);
        ssdp_running = RT_FALSE;
        ssdp_tid = RT_NULL;
        return;
    }

    start_tick = rt_tick_get();
    if (seconds > 0)
    {
        duration_ticks = rt_tick_from_millisecond((rt_int32_t)seconds * 1000);
    }

    {
        char filter[SSDP_FILTER_MAX];
        ssdp_get_filter(filter, sizeof(filter));
        rt_kprintf("[SSDP] listening %s:%d", SSDP_MCAST_ADDR, SSDP_PORT);
        if (filter[0])
        {
            rt_kprintf(", USN filter=\"%s\"", filter);
        }
        else
        {
            rt_kprintf(", USN filter=OFF (show all)");
        }
        if (seconds > 0)
        {
            rt_kprintf(", duration=%ds", seconds);
        }
        else
        {
            rt_kprintf(", until ssdp_stop");
        }
        rt_kprintf("\n");
    }

    while (!ssdp_abort)
    {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        int n;

        if (seconds > 0 && (rt_tick_get() - start_tick) >= duration_ticks)
        {
            break;
        }

        n = lwip_recvfrom(ssdp_sock, buf, SSDP_RX_BUF - 1, 0,
                          (struct sockaddr *)&from, &fromlen);
        if (n > 0)
        {
            buf[n] = '\0';
            count++;
            ssdp_handle_packet(buf, n, &from);
        }
        /* 超时/错误：继续，靠 abort / deadline 退出 */
    }

    ssdp_close_socket();
    rt_free(buf);
    ssdp_running = RT_FALSE;
    ssdp_tid = RT_NULL;
    rt_kprintf("[SSDP] stopped, rx packets=%d\n", count);
}

static int ssdp_send_msearch(const char *st)
{
    int s;
    struct sockaddr_in dest;
    char req[256];
    int len;
    int ret;

    if (st == RT_NULL || st[0] == '\0')
    {
        st = "ssdp:all";
    }

    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        rt_kprintf("[SSDP] msearch socket failed\n");
        return -1;
    }

    len = rt_snprintf(req, sizeof(req),
                      "M-SEARCH * HTTP/1.1\r\n"
                      "HOST: %s:%d\r\n"
                      "MAN: \"ssdp:discover\"\r\n"
                      "MX: 2\r\n"
                      "ST: %s\r\n"
                      "\r\n",
                      SSDP_MCAST_ADDR, SSDP_PORT, st);
    if (len <= 0 || len >= (int)sizeof(req))
    {
        lwip_close(s);
        return -1;
    }

    rt_memset(&dest, 0, sizeof(dest));
    dest.sin_len = sizeof(dest);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(SSDP_PORT);
    dest.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);

    ret = lwip_sendto(s, req, len, 0, (struct sockaddr *)&dest, sizeof(dest));
    lwip_close(s);

    if (ret < 0)
    {
        rt_kprintf("[SSDP] msearch send failed errno=%d\n", errno);
        return -1;
    }

    rt_kprintf("[SSDP] M-SEARCH sent, ST=%s (%d bytes)\n", st, ret);
    return 0;
}

static int cmd_ssdp_start(int argc, char **argv)
{
    const char *kw = RT_NULL;
    int seconds = SSDP_DEFAULT_SEC;

    if (ssdp_running)
    {
        rt_kprintf("[SSDP] already running, use ssdp_stop first\n");
        return -1;
    }

    if (argc >= 2 && argv[1][0] != '\0')
    {
        /* 若第一参是纯数字，当作秒数；否则当关键词 */
        char *end = RT_NULL;
        long v = strtol(argv[1], &end, 10);
        if (end != argv[1] && *end == '\0')
        {
            seconds = (int)v;
        }
        else
        {
            kw = argv[1];
        }
    }
    if (argc >= 3)
    {
        seconds = atoi(argv[2]);
    }

    ssdp_set_filter(kw);
    ssdp_abort = RT_FALSE;
    ssdp_running = RT_TRUE;
    ssdp_tid = rt_thread_create("ssdp",
                                ssdp_thread_entry,
                                (void *)(rt_ubase_t)seconds,
                                SSDP_STACK,
                                SSDP_PRIO,
                                10);
    if (ssdp_tid == RT_NULL)
    {
        ssdp_running = RT_FALSE;
        rt_kprintf("[SSDP] create thread failed\n");
        return -1;
    }

    rt_thread_startup(ssdp_tid);

    if (argc == 1)
    {
        rt_kprintf("Usage: ssdp_start [keyword] [seconds]\n");
        rt_kprintf("  keyword  USN 子串过滤，不区分大小写；省略则不过滤\n");
        rt_kprintf("  seconds  0=直到 ssdp_stop，默认 %d\n", SSDP_DEFAULT_SEC);
    }

    return 0;
}

static int cmd_ssdp_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!ssdp_running)
    {
        rt_kprintf("[SSDP] not running\n");
        return 0;
    }

    ssdp_abort = RT_TRUE;
    rt_kprintf("[SSDP] stop requested...\n");
    return 0;
}

static int cmd_ssdp_filter(int argc, char **argv)
{
    char cur[SSDP_FILTER_MAX];

    if (argc >= 2)
    {
        ssdp_set_filter(argv[1]);
    }
    else
    {
        ssdp_set_filter(RT_NULL);
    }

    ssdp_get_filter(cur, sizeof(cur));
    if (cur[0])
    {
        rt_kprintf("[SSDP] USN filter=\"%s\"\n", cur);
    }
    else
    {
        rt_kprintf("[SSDP] USN filter=OFF (show all)\n");
    }
    return 0;
}

static int cmd_ssdp_search(int argc, char **argv)
{
    const char *st = "ssdp:all";

    if (argc >= 2 && argv[1][0] != '\0')
    {
        st = argv[1];
    }

    if (!ssdp_running)
    {
        rt_kprintf("[SSDP] tip: run ssdp_start first to receive replies\n");
    }

    return ssdp_send_msearch(st);
}

MSH_CMD_EXPORT_ALIAS(cmd_ssdp_start, ssdp_start, start SSDP listen with USN filter);
MSH_CMD_EXPORT_ALIAS(cmd_ssdp_stop, ssdp_stop, stop SSDP listen);
MSH_CMD_EXPORT_ALIAS(cmd_ssdp_filter, ssdp_filter, set/clear SSDP USN keyword filter);
MSH_CMD_EXPORT_ALIAS(cmd_ssdp_search, ssdp_search, send SSDP M-SEARCH);

#endif /* RT_USING_LWIP && RT_USING_FINSH */
