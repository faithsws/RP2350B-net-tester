#include "net_tester_ip_input.h"

#include <stdio.h>
#include <string.h>

#define IP_DIGIT_COUNT  12
#define CLR_EDIT        0x44BB44
#define CLR_FOCUS       0x6EC8D8
#define CLR_NORMAL      0xC0C8D0
#define CLR_DIM         0x4A5A6A

static void update_digit_labels(net_tester_ip_input_t * ctx)
{
    for(int i = 0; i < IP_DIGIT_COUNT; i++) {
        lv_label_set_text_fmt(ctx->digit_lbl[i], "%d", ctx->digits[i]);
    }
}

static void update_digit_style(net_tester_ip_input_t * ctx)
{
    for(int i = 0; i < IP_DIGIT_COUNT; i++) {
        lv_obj_t * lbl = ctx->digit_lbl[i];
        if(ctx->editing && i == ctx->cur_digit) {
            if(ctx->blink_on) {
                lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_EDIT), 0);
                lv_obj_set_style_bg_color(lbl, lv_color_hex(0x1A3020), 0);
                lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
            }
            else {
                lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_EDIT), 0);
                lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
            }
        }
        else {
            lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_NORMAL), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
        }
    }
}

static void apply_root_style(net_tester_ip_input_t * ctx, bool focused)
{
    if(!ctx || !ctx->root) return;

    if(ctx->editing) {
        /* 与 ASCII 编辑态一致：绿色粗边框表示正在编辑 */
        lv_obj_set_style_border_color(ctx->root, lv_color_hex(CLR_EDIT), 0);
        lv_obj_set_style_border_width(ctx->root, 2, 0);
    }
    else if(focused) {
        lv_obj_set_style_border_color(ctx->root, lv_color_hex(CLR_FOCUS), 0);
        lv_obj_set_style_border_width(ctx->root, 2, 0);
    }
    else {
        lv_obj_set_style_border_color(ctx->root, lv_color_hex(0x243040), 0);
        lv_obj_set_style_border_width(ctx->root, 1, 0);
    }
}

static void blink_timer_cb(lv_timer_t * t)
{
    net_tester_ip_input_t * ctx = (net_tester_ip_input_t *)t->user_data;
    if(!ctx || !ctx->editing) return;
    ctx->blink_on = !ctx->blink_on;
    update_digit_style(ctx);
}

static int octet_from_digits(const uint8_t * d)
{
    return d[0] * 100 + d[1] * 10 + d[2];
}

static bool validate_and_format(const net_tester_ip_input_t * ctx, char * out, size_t out_sz)
{
    int o1 = octet_from_digits(&ctx->digits[0]);
    int o2 = octet_from_digits(&ctx->digits[3]);
    int o3 = octet_from_digits(&ctx->digits[6]);
    int o4 = octet_from_digits(&ctx->digits[9]);

    if(o1 < 0 || o1 > 255 || o2 < 0 || o2 > 255 || o3 < 0 || o3 > 255 || o4 < 0 || o4 > 255) {
        return false;
    }

    snprintf(out, out_sz, "%d.%d.%d.%d", o1, o2, o3, o4);
    return true;
}

static bool is_octet_valid_with_digit(const net_tester_ip_input_t * ctx, int digit_idx, uint8_t new_digit)
{
    int octet_start = (digit_idx / 3) * 3;
    uint8_t d[3] = {
        ctx->digits[octet_start],
        ctx->digits[octet_start + 1],
        ctx->digits[octet_start + 2]
    };

    d[digit_idx - octet_start] = new_digit;
    int v = octet_from_digits(d);
    return v >= 0 && v <= 255;
}

static void adjust_current_digit(net_tester_ip_input_t * ctx, int delta)
{
    int v = (int)ctx->digits[ctx->cur_digit] + delta;
    if(v < 0) v = 9;
    if(v > 9) v = 0;

    /* 滚轮/编码器调整前预判该段是否仍为合法 IP 段 (0-255) */
    if(!is_octet_valid_with_digit(ctx, ctx->cur_digit, (uint8_t)v)) {
        return;
    }

    ctx->digits[ctx->cur_digit] = (uint8_t)v;
    update_digit_labels(ctx);
    update_digit_style(ctx);
}

static void next_digit(net_tester_ip_input_t * ctx)
{
    ctx->cur_digit = (uint8_t)((ctx->cur_digit + 1) % IP_DIGIT_COUNT);
    ctx->blink_on = true;
    update_digit_style(ctx);
}

void net_tester_ip_input_enter_edit(net_tester_ip_input_t * ctx)
{
    if(!ctx || ctx->editing) return;
    ctx->editing = true;
    ctx->cur_digit = 0;
    ctx->blink_on = true;
    lv_group_t * g = lv_obj_get_group(ctx->root);
    if(g) lv_group_set_editing(g, true);
    update_digit_style(ctx);
    apply_root_style(ctx, true);
}

bool net_tester_ip_input_exit_edit(net_tester_ip_input_t * ctx)
{
    if(!ctx || !ctx->editing) return false;

    char ip_str[32];
    bool ok = validate_and_format(ctx, ip_str, sizeof(ip_str));

    if(ok) {
        rt_kprintf("[IP] saved: %s\n", ip_str);
    }
    else {
        rt_kprintf("[IP] invalid address, not saved\n");
    }

    ctx->editing = false;
    lv_group_t * g = lv_obj_get_group(ctx->root);
    if(g) lv_group_set_editing(g, false);
    update_digit_style(ctx);
    if(g && lv_group_get_focused(g) == ctx->root) {
        net_tester_ip_input_refresh_focus(ctx, true);
    }
    else {
        apply_root_style(ctx, false);
    }
    return ok;
}

bool net_tester_ip_input_is_editing(const net_tester_ip_input_t * ctx)
{
    return ctx && ctx->editing;
}

void net_tester_ip_input_cancel_edit(net_tester_ip_input_t * ctx)
{
    if(!ctx || !ctx->editing) return;
    ctx->editing = false;
    ctx->blink_on = false;
    lv_group_t * g = lv_obj_get_group(ctx->root);
    if(g) lv_group_set_editing(g, false);
    update_digit_style(ctx);
    if(g && lv_group_get_focused(g) == ctx->root) {
        apply_root_style(ctx, true);
    }
    else {
        apply_root_style(ctx, false);
    }
}

void net_tester_ip_input_handle_key(net_tester_ip_input_t * ctx, uint32_t key)
{
    if(!ctx) return;

    if(ctx->editing) {
        if(key == LV_KEY_ENTER) {
            net_tester_ip_input_exit_edit(ctx);
        }
        else if(key == NET_TESTER_IP_KEY_DIGIT_NEXT || key == LV_KEY_NEXT) {
            next_digit(ctx);
        }
        else if(key == LV_KEY_LEFT) {
            adjust_current_digit(ctx, -1);
        }
        else if(key == LV_KEY_RIGHT) {
            adjust_current_digit(ctx, 1);
        }
    }
}

void net_tester_ip_input_refresh_focus(net_tester_ip_input_t * ctx, bool focused)
{
    if(!ctx || !ctx->root) return;
    apply_root_style(ctx, focused);
}

bool net_tester_ip_input_get_value(const net_tester_ip_input_t * ctx, char * out, size_t out_sz)
{
    if(!ctx || !out || out_sz == 0) return false;

    int o1 = octet_from_digits(&ctx->digits[0]);
    int o2 = octet_from_digits(&ctx->digits[3]);
    int o3 = octet_from_digits(&ctx->digits[6]);
    int o4 = octet_from_digits(&ctx->digits[9]);

    snprintf(out, out_sz, "%d.%d.%d.%d", o1, o2, o3, o4);
    return o1 >= 0 && o1 <= 255 && o2 >= 0 && o2 <= 255 &&
           o3 >= 0 && o3 <= 255 && o4 >= 0 && o4 <= 255;
}

static void ip_root_event_cb(lv_event_t * e)
{
    net_tester_ip_input_t * ctx = (net_tester_ip_input_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(ctx->editing) {
            net_tester_ip_input_handle_key(ctx, key);
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ENTER) {
            net_tester_ip_input_enter_edit(ctx);
            lv_event_stop_processing(e);
        }
    }
    else if(code == LV_EVENT_FOCUSED) {
        net_tester_ip_input_refresh_focus(ctx, true);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        net_tester_ip_input_refresh_focus(ctx, false);
    }
}

bool net_tester_ip_input_set_value(net_tester_ip_input_t * ctx, const char * ip_str)
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    int n;

    if(!ctx || !ip_str) {
        return false;
    }

    n = sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    if(n != 4 || a > 255 || b > 255 || c > 255 || d > 255) {
        return false;
    }

    ctx->digits[0] = (uint8_t)(a / 100);
    ctx->digits[1] = (uint8_t)((a / 10) % 10);
    ctx->digits[2] = (uint8_t)(a % 10);
    ctx->digits[3] = (uint8_t)(b / 100);
    ctx->digits[4] = (uint8_t)((b / 10) % 10);
    ctx->digits[5] = (uint8_t)(b % 10);
    ctx->digits[6] = (uint8_t)(c / 100);
    ctx->digits[7] = (uint8_t)((c / 10) % 10);
    ctx->digits[8] = (uint8_t)(c % 10);
    ctx->digits[9] = (uint8_t)(d / 100);
    ctx->digits[10] = (uint8_t)((d / 10) % 10);
    ctx->digits[11] = (uint8_t)(d % 10);

    if(ctx->editing) {
        net_tester_ip_input_cancel_edit(ctx);
    }
    update_digit_labels(ctx);
    update_digit_style(ctx);
    return true;
}

lv_obj_t * net_tester_ip_input_create(lv_obj_t * parent, net_tester_ip_input_t * ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    /* 默认 192.168.001.100 */
    static const uint8_t def_digits[IP_DIGIT_COUNT] = {1,9,2, 1,6,8, 0,0,1, 1,0,0};
    memcpy(ctx->digits, def_digits, sizeof(ctx->digits));

    lv_obj_t * root = lv_obj_create(parent);
    ctx->root = root;
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, 200, 28);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x121820), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(root, lv_color_hex(0x243040), 0);
    lv_obj_set_style_border_width(root, 1, 0);
    lv_obj_set_style_radius(root, 3, 0);
    lv_obj_set_style_pad_all(root, 2, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(root, 0, 0);

    for(int i = 0; i < IP_DIGIT_COUNT; i++) {
        if(i == 3 || i == 6 || i == 9) {
            lv_obj_t * dot = lv_label_create(root);
            lv_label_set_text(dot, ".");
            lv_obj_set_style_text_font(dot, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(dot, lv_color_hex(CLR_DIM), 0);
            lv_obj_set_style_pad_hor(dot, 1, 0);
        }

        lv_obj_t * lbl = lv_label_create(root);
        ctx->digit_lbl[i] = lbl;
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_NORMAL), 0);
        lv_obj_set_style_pad_hor(lbl, 1, 0);
        lv_obj_set_style_radius(lbl, 2, 0);
        lv_label_set_text_fmt(lbl, "%d", ctx->digits[i]);
    }

    lv_obj_add_event_cb(root, ip_root_event_cb, LV_EVENT_ALL, ctx);

    ctx->blink_timer = lv_timer_create(blink_timer_cb, 400, ctx);

    return root;
}
