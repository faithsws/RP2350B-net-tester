#include "net_tester_ascii_input.h"

#include <stdio.h>
#include <string.h>

#define CLR_FOCUS       0x6EC8D8
#define CLR_NORMAL      0xC0C8D0
#define CLR_DIM         0x4A5A6A
#define CLR_CURSOR      0x44BB44
#define CLR_MODIFY_BG   0x2A2210
#define CLR_MODIFY_BD   0xCCAA00
#define CLR_MODIFY_CH   0xFFDD44
#define CLR_INSERT_BG   0x22102A
#define CLR_INSERT_BD   0xCC44AA
#define CLR_INSERT_CH   0x66EEFF

#define TAB_SHORT_MS    450

static void set_group_editing(net_tester_ascii_input_t * ctx, bool editing)
{
    if(!ctx || !ctx->root) return;
    lv_group_t * g = lv_obj_get_group(ctx->root);
    if(g) lv_group_set_editing(g, editing);
}

static char cycle_ascii(char c, int delta)
{
    int v = (unsigned char)c + delta;
    if(v < 32) v = 126;
    if(v > 126) v = 32;
    return (char)v;
}

static void update_slot_labels(net_tester_ascii_input_t * ctx)
{
    for(int i = 0; i < NET_ASCII_MAX_LEN; i++) {
        if(i < ctx->len) {
            lv_label_set_text_fmt(ctx->slot_lbl[i], "%c", ctx->text[i]);
        }
        else {
            lv_label_set_text(ctx->slot_lbl[i], " ");
        }
    }
}

static void apply_root_style(net_tester_ascii_input_t * ctx)
{
    if(!ctx || !ctx->root) return;

    switch(ctx->mode) {
        case NET_ASCII_MODE_MODIFY:
            lv_obj_set_style_bg_color(ctx->root, lv_color_hex(CLR_MODIFY_BG), 0);
            lv_obj_set_style_border_color(ctx->root, lv_color_hex(CLR_MODIFY_BD), 0);
            lv_obj_set_style_border_width(ctx->root, 2, 0);
            break;
        case NET_ASCII_MODE_INSERT:
            lv_obj_set_style_bg_color(ctx->root, lv_color_hex(CLR_INSERT_BG), 0);
            lv_obj_set_style_border_color(ctx->root, lv_color_hex(CLR_INSERT_BD), 0);
            lv_obj_set_style_border_width(ctx->root, 2, 0);
            break;
        case NET_ASCII_MODE_CURSOR:
            lv_obj_set_style_bg_color(ctx->root, lv_color_hex(0x121820), 0);
            lv_obj_set_style_border_color(ctx->root, lv_color_hex(CLR_CURSOR), 0);
            lv_obj_set_style_border_width(ctx->root, 2, 0);
            break;
        default:
            lv_obj_set_style_bg_color(ctx->root, lv_color_hex(0x121820), 0);
            lv_obj_set_style_border_color(ctx->root, lv_color_hex(0x243040), 0);
            lv_obj_set_style_border_width(ctx->root, 1, 0);
            break;
    }
}

static void update_slot_style(net_tester_ascii_input_t * ctx)
{
    for(int i = 0; i < NET_ASCII_MAX_LEN; i++) {
        lv_obj_t * lbl = ctx->slot_lbl[i];

        if(ctx->mode == NET_ASCII_MODE_CURSOR && i == (int)ctx->cursor) {
            if(ctx->blink_on) {
                lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_CURSOR), 0);
                lv_obj_set_style_bg_color(lbl, lv_color_hex(0x1A3020), 0);
                lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
            }
            else {
                lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_CURSOR), 0);
                lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
            }
        }
        else if(ctx->mode == NET_ASCII_MODE_MODIFY && i == (int)ctx->cursor && i < (int)ctx->len) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_MODIFY_CH), 0);
            lv_obj_set_style_bg_color(lbl, lv_color_hex(0x403010), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
        }
        else if(ctx->mode == NET_ASCII_MODE_INSERT && i == (int)ctx->cursor && i < (int)ctx->len) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_INSERT_CH), 0);
            lv_obj_set_style_bg_color(lbl, lv_color_hex(0x302040), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
        }
        else if(i < (int)ctx->len) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_NORMAL), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
        }
        else {
            lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_DIM), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
        }
    }

    apply_root_style(ctx);
}

static void refresh_view(net_tester_ascii_input_t * ctx)
{
    update_slot_labels(ctx);
    update_slot_style(ctx);
}

static void cancel_tab_timer(net_tester_ascii_input_t * ctx)
{
    if(ctx->tab_timer) {
        lv_timer_del(ctx->tab_timer);
        ctx->tab_timer = NULL;
    }
    ctx->tab_waiting = false;
}

static void enter_modify_mode(net_tester_ascii_input_t * ctx);
static void enter_insert_mode(net_tester_ascii_input_t * ctx);

static void tab_short_timer_cb(lv_timer_t * t)
{
    net_tester_ascii_input_t * ctx = (net_tester_ascii_input_t *)t->user_data;
    if(!ctx) return;
    ctx->tab_timer = NULL;
    ctx->tab_waiting = false;
    enter_modify_mode(ctx);
}

static void on_tab_key(net_tester_ascii_input_t * ctx)
{
    if(!ctx || ctx->mode != NET_ASCII_MODE_CURSOR) return;

    if(ctx->tab_waiting) {
        cancel_tab_timer(ctx);
        enter_insert_mode(ctx);
        return;
    }

    ctx->tab_waiting = true;
    ctx->tab_timer = lv_timer_create(tab_short_timer_cb, TAB_SHORT_MS, ctx);
    lv_timer_set_repeat_count(ctx->tab_timer, 1);
}

static void blink_timer_cb(lv_timer_t * t)
{
    net_tester_ascii_input_t * ctx = (net_tester_ascii_input_t *)t->user_data;
    if(!ctx || ctx->mode != NET_ASCII_MODE_CURSOR) return;
    ctx->blink_on = !ctx->blink_on;
    update_slot_style(ctx);
}

static void move_cursor(net_tester_ascii_input_t * ctx, int delta)
{
    int pos = (int)ctx->cursor + delta;
    if(pos < 0) pos = 0;
    if(pos > ctx->len) pos = ctx->len;
    ctx->cursor = (uint8_t)pos;
    ctx->blink_on = true;
    refresh_view(ctx);
}

static void delete_at_cursor(net_tester_ascii_input_t * ctx)
{
    if(ctx->len == 0) return;

    if(ctx->cursor >= ctx->len) {
        ctx->len--;
        ctx->text[ctx->len] = '\0';
        ctx->cursor = ctx->len;
    }
    else {
        memmove(&ctx->text[ctx->cursor], &ctx->text[ctx->cursor + 1], ctx->len - ctx->cursor);
        ctx->len--;
        ctx->text[ctx->len] = '\0';
    }

    refresh_view(ctx);
}

static void adjust_char_at_cursor(net_tester_ascii_input_t * ctx, int delta)
{
    if(ctx->cursor >= ctx->len) return;
    ctx->text[ctx->cursor] = cycle_ascii(ctx->text[ctx->cursor], delta);
    refresh_view(ctx);
}

static void enter_cursor_mode(net_tester_ascii_input_t * ctx)
{
    if(!ctx || ctx->mode != NET_ASCII_MODE_FOCUS) return;
    ctx->mode = NET_ASCII_MODE_CURSOR;
    ctx->cursor = ctx->len;
    ctx->blink_on = true;
    set_group_editing(ctx, true);
    refresh_view(ctx);
}

static void exit_to_focus_mode(net_tester_ascii_input_t * ctx)
{
    if(!ctx || ctx->mode != NET_ASCII_MODE_CURSOR) return;

    cancel_tab_timer(ctx);
    ctx->mode = NET_ASCII_MODE_FOCUS;
    set_group_editing(ctx, false);

    rt_kprintf("[ASCII] %s\n", ctx->text);

    refresh_view(ctx);

    lv_group_t * g = lv_obj_get_group(ctx->root);
    if(g && lv_group_get_focused(g) == ctx->root) {
        net_tester_ascii_input_refresh_focus(ctx, true);
    }
}

static void enter_modify_mode(net_tester_ascii_input_t * ctx)
{
    if(!ctx || ctx->mode != NET_ASCII_MODE_CURSOR) return;
    if(ctx->len == 0) return;

    cancel_tab_timer(ctx);
    if(ctx->cursor >= ctx->len) {
        ctx->cursor = (uint8_t)(ctx->len - 1);
    }

    ctx->mode = NET_ASCII_MODE_MODIFY;
    refresh_view(ctx);
}

static void enter_insert_mode(net_tester_ascii_input_t * ctx)
{
    if(!ctx || ctx->mode != NET_ASCII_MODE_CURSOR) return;
    if(ctx->len >= NET_ASCII_MAX_LEN) return;

    cancel_tab_timer(ctx);
    memmove(&ctx->text[ctx->cursor + 1], &ctx->text[ctx->cursor], ctx->len - ctx->cursor);
    ctx->text[ctx->cursor] = 'A';
    ctx->len++;
    ctx->text[ctx->len] = '\0';

    ctx->mode = NET_ASCII_MODE_INSERT;
    refresh_view(ctx);
}

static void insert_char_after_cursor(net_tester_ascii_input_t * ctx)
{
    if(!ctx || ctx->len >= NET_ASCII_MAX_LEN) return;

    uint8_t pos = (uint8_t)(ctx->cursor + 1);
    memmove(&ctx->text[pos + 1], &ctx->text[pos], ctx->len - pos);
    ctx->text[pos] = 'A';
    ctx->len++;
    ctx->text[ctx->len] = '\0';
    ctx->cursor = pos;
    refresh_view(ctx);
}

static void move_edit_char_next(net_tester_ascii_input_t * ctx)
{
    if(ctx->cursor + 1 < ctx->len) {
        ctx->cursor++;
        refresh_view(ctx);
        return;
    }

    /* 修改模式：Tab 到末尾时追加字符并继续修改 */
    if(ctx->mode == NET_ASCII_MODE_MODIFY && ctx->len < NET_ASCII_MAX_LEN) {
        ctx->text[ctx->len] = 'A';
        ctx->len++;
        ctx->text[ctx->len] = '\0';
        ctx->cursor = (uint8_t)(ctx->len - 1);
        refresh_view(ctx);
    }
}

static void back_to_cursor_mode(net_tester_ascii_input_t * ctx)
{
    if(!ctx) return;
    if(ctx->mode != NET_ASCII_MODE_MODIFY && ctx->mode != NET_ASCII_MODE_INSERT) return;

    ctx->mode = NET_ASCII_MODE_CURSOR;
    ctx->blink_on = true;
    if(ctx->cursor > ctx->len) ctx->cursor = ctx->len;
    refresh_view(ctx);
}

static void handle_key(net_tester_ascii_input_t * ctx, uint32_t key)
{
    if(!ctx) return;

    switch(ctx->mode) {
        case NET_ASCII_MODE_CURSOR:
            if(key == LV_KEY_ENTER) {
                exit_to_focus_mode(ctx);
            }
            else if(key == LV_KEY_ESC) {
                delete_at_cursor(ctx);
            }
            else if(key == NET_TESTER_ASCII_KEY_TAB || key == LV_KEY_NEXT) {
                on_tab_key(ctx);
            }
            else if(key == LV_KEY_LEFT) {
                move_cursor(ctx, -1);
            }
            else if(key == LV_KEY_RIGHT) {
                move_cursor(ctx, 1);
            }
            break;

        case NET_ASCII_MODE_MODIFY:
            if(key == LV_KEY_ESC) {
                back_to_cursor_mode(ctx);
            }
            else if(key == NET_TESTER_ASCII_KEY_TAB_NEXT || key == LV_KEY_NEXT) {
                move_edit_char_next(ctx);
            }
            else if(key == LV_KEY_LEFT) {
                adjust_char_at_cursor(ctx, -1);
            }
            else if(key == LV_KEY_RIGHT) {
                adjust_char_at_cursor(ctx, 1);
            }
            break;

        case NET_ASCII_MODE_INSERT:
            if(key == LV_KEY_ESC) {
                back_to_cursor_mode(ctx);
            }
            else if(key == NET_TESTER_ASCII_KEY_TAB_NEXT || key == LV_KEY_NEXT) {
                insert_char_after_cursor(ctx);
            }
            else if(key == LV_KEY_LEFT) {
                adjust_char_at_cursor(ctx, -1);
            }
            else if(key == LV_KEY_RIGHT) {
                adjust_char_at_cursor(ctx, 1);
            }
            break;

        default:
            break;
    }
}

bool net_tester_ascii_input_is_active(const net_tester_ascii_input_t * ctx)
{
    return ctx && ctx->mode != NET_ASCII_MODE_FOCUS;
}

bool net_tester_ascii_input_wants_tab_key(const net_tester_ascii_input_t * ctx)
{
    return ctx && ctx->mode == NET_ASCII_MODE_CURSOR;
}

bool net_tester_ascii_input_wants_tab_next(const net_tester_ascii_input_t * ctx)
{
    return ctx && (ctx->mode == NET_ASCII_MODE_MODIFY || ctx->mode == NET_ASCII_MODE_INSERT);
}

void net_tester_ascii_input_cancel_active(net_tester_ascii_input_t * ctx)
{
    if(!ctx || !net_tester_ascii_input_is_active(ctx)) return;

    cancel_tab_timer(ctx);
    ctx->mode = NET_ASCII_MODE_FOCUS;
    set_group_editing(ctx, false);
    refresh_view(ctx);
}

void net_tester_ascii_input_handle_esc(net_tester_ascii_input_t * ctx)
{
    if(!ctx || !net_tester_ascii_input_is_active(ctx)) return;

    if(ctx->mode == NET_ASCII_MODE_MODIFY || ctx->mode == NET_ASCII_MODE_INSERT) {
        back_to_cursor_mode(ctx);
    }
    else if(ctx->mode == NET_ASCII_MODE_CURSOR) {
        delete_at_cursor(ctx);
    }
}

void net_tester_ascii_input_refresh_focus(net_tester_ascii_input_t * ctx, bool focused)
{
    if(!ctx || !ctx->root) return;
    if(net_tester_ascii_input_is_active(ctx)) return;

    if(focused) {
        lv_obj_set_style_border_color(ctx->root, lv_color_hex(CLR_FOCUS), 0);
        lv_obj_set_style_border_width(ctx->root, 2, 0);
    }
    else {
        lv_obj_set_style_border_color(ctx->root, lv_color_hex(0x243040), 0);
        lv_obj_set_style_border_width(ctx->root, 1, 0);
    }
}

const char * net_tester_ascii_input_get_value(const net_tester_ascii_input_t * ctx)
{
    if(!ctx) return "";
    return ctx->text;
}

static void ascii_root_event_cb(lv_event_t * e)
{
    net_tester_ascii_input_t * ctx = (net_tester_ascii_input_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(net_tester_ascii_input_is_active(ctx)) {
            handle_key(ctx, key);
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ENTER) {
            enter_cursor_mode(ctx);
            lv_event_stop_processing(e);
        }
    }
    else if(code == LV_EVENT_FOCUSED) {
        net_tester_ascii_input_refresh_focus(ctx, true);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        net_tester_ascii_input_refresh_focus(ctx, false);
    }
}

void net_tester_ascii_input_set_value(net_tester_ascii_input_t * ctx, const char * text)
{
    size_t n;

    if(!ctx) {
        return;
    }

    if(net_tester_ascii_input_is_active(ctx)) {
        net_tester_ascii_input_cancel_active(ctx);
    }

    memset(ctx->text, 0, sizeof(ctx->text));
    if(text) {
        n = strlen(text);
        if(n > NET_ASCII_MAX_LEN) {
            n = NET_ASCII_MAX_LEN;
        }
        memcpy(ctx->text, text, n);
        ctx->len = (uint8_t)n;
    }
    else {
        ctx->len = 0;
    }
    ctx->cursor = 0;
    update_slot_labels(ctx);
    update_slot_style(ctx);
    apply_root_style(ctx);
}

lv_obj_t * net_tester_ascii_input_create(lv_obj_t * parent, net_tester_ascii_input_t * ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->text, "hello", sizeof(ctx->text) - 1);
    ctx->len = (uint8_t)strlen(ctx->text);
    ctx->mode = NET_ASCII_MODE_FOCUS;

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
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(root, 0, 0);

    for(int i = 0; i < NET_ASCII_MAX_LEN; i++) {
        lv_obj_t * lbl = lv_label_create(root);
        ctx->slot_lbl[i] = lbl;
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_pad_hor(lbl, 1, 0);
        lv_obj_set_style_radius(lbl, 2, 0);
    }

    lv_obj_add_event_cb(root, ascii_root_event_cb, LV_EVENT_ALL, ctx);
    ctx->blink_timer = lv_timer_create(blink_timer_cb, 400, ctx);

    refresh_view(ctx);
    return root;
}
