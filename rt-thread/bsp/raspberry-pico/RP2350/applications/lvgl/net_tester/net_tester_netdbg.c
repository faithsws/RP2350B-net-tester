/*
 * 调试 → 网络协议二级菜单与功能页
 * 覆盖 FinSH: IP设置/网卡信息/Ping/TCP/UDP/DNS/HTTP/ARP/链路闪灯
 */
#include "net_tester_netdbg.h"

#include "hzk12_font.h"
#include "net_tester_ip_input.h"
#include "net_tester_ascii_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdev_ipaddr.h>
#include <netdev.h>

#include "net_tester_netops.h"

#ifdef BSP_USING_CH390
#include <ch390.h>
#endif

LV_FONT_DECLARE(lv_font_montserrat_14)
LV_FONT_DECLARE(lv_font_montserrat_20)

#define UI_BG        0x0E1218
#define UI_PANEL     0x121820
#define UI_PANEL_ON  0x161E28
#define UI_BORDER    0x243040
#define UI_TITLE     0x6EC8D8
#define UI_FOOTER    0x4A5A6A
#define CLR_DEBUG    0x44BB44
#define CLR_WARN     0xC88838
#define CLR_ERR      0xCC5555

#define SCR_W   240
#define SCR_H   240
#define HDR_H   32

#define UI_CN_FONT   hzk24_font_get()
#define UI_SM_FONT   hzk12_font_get()
#define UI_ICON_FONT (&lv_font_montserrat_20)

#ifndef BSP_CH390_NETIF_NAME
#define BSP_CH390_NETIF_NAME "e0"
#endif

#define NETDBG_RESULT_MAX  1024
#define NETDBG_WORKER_STACK 4096
#define NETDBG_PING_SIZE    32
#define NETDBG_PING_TO_MS   2000
#define NETDBG_PING_GAP_MS  1000
#define NETDBG_DONE_DIALOG  0x01  /* 完成后弹窗显示 */

typedef enum {
    NETDBG_IP_CFG = 0,
    NETDBG_INFO,
    NETDBG_PING,
    NETDBG_TCP,
    NETDBG_UDP,
    NETDBG_DNS,
    NETDBG_HTTP,
    NETDBG_ARP,
    NETDBG_BLINK,
    NETDBG_COUNT
} netdbg_id_t;

typedef enum {
    NETDBG_PRE_OK = 0,
    NETDBG_PRE_NO_DEV,
    NETDBG_PRE_NO_LINK,
    NETDBG_PRE_NO_IP
} netdbg_pre_t;

typedef struct {
    const char * name;
    const char * icon;
} netdbg_menu_desc_t;

typedef struct {
    lv_obj_t * screen;
    lv_obj_t * result_box;
    lv_obj_t * result_lbl;
    lv_obj_t * start_btn;
    lv_obj_t * start_lbl;
    lv_obj_t * goto_ip_btn;
    net_tester_ip_input_t ip;
    net_tester_ascii_input_t ascii;
    /* IP 设置页额外 */
    net_tester_ip_input_t ip_gw;
    net_tester_ip_input_t ip_mask;
    lv_obj_t * dhcp_btn;
    lv_obj_t * static_btn;
    bool has_ip;
    bool has_ascii;
    bool result_browsing; /* 结果区浏览模式：编码器滚动 */
} netdbg_page_t;

static const netdbg_menu_desc_t s_menu_desc[NETDBG_COUNT] = {
    { "IP设置",   LV_SYMBOL_SETTINGS },
    { "网卡信息", LV_SYMBOL_LIST },
    { "Ping",     LV_SYMBOL_REFRESH },
    { "TCP探测",  LV_SYMBOL_UPLOAD },
    { "UDP探测",  LV_SYMBOL_DOWNLOAD },
    { "DNS查询",  LV_SYMBOL_DIRECTORY },
    { "HTTP获取", LV_SYMBOL_HOME },
    { "ARP查询",  LV_SYMBOL_LOOP },
    { "链路闪灯", LV_SYMBOL_CHARGE },
};

static lv_obj_t * s_menu_screen;
static lv_obj_t * s_menu_btns[NETDBG_COUNT];
static netdbg_page_t s_pages[NETDBG_COUNT];
static netdbg_id_t s_cur_id = NETDBG_COUNT;
static lv_group_t * s_bound_group;
static net_tester_netdbg_leave_cb_t s_leave_cb;

static volatile int s_busy;
static volatile int s_done;
static volatile int s_progress;   /* 过程刷新（Ping 等） */
static volatile int s_done_flags; /* NETDBG_DONE_DIALOG 等 */
static char s_result[NETDBG_RESULT_MAX];
static netdbg_id_t s_work_id;
static char s_work_host[64];
static char s_work_extra[32];
static int s_work_port;
static int s_work_times;
static lv_timer_t * s_poll_timer;

static lv_obj_t * s_dialog;       /* 结果弹窗 */
static lv_obj_t * s_dialog_ok;

static volatile rt_bool_t s_blink_running;
static volatile rt_bool_t s_blink_abort;
static rt_thread_t s_blink_tid;

/* ---------- 网络前置检查 ---------- */

static struct netdev * netdbg_get_netdev(void)
{
    struct netdev * nd = netdev_get_by_name(BSP_CH390_NETIF_NAME);
    if(nd == RT_NULL) {
        nd = netdev_default;
    }
    return nd;
}

static netdbg_pre_t netdbg_check_pre(bool need_ip)
{
    struct netdev * nd = netdbg_get_netdev();
    if(nd == RT_NULL) {
        return NETDBG_PRE_NO_DEV;
    }
    if(!netdev_is_up(nd) || !netdev_is_link_up(nd)) {
        return NETDBG_PRE_NO_LINK;
    }
    if(need_ip && ip_addr_isany(&nd->ip_addr)) {
        return NETDBG_PRE_NO_IP;
    }
    return NETDBG_PRE_OK;
}

static void netdbg_pre_msg(netdbg_pre_t pre, char * out, size_t out_sz)
{
    switch(pre) {
    case NETDBG_PRE_NO_DEV:
        rt_snprintf(out, out_sz, "未找到网卡\n请检查驱动");
        break;
    case NETDBG_PRE_NO_LINK:
        rt_snprintf(out, out_sz, "无链路(LINK)\n请检查网线");
        break;
    case NETDBG_PRE_NO_IP:
        rt_snprintf(out, out_sz, "无IP地址\n请进入IP设置\n配置DHCP或静态");
        break;
    default:
        out[0] = '\0';
        break;
    }
}

/* ---------- 样式 ---------- */

static void style_screen_bg(lv_obj_t * scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t * create_header(lv_obj_t * parent, const char * text)
{
    lv_obj_t * hdr = lv_obj_create(parent);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, SCR_W, HDR_H);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * lbl = lv_label_create(hdr);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, UI_CN_FONT, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(UI_TITLE), 0);
    lv_obj_center(lbl);
    return hdr;
}

static void style_action_btn(lv_obj_t * btn, bool focused)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(focused ? UI_PANEL_ON : UI_PANEL), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(focused ? UI_TITLE : UI_BORDER), 0);
    lv_obj_set_style_border_width(btn, focused ? 2 : 1, 0);
    lv_obj_set_style_radius(btn, 3, 0);
}

static void btn_focus_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED) {
        style_action_btn(btn, true);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        style_action_btn(btn, false);
    }
}

static void bind_page_group(netdbg_id_t id);
static bool indev_is_keypad(void);

static void set_result_text(netdbg_page_t * page, const char * text, uint32_t color)
{
    if(!page || !page->result_lbl) {
        return;
    }
    lv_label_set_text(page->result_lbl, text ? text : "");
    lv_obj_set_style_text_color(page->result_lbl, lv_color_hex(color), 0);
    if(page->result_box) {
        lv_obj_scroll_to_y(page->result_box, 0, LV_ANIM_OFF);
    }
}

static void dialog_close(void)
{
    if(s_dialog) {
        lv_obj_del(s_dialog);
        s_dialog = NULL;
        s_dialog_ok = NULL;
    }
    /* 关闭后把焦点交回当前页 */
    if(s_cur_id < NETDBG_COUNT && s_bound_group) {
        bind_page_group(s_cur_id);
    }
}

static void dialog_ok_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        if(indev_is_keypad()) {
            return;
        }
        dialog_close();
    }
    else if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ENTER || key == LV_KEY_ESC) {
            dialog_close();
            lv_event_stop_processing(e);
        }
    }
    else if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        btn_focus_cb(e);
    }
}

static void show_result_dialog(const char * title, const char * msg)
{
    lv_obj_t * panel;
    lv_obj_t * title_lbl;
    lv_obj_t * body;
    lv_obj_t * msg_lbl;
    lv_obj_t * ok_lbl;

    dialog_close();

    s_dialog = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_dialog);
    lv_obj_set_size(s_dialog, SCR_W, SCR_H);
    lv_obj_set_style_bg_color(s_dialog, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_dialog, LV_OPA_70, 0);
    lv_obj_clear_flag(s_dialog, LV_OBJ_FLAG_SCROLLABLE);

    panel = lv_obj_create(s_dialog);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, SCR_W - 24, SCR_H - 48);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(CLR_DEBUG), 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 4, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    title_lbl = lv_label_create(panel);
    lv_label_set_text(title_lbl, title ? title : "结果");
    lv_obj_set_style_text_font(title_lbl, UI_CN_FONT, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(UI_TITLE), 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 0);

    body = lv_obj_create(panel);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, SCR_W - 48, SCR_H - 48 - 80);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(body, lv_color_hex(UI_BG), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(body, 4, 0);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);

    msg_lbl = lv_label_create(body);
    lv_label_set_text(msg_lbl, msg ? msg : "");
    lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(msg_lbl, SCR_W - 60);
    lv_obj_set_style_text_font(msg_lbl, UI_SM_FONT, 0);
    lv_obj_set_style_text_color(msg_lbl, lv_color_hex(CLR_DEBUG), 0);

    s_dialog_ok = lv_btn_create(panel);
    lv_obj_remove_style_all(s_dialog_ok);
    lv_obj_set_size(s_dialog_ok, 96, 32);
    lv_obj_align(s_dialog_ok, LV_ALIGN_BOTTOM_MID, 0, -4);
    style_action_btn(s_dialog_ok, true);
    lv_obj_add_event_cb(s_dialog_ok, dialog_ok_cb, LV_EVENT_ALL, NULL);
    ok_lbl = lv_label_create(s_dialog_ok);
    lv_label_set_text(ok_lbl, "确定");
    lv_obj_set_style_text_font(ok_lbl, UI_CN_FONT, 0);
    lv_obj_set_style_text_color(ok_lbl, lv_color_hex(CLR_DEBUG), 0);
    lv_obj_center(ok_lbl);

    if(s_bound_group) {
        lv_group_remove_all_objs(s_bound_group);
        lv_group_add_obj(s_bound_group, s_dialog_ok);
        lv_group_focus_obj(s_dialog_ok);
    }
}

static void set_busy_ui(netdbg_page_t * page, bool busy)
{
    if(!page) {
        return;
    }
    if(page->start_lbl) {
        lv_label_set_text(page->start_lbl, busy ? "测试中" : "开始");
    }
    if(page->dhcp_btn) {
        if(busy) {
            lv_obj_add_state(page->dhcp_btn, LV_STATE_DISABLED);
        }
        else {
            lv_obj_clear_state(page->dhcp_btn, LV_STATE_DISABLED);
        }
    }
    if(page->static_btn) {
        if(busy) {
            lv_obj_add_state(page->static_btn, LV_STATE_DISABLED);
        }
        else {
            lv_obj_clear_state(page->static_btn, LV_STATE_DISABLED);
        }
    }
}

/* ---------- 后台任务结果 ---------- */

static void poll_timer_cb(lv_timer_t * t)
{
    netdbg_page_t * page;
    LV_UNUSED(t);

    /* 过程刷新（如 Ping 每一跳） */
    if(s_progress) {
        s_progress = 0;
        if(s_work_id < NETDBG_COUNT) {
            page = &s_pages[s_work_id];
            set_result_text(page, s_result, CLR_DEBUG);
            if(page->result_box) {
                lv_obj_scroll_to_view(page->result_lbl, LV_ANIM_OFF);
                lv_obj_scroll_to_y(page->result_box,
                                   lv_obj_get_scroll_bottom(page->result_box),
                                   LV_ANIM_OFF);
            }
        }
    }

    if(!s_done) {
        return;
    }
    s_done = 0;
    page = &s_pages[s_work_id];
    set_busy_ui(page, false);
    set_result_text(page, s_result, CLR_DEBUG);

    if(s_done_flags & NETDBG_DONE_DIALOG) {
        show_result_dialog(s_work_id == NETDBG_IP_CFG ? "DHCP结果" : "结果", s_result);
    }
    s_done_flags = 0;

    if(s_work_id == NETDBG_IP_CFG && page->goto_ip_btn) {
        lv_obj_add_flag(page->goto_ip_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void worker_finish(const char * msg)
{
    rt_strncpy(s_result, msg ? msg : "", NETDBG_RESULT_MAX - 1);
    s_result[NETDBG_RESULT_MAX - 1] = '\0';
    s_busy = 0;
    s_done = 1;
}

static void worker_finish_ex(const char * msg, int flags)
{
    s_done_flags = flags;
    worker_finish(msg);
}

static void worker_progress(const char * msg)
{
    rt_strncpy(s_result, msg ? msg : "", NETDBG_RESULT_MAX - 1);
    s_result[NETDBG_RESULT_MAX - 1] = '\0';
    s_progress = 1;
}

static void worker_entry(void * parameter)
{
    char buf[NETDBG_RESULT_MAX];
    struct netdev * nd;
    netdbg_pre_t pre;
    LV_UNUSED(parameter);

    nd = netdbg_get_netdev();
    s_done_flags = 0;

    switch(s_work_id) {
    case NETDBG_IP_CFG:
        /* DHCP */
        pre = netdbg_check_pre(false);
        if(pre == NETDBG_PRE_NO_LINK || pre == NETDBG_PRE_NO_DEV) {
            netdbg_pre_msg(pre, buf, sizeof(buf));
            worker_finish_ex(buf, NETDBG_DONE_DIALOG);
            return;
        }
        if(nd) {
            char ipbuf[16], gwbuf[16], maskbuf[16], dns0[16], dns1[16];
            netdev_dhcp_enabled(nd, RT_FALSE);
            rt_thread_mdelay(100);
            if(netdev_dhcp_enabled(nd, RT_TRUE) != RT_EOK) {
                worker_finish_ex("DHCP启动失败", NETDBG_DONE_DIALOG);
                return;
            }
            for(int i = 0; i < 40; i++) {
                if(!ip_addr_isany(&nd->ip_addr)) {
                    rt_strncpy(ipbuf, inet_ntoa(nd->ip_addr), sizeof(ipbuf) - 1);
                    rt_strncpy(gwbuf, inet_ntoa(nd->gw), sizeof(gwbuf) - 1);
                    rt_strncpy(maskbuf, inet_ntoa(nd->netmask), sizeof(maskbuf) - 1);
                    rt_strncpy(dns0, inet_ntoa(nd->dns_servers[0]), sizeof(dns0) - 1);
                    rt_strncpy(dns1, inet_ntoa(nd->dns_servers[1]), sizeof(dns1) - 1);
                    ipbuf[15] = gwbuf[15] = maskbuf[15] = dns0[15] = dns1[15] = '\0';
                    rt_snprintf(buf, sizeof(buf),
                                "DHCP成功\n"
                                "IP  %s\n"
                                "GW  %s\n"
                                "MASK %s\n"
                                "DNS0 %s\n"
                                "DNS1 %s",
                                ipbuf, gwbuf, maskbuf, dns0, dns1);
                    worker_finish_ex(buf, NETDBG_DONE_DIALOG);
                    return;
                }
                rt_thread_mdelay(500);
            }
            worker_finish_ex("DHCP超时\n仍为0.0.0.0", NETDBG_DONE_DIALOG);
        }
        break;

    case NETDBG_PING: {
        struct netdev_ping_resp resp;
        rt_uint32_t i, received = 0, lost = 0;
        rt_uint32_t min_t = 0xFFFFFFFF, max_t = 0, sum_t = 0;
        size_t used;
        int ret;

        pre = netdbg_check_pre(true);
        if(pre != NETDBG_PRE_OK) {
            netdbg_pre_msg(pre, buf, sizeof(buf));
            worker_finish(buf);
            return;
        }
        if(nd == RT_NULL || nd->ops == RT_NULL || nd->ops->ping == RT_NULL) {
            worker_finish("网卡不支持Ping");
            return;
        }

        used = (size_t)rt_snprintf(buf, sizeof(buf), "Ping %s x%d\n",
                                   s_work_host, s_work_times);
        worker_progress(buf);

        for(i = 0; i < (rt_uint32_t)s_work_times; i++) {
            rt_memset(&resp, 0, sizeof(resp));
            ret = nd->ops->ping(nd, s_work_host, NETDBG_PING_SIZE,
                                NETDBG_PING_TO_MS, &resp, RT_TRUE);
            if(ret == -RT_ETIMEOUT) {
                used += (size_t)rt_snprintf(buf + used, sizeof(buf) - used,
                                            "#%u 超时\n", i + 1);
                lost++;
            }
            else if(ret < 0) {
                used += (size_t)rt_snprintf(buf + used, sizeof(buf) - used,
                                            "#%u 失败\n", i + 1);
                lost++;
            }
            else {
                char ipbuf[16];
                rt_strncpy(ipbuf, inet_ntoa(resp.ip_addr), sizeof(ipbuf) - 1);
                ipbuf[15] = '\0';
                if(resp.ttl == 0) {
                    used += (size_t)rt_snprintf(buf + used, sizeof(buf) - used,
                                                "#%u %s %ums\n",
                                                i + 1, ipbuf, (unsigned)resp.ticks);
                }
                else {
                    used += (size_t)rt_snprintf(buf + used, sizeof(buf) - used,
                                                "#%u %s ttl%u %ums\n",
                                                i + 1, ipbuf,
                                                (unsigned)resp.ttl,
                                                (unsigned)resp.ticks);
                }
                received++;
                if(resp.ticks < min_t) min_t = resp.ticks;
                if(resp.ticks > max_t) max_t = resp.ticks;
                sum_t += resp.ticks;
            }
            worker_progress(buf);
            if(i + 1 < (rt_uint32_t)s_work_times) {
                rt_thread_mdelay(NETDBG_PING_GAP_MS);
            }
        }

        used += (size_t)rt_snprintf(buf + used, sizeof(buf) - used,
                                    "---统计---\n"
                                    "发%u 收%u 丢%u\n",
                                    (unsigned)s_work_times,
                                    (unsigned)received,
                                    (unsigned)lost);
        if(received > 0) {
            rt_snprintf(buf + used, sizeof(buf) - used,
                        "min%u max%u avg%u ms",
                        (unsigned)min_t, (unsigned)max_t,
                        (unsigned)(sum_t / received));
        }
        worker_finish(buf);
        break;
    }

    case NETDBG_TCP:
        pre = netdbg_check_pre(true);
        if(pre != NETDBG_PRE_OK) {
            netdbg_pre_msg(pre, buf, sizeof(buf));
            worker_finish(buf);
            return;
        }
        net_tester_ops_tcp_probe(s_work_host, s_work_port, 3000, buf, sizeof(buf));
        worker_finish(buf);
        break;
    case NETDBG_UDP:
        pre = netdbg_check_pre(true);
        if(pre != NETDBG_PRE_OK) {
            netdbg_pre_msg(pre, buf, sizeof(buf));
            worker_finish(buf);
            return;
        }
        net_tester_ops_udp_probe(s_work_host, s_work_port, 2000, buf, sizeof(buf));
        worker_finish(buf);
        break;
    case NETDBG_DNS:
        pre = netdbg_check_pre(true);
        if(pre != NETDBG_PRE_OK) {
            netdbg_pre_msg(pre, buf, sizeof(buf));
            worker_finish(buf);
            return;
        }
        net_tester_ops_dns_lookup(s_work_host, buf, sizeof(buf));
        worker_finish(buf);
        break;
    case NETDBG_HTTP:
        pre = netdbg_check_pre(true);
        if(pre != NETDBG_PRE_OK) {
            netdbg_pre_msg(pre, buf, sizeof(buf));
            worker_finish(buf);
            return;
        }
        net_tester_ops_http_get(s_work_host, s_work_port,
                                s_work_extra[0] ? s_work_extra : "/",
                                buf, sizeof(buf));
        worker_finish(buf);
        break;
    case NETDBG_ARP:
        pre = netdbg_check_pre(true);
        if(pre != NETDBG_PRE_OK) {
            netdbg_pre_msg(pre, buf, sizeof(buf));
            worker_finish(buf);
            return;
        }
        net_tester_ops_arp_query(s_work_host, buf, sizeof(buf));
        worker_finish(buf);
        break;

    default:
        worker_finish("未实现");
        break;
    }
}

static bool start_worker(netdbg_id_t id)
{
    rt_thread_t tid;

    if(s_busy) {
        return false;
    }
    s_busy = 1;
    s_done = 0;
    s_progress = 0;
    s_done_flags = 0;
    s_work_id = id;
    s_result[0] = '\0';

    tid = rt_thread_create("netdbg", worker_entry, RT_NULL,
                           NETDBG_WORKER_STACK, 20, 10);
    if(tid == RT_NULL) {
        s_busy = 0;
        return false;
    }
    rt_thread_startup(tid);
    return true;
}

/* ---------- 链路闪灯 ---------- */

#ifdef BSP_USING_CH390
static void blink_thread_entry(void * parameter)
{
    int i;
    LV_UNUSED(parameter);

    i = 0;
    while(!s_blink_abort && i < 10) {
        i++;
        ch390_phy_power_set(RT_FALSE);
        rt_thread_mdelay(1000);
        if(s_blink_abort) {
            break;
        }
        ch390_phy_power_set(RT_TRUE);
        rt_thread_mdelay(1000);
    }
    ch390_phy_power_set(RT_TRUE);
    s_blink_running = RT_FALSE;
    s_blink_tid = RT_NULL;
    rt_snprintf(s_result, sizeof(s_result), "闪灯结束\n共%d次", i);
    s_work_id = NETDBG_BLINK;
    s_busy = 0;
    s_done = 1;
}

static void blink_start_stop(netdbg_page_t * page)
{
    if(s_blink_running) {
        s_blink_abort = RT_TRUE;
        set_result_text(page, "正在停止...", CLR_WARN);
        if(page->start_lbl) {
            lv_label_set_text(page->start_lbl, "开始");
        }
        return;
    }

    s_blink_abort = RT_FALSE;
    s_blink_running = RT_TRUE;
    s_busy = 1;
    s_done = 0;
    s_blink_tid = rt_thread_create("blinkui", blink_thread_entry, RT_NULL,
                                   1024, 22, 10);
    if(s_blink_tid == RT_NULL) {
        s_blink_running = RT_FALSE;
        s_busy = 0;
        set_result_text(page, "启动失败", CLR_ERR);
        return;
    }
    rt_thread_startup(s_blink_tid);
    set_result_text(page, "闪灯中...\n看交换机口灯", CLR_DEBUG);
    if(page->start_lbl) {
        lv_label_set_text(page->start_lbl, "停止");
    }
}
#endif

/* ---------- 网卡信息刷新 ---------- */

static void fill_info_result(netdbg_page_t * page)
{
    struct netdev * nd = netdbg_get_netdev();
    char buf[NETDBG_RESULT_MAX];
    char ipbuf[16], gwbuf[16], maskbuf[16], dns0[16], dns1[16];
    size_t n = 0;

#ifdef BSP_USING_CH390
    {
        struct ch390_link_info info;
        if(ch390_get_link_info(&info) == RT_EOK) {
            n += (size_t)rt_snprintf(buf + n, sizeof(buf) - n,
                                     "链路 %s", info.link_up ? "UP" : "DOWN");
            if(info.link_up) {
                n += (size_t)rt_snprintf(buf + n, sizeof(buf) - n,
                                         " %uM %s\n",
                                         (unsigned)info.speed_mbps,
                                         info.full_duplex ? "全双工" : "半双工");
            }
            else {
                n += (size_t)rt_snprintf(buf + n, sizeof(buf) - n, "\n请检查网线\n");
            }
        }
    }
#endif

    if(nd == RT_NULL) {
        rt_snprintf(buf + n, sizeof(buf) - n, "无网卡");
        set_result_text(page, buf, CLR_ERR);
        return;
    }

    n += (size_t)rt_snprintf(buf + n, sizeof(buf) - n,
                             "%s %s %s\n",
                             nd->name,
                             netdev_is_up(nd) ? "UP" : "DOWN",
                             netdev_is_link_up(nd) ? "LINK" : "NO_LINK");
    n += (size_t)rt_snprintf(buf + n, sizeof(buf) - n,
                             "%s\n",
                             netdev_is_dhcp_enabled(nd) ? "DHCP开" : "DHCP关");

    /* inet_ntoa 共用静态缓冲，逐项拷贝后再拼接 */
    rt_strncpy(ipbuf, inet_ntoa(nd->ip_addr), sizeof(ipbuf) - 1);
    ipbuf[15] = '\0';
    if(ip_addr_isany(&nd->ip_addr)) {
        n += (size_t)rt_snprintf(buf + n, sizeof(buf) - n, "IP 未设置");
    }
    else {
        rt_strncpy(gwbuf, inet_ntoa(nd->gw), sizeof(gwbuf) - 1);
        rt_strncpy(maskbuf, inet_ntoa(nd->netmask), sizeof(maskbuf) - 1);
        rt_strncpy(dns0, inet_ntoa(nd->dns_servers[0]), sizeof(dns0) - 1);
        rt_strncpy(dns1, inet_ntoa(nd->dns_servers[1]), sizeof(dns1) - 1);
        gwbuf[15] = maskbuf[15] = dns0[15] = dns1[15] = '\0';
        n += (size_t)rt_snprintf(buf + n, sizeof(buf) - n,
                                 "IP   %s\n"
                                 "GW   %s\n"
                                 "MASK %s\n"
                                 "DNS0 %s\n"
                                 "DNS1 %s",
                                 ipbuf, gwbuf, maskbuf, dns0, dns1);
    }
    set_result_text(page, buf,
                    netdev_is_link_up(nd) ? CLR_DEBUG : CLR_WARN);
}

/* ---------- 页面动作 ---------- */

static void open_page(netdbg_id_t id);

static void apply_static_ip(netdbg_page_t * page)
{
    char ip[32], gw[32], mask[32];
    ip_addr_t addr;
    struct netdev * nd;
    netdbg_pre_t pre;

    pre = netdbg_check_pre(false);
    if(pre == NETDBG_PRE_NO_LINK || pre == NETDBG_PRE_NO_DEV) {
        char msg[64];
        netdbg_pre_msg(pre, msg, sizeof(msg));
        set_result_text(page, msg, CLR_WARN);
        return;
    }

    if(!net_tester_ip_input_get_value(&page->ip, ip, sizeof(ip)) ||
       !net_tester_ip_input_get_value(&page->ip_gw, gw, sizeof(gw)) ||
       !net_tester_ip_input_get_value(&page->ip_mask, mask, sizeof(mask))) {
        set_result_text(page, "IP/GW/MASK无效", CLR_ERR);
        return;
    }

    nd = netdbg_get_netdev();
    if(!nd) {
        set_result_text(page, "未找到网卡", CLR_ERR);
        return;
    }

    netdev_dhcp_enabled(nd, RT_FALSE);
    inet_aton(ip, &addr);
    netdev_set_ipaddr(nd, &addr);
    inet_aton(gw, &addr);
    netdev_set_gw(nd, &addr);
    inet_aton(mask, &addr);
    netdev_set_netmask(nd, &addr);

    {
        char buf[128];
        rt_snprintf(buf, sizeof(buf), "静态已应用\nIP %s\nGW %s", ip, gw);
        set_result_text(page, buf, CLR_DEBUG);
    }
}

static void on_page_start(netdbg_id_t id)
{
    netdbg_page_t * page = &s_pages[id];
    netdbg_pre_t pre;
    char msg[96];

    if(id == NETDBG_INFO) {
        fill_info_result(page);
        return;
    }

#ifdef BSP_USING_CH390
    if(id == NETDBG_BLINK) {
        blink_start_stop(page);
        return;
    }
#endif

    if(id == NETDBG_IP_CFG) {
        /* 开始按钮在 IP 页不用；DHCP/静态走专用按钮 */
        return;
    }

    if(s_busy) {
        set_result_text(page, "忙，请稍候", CLR_WARN);
        return;
    }

    /* 依赖检查 */
    if(id != NETDBG_IP_CFG) {
        bool need_ip = (id != NETDBG_BLINK);
        pre = netdbg_check_pre(need_ip);
        if(pre != NETDBG_PRE_OK) {
            netdbg_pre_msg(pre, msg, sizeof(msg));
            set_result_text(page, msg, CLR_WARN);
            if(pre == NETDBG_PRE_NO_IP && page->goto_ip_btn) {
                lv_obj_clear_flag(page->goto_ip_btn, LV_OBJ_FLAG_HIDDEN);
            }
            return;
        }
    }

    if(page->has_ip) {
        if(!net_tester_ip_input_get_value(&page->ip, s_work_host, sizeof(s_work_host))) {
            set_result_text(page, "目标IP无效", CLR_ERR);
            return;
        }
    }

    s_work_extra[0] = '\0';
    s_work_port = 80;
    s_work_times = 4;

    if(page->has_ascii) {
        const char * ascii = net_tester_ascii_input_get_value(&page->ascii);
        if(id == NETDBG_PING) {
            int t = atoi(ascii);
            if(t > 0 && t < 20) {
                s_work_times = t;
            }
        }
        else if(id == NETDBG_DNS) {
            if(ascii && ascii[0]) {
                rt_strncpy(s_work_host, ascii, sizeof(s_work_host) - 1);
            }
            else {
                set_result_text(page, "请输入域名", CLR_ERR);
                return;
            }
        }
        else if(id == NETDBG_HTTP) {
            /* ASCII: 端口或 /path；若以 / 开头当路径，否则当端口 */
            if(ascii && ascii[0] == '/') {
                rt_strncpy(s_work_extra, ascii, sizeof(s_work_extra) - 1);
                s_work_port = 80;
            }
            else {
                int p = atoi(ascii);
                s_work_port = (p > 0) ? p : 80;
                rt_strncpy(s_work_extra, "/", sizeof(s_work_extra) - 1);
            }
        }
        else if(id == NETDBG_TCP || id == NETDBG_UDP) {
            int p = atoi(ascii);
            s_work_port = (p > 0) ? p : ((id == NETDBG_TCP) ? 80 : 53);
        }
    }

    set_busy_ui(page, true);
    set_result_text(page, "测试中...", UI_FOOTER);
    if(!start_worker(id)) {
        set_busy_ui(page, false);
        set_result_text(page, "无法创建线程", CLR_ERR);
    }
}

static void on_dhcp_clicked(void)
{
    netdbg_page_t * page = &s_pages[NETDBG_IP_CFG];
    if(s_busy) {
        return;
    }
    set_busy_ui(page, true);
    set_result_text(page, "DHCP中...", UI_FOOTER);
    if(!start_worker(NETDBG_IP_CFG)) {
        set_busy_ui(page, false);
        set_result_text(page, "无法创建线程", CLR_ERR);
    }
}

/* ---------- 控件事件 ---------- */

static bool indev_is_keypad(void)
{
    lv_indev_t * indev = lv_indev_get_act();
    return indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD;
}

static void page_widget_esc_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) {
        return;
    }
    if(lv_event_get_key(e) != LV_KEY_ESC) {
        return;
    }
    /* 复用统一 ESC：先退编辑，再回菜单 */
    if(net_tester_netdbg_handle_esc()) {
        lv_event_stop_processing(e);
    }
}

static void page_start_event_cb(lv_event_t * e)
{
    netdbg_id_t id = (netdbg_id_t)(uintptr_t)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        btn_focus_cb(e);
        return;
    }
    if(code == LV_EVENT_CLICKED) {
        if(indev_is_keypad()) {
            return;
        }
        on_page_start(id);
    }
    else if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ENTER) {
            on_page_start(id);
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ESC) {
            if(net_tester_netdbg_handle_esc()) {
                lv_event_stop_processing(e);
            }
        }
    }
}

static void dhcp_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        btn_focus_cb(e);
        return;
    }
    if(code == LV_EVENT_CLICKED) {
        if(indev_is_keypad()) {
            return;
        }
        on_dhcp_clicked();
    }
    else if(code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ENTER) {
        on_dhcp_clicked();
        lv_event_stop_processing(e);
    }
    else if(code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        if(net_tester_netdbg_handle_esc()) {
            lv_event_stop_processing(e);
        }
    }
}

static void static_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        btn_focus_cb(e);
        return;
    }
    if(code == LV_EVENT_CLICKED) {
        if(indev_is_keypad()) {
            return;
        }
        apply_static_ip(&s_pages[NETDBG_IP_CFG]);
    }
    else if(code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ENTER) {
        apply_static_ip(&s_pages[NETDBG_IP_CFG]);
        lv_event_stop_processing(e);
    }
    else if(code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        if(net_tester_netdbg_handle_esc()) {
            lv_event_stop_processing(e);
        }
    }
}

static void goto_ip_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        btn_focus_cb(e);
        return;
    }
    if(code == LV_EVENT_CLICKED ||
       (code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ENTER)) {
        open_page(NETDBG_IP_CFG);
        if(code == LV_EVENT_KEY) {
            lv_event_stop_processing(e);
        }
    }
}

static void menu_btn_event_cb(lv_event_t * e)
{
    netdbg_id_t id = (netdbg_id_t)(uintptr_t)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_current_target(e);

    if(code == LV_EVENT_FOCUSED) {
        style_action_btn(btn, true);
        lv_obj_set_style_border_color(btn, lv_color_hex(CLR_DEBUG), 0);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        style_action_btn(btn, false);
    }
    else if(code == LV_EVENT_CLICKED) {
        /* Enter 松开会再发 CLICKED：主菜单进调试后焦点已在首项，会误开 IP 设置 */
        if(indev_is_keypad()) {
            return;
        }
        open_page(id);
    }
    else if(code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ENTER) {
        open_page(id);
        lv_event_stop_processing(e);
    }
    else if(code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        if(s_leave_cb) {
            s_leave_cb();
        }
        lv_event_stop_processing(e);
    }
}

/* ---------- 创建页面 ---------- */

static lv_obj_t * make_sm_label(lv_obj_t * parent, const char * text, lv_coord_t y)
{
    lv_obj_t * lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, UI_SM_FONT, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_DEBUG), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, y);
    return lbl;
}

static void result_set_browse(netdbg_page_t * page, bool on)
{
    if(!page || !page->result_box) {
        return;
    }
    page->result_browsing = on;
    if(on) {
        lv_obj_set_style_border_color(page->result_box, lv_color_hex(CLR_DEBUG), 0);
        lv_obj_set_style_border_width(page->result_box, 2, 0);
        if(s_bound_group) {
            lv_group_set_editing(s_bound_group, true);
        }
    }
    else {
        lv_obj_t * focused = s_bound_group ? lv_group_get_focused(s_bound_group) : NULL;
        bool focused_here = (focused == page->result_box);
        lv_obj_set_style_border_color(page->result_box,
                                      lv_color_hex(focused_here ? UI_TITLE : UI_BORDER), 0);
        lv_obj_set_style_border_width(page->result_box, focused_here ? 2 : 1, 0);
        if(s_bound_group) {
            lv_group_set_editing(s_bound_group, false);
        }
    }
}

static void result_box_event_cb(lv_event_t * e)
{
    netdbg_page_t * page = (netdbg_page_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * box = lv_event_get_current_target(e);

    if(!page) {
        return;
    }

    if(code == LV_EVENT_FOCUSED) {
        if(!page->result_browsing) {
            lv_obj_set_style_border_color(box, lv_color_hex(UI_TITLE), 0);
            lv_obj_set_style_border_width(box, 2, 0);
        }
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        result_set_browse(page, false);
        lv_obj_set_style_border_color(box, lv_color_hex(UI_BORDER), 0);
        lv_obj_set_style_border_width(box, 1, 0);
    }
    else if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ENTER) {
            result_set_browse(page, !page->result_browsing);
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ESC) {
            if(page->result_browsing) {
                result_set_browse(page, false);
                lv_event_stop_processing(e);
            }
            else if(net_tester_netdbg_handle_esc()) {
                lv_event_stop_processing(e);
            }
        }
        else if(page->result_browsing) {
            /* 浏览模式：编码器 LEFT/RIGHT 滚动；正 y 为向下看更下方内容 */
            if(key == LV_KEY_LEFT || key == LV_KEY_UP || key == LV_KEY_PREV) {
                lv_obj_scroll_by(box, 0, -24, LV_ANIM_OFF);
                lv_event_stop_processing(e);
            }
            else if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN || key == LV_KEY_NEXT) {
                lv_obj_scroll_by(box, 0, 24, LV_ANIM_OFF);
                lv_event_stop_processing(e);
            }
        }
    }
    else if(code == LV_EVENT_CLICKED) {
        if(indev_is_keypad()) {
            return;
        }
        result_set_browse(page, !page->result_browsing);
    }
}

static lv_obj_t * make_result_box(lv_obj_t * parent, netdbg_page_t * page,
                                  lv_coord_t y, lv_coord_t h)
{
    lv_obj_t * box = lv_obj_create(parent);
    page->result_box = box;
    page->result_browsing = false;
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, SCR_W - 12, h);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_bg_color(box, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, 3, 0);
    lv_obj_set_style_pad_all(box, 4, 0);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(box, result_box_event_cb, LV_EVENT_ALL, page);

    page->result_lbl = lv_label_create(box);
    lv_label_set_text(page->result_lbl, "");
    lv_label_set_long_mode(page->result_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(page->result_lbl, SCR_W - 24);
    lv_obj_set_style_text_font(page->result_lbl, UI_SM_FONT, 0);
    lv_obj_set_style_text_color(page->result_lbl, lv_color_hex(UI_FOOTER), 0);
    return box;
}

static lv_obj_t * make_start_btn(lv_obj_t * parent, netdbg_page_t * page,
                                 netdbg_id_t id, lv_coord_t y)
{
    page->start_btn = lv_btn_create(parent);
    lv_obj_remove_style_all(page->start_btn);
    lv_obj_set_size(page->start_btn, 96, 32);
    lv_obj_align(page->start_btn, LV_ALIGN_TOP_MID, 0, y);
    style_action_btn(page->start_btn, false);
    lv_obj_add_event_cb(page->start_btn, page_start_event_cb, LV_EVENT_ALL,
                        (void *)(uintptr_t)id);

    page->start_lbl = lv_label_create(page->start_btn);
    lv_label_set_text(page->start_lbl, "开始");
    lv_obj_set_style_text_font(page->start_lbl, UI_CN_FONT, 0);
    lv_obj_set_style_text_color(page->start_lbl, lv_color_hex(CLR_DEBUG), 0);
    lv_obj_center(page->start_lbl);
    return page->start_btn;
}

static void create_ip_cfg_page(void)
{
    netdbg_page_t * page = &s_pages[NETDBG_IP_CFG];
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_t * body;
    lv_obj_t * box;

    style_screen_bg(scr);
    create_header(scr, "IP设置");
    page->screen = scr;
    page->has_ip = true;

    body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, SCR_W, SCR_H - HDR_H);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    make_sm_label(body, "本机IP", 2);
    box = net_tester_ip_input_create(body, &page->ip);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_add_event_cb(box, page_widget_esc_cb, LV_EVENT_KEY, NULL);

    make_sm_label(body, "网关", 46);
    box = net_tester_ip_input_create(body, &page->ip_gw);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 60);
    net_tester_ip_input_set_value(&page->ip_gw, "192.168.1.1");
    lv_obj_add_event_cb(box, page_widget_esc_cb, LV_EVENT_KEY, NULL);

    make_sm_label(body, "掩码", 90);
    box = net_tester_ip_input_create(body, &page->ip_mask);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 104);
    net_tester_ip_input_set_value(&page->ip_mask, "255.255.255.0");
    lv_obj_add_event_cb(box, page_widget_esc_cb, LV_EVENT_KEY, NULL);

    page->dhcp_btn = lv_btn_create(body);
    lv_obj_remove_style_all(page->dhcp_btn);
    lv_obj_set_size(page->dhcp_btn, 88, 30);
    lv_obj_align(page->dhcp_btn, LV_ALIGN_TOP_LEFT, 20, 138);
    style_action_btn(page->dhcp_btn, false);
    lv_obj_add_event_cb(page->dhcp_btn, dhcp_btn_event_cb, LV_EVENT_ALL, NULL);
    {
        lv_obj_t * l = lv_label_create(page->dhcp_btn);
        lv_label_set_text(l, "DHCP");
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(CLR_DEBUG), 0);
        lv_obj_center(l);
    }

    page->static_btn = lv_btn_create(body);
    lv_obj_remove_style_all(page->static_btn);
    lv_obj_set_size(page->static_btn, 88, 30);
    lv_obj_align(page->static_btn, LV_ALIGN_TOP_RIGHT, -20, 138);
    style_action_btn(page->static_btn, false);
    lv_obj_add_event_cb(page->static_btn, static_btn_event_cb, LV_EVENT_ALL, NULL);
    {
        lv_obj_t * l = lv_label_create(page->static_btn);
        lv_label_set_text(l, "静态");
        lv_obj_set_style_text_font(l, UI_SM_FONT, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(CLR_DEBUG), 0);
        lv_obj_center(l);
    }

    make_result_box(body, page, 172, 32);
    set_result_text(page, "选DHCP或静态", UI_FOOTER);
}

static void create_simple_probe_page(netdbg_id_t id, const char * title,
                                     const char * ascii_title, const char * ascii_def,
                                     bool need_ip, bool need_ascii)
{
    netdbg_page_t * page = &s_pages[id];
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_t * body;
    lv_coord_t y = 2;

    style_screen_bg(scr);
    create_header(scr, title);
    page->screen = scr;
    page->has_ip = need_ip;
    page->has_ascii = need_ascii;

    body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, SCR_W, SCR_H - HDR_H);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    if(need_ip) {
        make_sm_label(body, "目标IP", y);
        y += 14;
        {
            lv_obj_t * box = net_tester_ip_input_create(body, &page->ip);
            lv_obj_align(box, LV_ALIGN_TOP_MID, 0, y);
            lv_obj_add_event_cb(box, page_widget_esc_cb, LV_EVENT_KEY, NULL);
        }
        y += 32;
    }

    if(need_ascii) {
        make_sm_label(body, ascii_title, y);
        y += 14;
        {
            lv_obj_t * box = net_tester_ascii_input_create(body, &page->ascii);
            lv_obj_align(box, LV_ALIGN_TOP_MID, 0, y);
            if(ascii_def) {
                net_tester_ascii_input_set_value(&page->ascii, ascii_def);
            }
            lv_obj_add_event_cb(box, page_widget_esc_cb, LV_EVENT_KEY, NULL);
        }
        y += 32;
    }

    make_result_box(body, page, y, (lv_coord_t)(SCR_H - HDR_H - y - 40));
    y = SCR_H - HDR_H - 36;
    make_start_btn(body, page, id, y);

    /* 无IP时引导去设置 */
    page->goto_ip_btn = lv_btn_create(body);
    lv_obj_remove_style_all(page->goto_ip_btn);
    lv_obj_set_size(page->goto_ip_btn, 100, 24);
    lv_obj_align(page->goto_ip_btn, LV_ALIGN_TOP_RIGHT, -6, 2);
    style_action_btn(page->goto_ip_btn, false);
    lv_obj_add_event_cb(page->goto_ip_btn, goto_ip_event_cb, LV_EVENT_ALL, NULL);
    {
        lv_obj_t * l = lv_label_create(page->goto_ip_btn);
        lv_label_set_text(l, "IP设置");
        lv_obj_set_style_text_font(l, UI_SM_FONT, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(CLR_WARN), 0);
        lv_obj_center(l);
    }
    lv_obj_add_flag(page->goto_ip_btn, LV_OBJ_FLAG_HIDDEN);

    set_result_text(page, "按开始测试", UI_FOOTER);
}

static void create_all_pages(void)
{
    create_ip_cfg_page();
    create_simple_probe_page(NETDBG_INFO, "网卡信息", NULL, NULL, false, false);
    create_simple_probe_page(NETDBG_PING, "Ping", "次数", "4", true, true);
    create_simple_probe_page(NETDBG_TCP, "TCP探测", "端口", "80", true, true);
    create_simple_probe_page(NETDBG_UDP, "UDP探测", "端口", "53", true, true);
    create_simple_probe_page(NETDBG_DNS, "DNS查询", "域名", "baidu.com", false, true);
    create_simple_probe_page(NETDBG_HTTP, "HTTP获取", "端口/路径", "80", true, true);
    create_simple_probe_page(NETDBG_ARP, "ARP查询", NULL, NULL, true, false);
    create_simple_probe_page(NETDBG_BLINK, "链路闪灯", NULL, NULL, false, false);
}

static void bind_page_group(netdbg_id_t id)
{
    netdbg_page_t * page = &s_pages[id];
    lv_group_t * g = s_bound_group;

    if(!g || !page->screen) {
        return;
    }

    lv_group_remove_all_objs(g);
    lv_group_set_editing(g, false);

    if(id == NETDBG_IP_CFG) {
        net_tester_ip_input_cancel_edit(&page->ip);
        net_tester_ip_input_cancel_edit(&page->ip_gw);
        net_tester_ip_input_cancel_edit(&page->ip_mask);
        lv_group_add_obj(g, page->ip.root);
        lv_group_add_obj(g, page->ip_gw.root);
        lv_group_add_obj(g, page->ip_mask.root);
        lv_group_add_obj(g, page->dhcp_btn);
        lv_group_add_obj(g, page->static_btn);
        lv_group_focus_obj(page->dhcp_btn);
    }
    else {
        if(page->has_ip) {
            net_tester_ip_input_cancel_edit(&page->ip);
            lv_group_add_obj(g, page->ip.root);
        }
        if(page->has_ascii) {
            net_tester_ascii_input_cancel_active(&page->ascii);
            lv_group_add_obj(g, page->ascii.root);
        }
        if(page->start_btn) {
            lv_group_add_obj(g, page->start_btn);
        }
        if(page->goto_ip_btn && !lv_obj_has_flag(page->goto_ip_btn, LV_OBJ_FLAG_HIDDEN)) {
            lv_group_add_obj(g, page->goto_ip_btn);
        }
        if(page->result_box) {
            page->result_browsing = false;
            lv_group_add_obj(g, page->result_box);
        }
        if(page->has_ip) {
            lv_group_focus_obj(page->ip.root);
            net_tester_ip_input_refresh_focus(&page->ip, true);
        }
        else if(page->has_ascii) {
            lv_group_focus_obj(page->ascii.root);
        }
        else if(page->start_btn) {
            lv_group_focus_obj(page->start_btn);
        }
        else if(page->result_box) {
            lv_group_focus_obj(page->result_box);
        }
    }

    /* IP 设置页结果区也可聚焦浏览 */
    if(id == NETDBG_IP_CFG && page->result_box) {
        page->result_browsing = false;
        lv_group_add_obj(g, page->result_box);
    }
}

static void open_page(netdbg_id_t id)
{
    if(id < 0 || id >= NETDBG_COUNT || !s_pages[id].screen) {
        return;
    }
    s_cur_id = id;
    if(id == NETDBG_INFO) {
        fill_info_result(&s_pages[id]);
    }
    bind_page_group(id);
    lv_scr_load(s_pages[id].screen);
}

static void open_menu(void)
{
    int i;
    s_cur_id = NETDBG_COUNT;
    if(s_bound_group) {
        lv_group_remove_all_objs(s_bound_group);
        for(i = 0; i < NETDBG_COUNT; i++) {
            lv_group_add_obj(s_bound_group, s_menu_btns[i]);
        }
        lv_group_focus_obj(s_menu_btns[0]);
    }
    lv_scr_load(s_menu_screen);
}

/* ---------- 对外接口 ---------- */

lv_obj_t * net_tester_netdbg_create_menu(void)
{
    lv_obj_t * list;
    int i;

    s_menu_screen = lv_obj_create(NULL);
    style_screen_bg(s_menu_screen);
    create_header(s_menu_screen, "网络调试");

    list = lv_obj_create(s_menu_screen);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, SCR_W, SCR_H - HDR_H);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_set_style_pad_all(list, 4, 0);
    lv_obj_set_style_pad_row(list, 3, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    for(i = 0; i < NETDBG_COUNT; i++) {
        lv_obj_t * btn = lv_btn_create(list);
        lv_obj_t * ico;
        lv_obj_t * name;

        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, SCR_W - 16, 36);
        style_action_btn(btn, false);
        lv_obj_set_style_pad_hor(btn, 10, 0);
        lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_ALL, (void *)(uintptr_t)i);

        ico = lv_label_create(btn);
        lv_label_set_text(ico, s_menu_desc[i].icon);
        lv_obj_set_style_text_font(ico, UI_ICON_FONT, 0);
        lv_obj_set_style_text_color(ico, lv_color_hex(CLR_DEBUG), 0);
        lv_obj_align(ico, LV_ALIGN_LEFT_MID, 0, 0);

        name = lv_label_create(btn);
        lv_label_set_text(name, s_menu_desc[i].name);
        lv_obj_set_style_text_font(name, UI_CN_FONT, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(CLR_DEBUG), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 36, 0);

        s_menu_btns[i] = btn;
    }

    create_all_pages();
    s_poll_timer = lv_timer_create(poll_timer_cb, 100, NULL);
    s_cur_id = NETDBG_COUNT;
    return s_menu_screen;
}

void net_tester_netdbg_bind_group(lv_group_t * group)
{
    int i;
    s_bound_group = group;
    if(!group) {
        return;
    }

    if(s_cur_id < NETDBG_COUNT) {
        bind_page_group(s_cur_id);
        return;
    }

    lv_group_remove_all_objs(group);
    for(i = 0; i < NETDBG_COUNT; i++) {
        lv_group_add_obj(group, s_menu_btns[i]);
    }
    lv_group_focus_obj(s_menu_btns[0]);
}

bool net_tester_netdbg_handle_esc(void)
{
    netdbg_page_t * page;

    /* 先关结果弹窗 */
    if(s_dialog) {
        dialog_close();
        return true;
    }

    if(s_cur_id >= NETDBG_COUNT) {
        return false;
    }

    page = &s_pages[s_cur_id];

    /* 结果区浏览模式：先退出浏览 */
    if(page->result_browsing) {
        result_set_browse(page, false);
        return true;
    }

    if(page->has_ascii && net_tester_ascii_input_is_active(&page->ascii)) {
        net_tester_ascii_input_handle_esc(&page->ascii);
        return true;
    }
    if(page->has_ip && net_tester_ip_input_is_editing(&page->ip)) {
        net_tester_ip_input_cancel_edit(&page->ip);
        net_tester_ip_input_refresh_focus(&page->ip, true);
        return true;
    }
    if(s_cur_id == NETDBG_IP_CFG) {
        if(net_tester_ip_input_is_editing(&page->ip_gw)) {
            net_tester_ip_input_cancel_edit(&page->ip_gw);
            return true;
        }
        if(net_tester_ip_input_is_editing(&page->ip_mask)) {
            net_tester_ip_input_cancel_edit(&page->ip_mask);
            return true;
        }
    }

    open_menu();
    return true;
}

bool net_tester_netdbg_is_on_screen(void)
{
    lv_obj_t * act = lv_scr_act();
    int i;

    if(act == s_menu_screen) {
        return true;
    }
    for(i = 0; i < NETDBG_COUNT; i++) {
        if(act == s_pages[i].screen) {
            return true;
        }
    }
    return false;
}

uint32_t net_tester_netdbg_key_remap(uint32_t key)
{
    netdbg_page_t * page;

    if(key != LV_KEY_NEXT || s_cur_id >= NETDBG_COUNT) {
        return key;
    }
    page = &s_pages[s_cur_id];

    if(page->has_ip && net_tester_ip_input_is_editing(&page->ip)) {
        return NET_TESTER_IP_KEY_DIGIT_NEXT;
    }
    if(s_cur_id == NETDBG_IP_CFG) {
        if(net_tester_ip_input_is_editing(&page->ip_gw) ||
           net_tester_ip_input_is_editing(&page->ip_mask)) {
            return NET_TESTER_IP_KEY_DIGIT_NEXT;
        }
    }
    if(page->has_ascii) {
        if(net_tester_ascii_input_wants_tab_key(&page->ascii)) {
            return NET_TESTER_ASCII_KEY_TAB;
        }
        if(net_tester_ascii_input_wants_tab_next(&page->ascii)) {
            return NET_TESTER_ASCII_KEY_TAB_NEXT;
        }
    }
    return key;
}

void net_tester_netdbg_on_leave(void)
{
    int i;

    if(s_blink_running) {
        s_blink_abort = RT_TRUE;
    }

    for(i = 0; i < NETDBG_COUNT; i++) {
        netdbg_page_t * page = &s_pages[i];
        if(page->has_ip) {
            net_tester_ip_input_cancel_edit(&page->ip);
        }
        if(page->has_ascii) {
            net_tester_ascii_input_cancel_active(&page->ascii);
        }
    }
    if(s_pages[NETDBG_IP_CFG].screen) {
        net_tester_ip_input_cancel_edit(&s_pages[NETDBG_IP_CFG].ip_gw);
        net_tester_ip_input_cancel_edit(&s_pages[NETDBG_IP_CFG].ip_mask);
    }
    s_cur_id = NETDBG_COUNT;
}

void net_tester_netdbg_set_leave_cb(net_tester_netdbg_leave_cb_t cb)
{
    s_leave_cb = cb;
}
