#include "net_tester_ui.h"
#include <rtthread.h>
#include <stdbool.h>

#include "hzk12_font.h"
#include "lv_port_indev.h"
#include "power_status.h"

LV_FONT_DECLARE(lv_font_montserrat_14)
LV_FONT_DECLARE(lv_font_montserrat_20)
#include "net_tester_trace.h"
#include "net_tester_pair.h"
#include "net_tester_crimp.h"
#include "net_tester_press.h"
#include "net_tester_about.h"
#include "net_tester_netdbg.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* 配色与 HTML 预览一致 */
#define UI_BG          0x0E1218
#define UI_PANEL       0x121820
#define UI_PANEL_ON    0x161E28
#define UI_BORDER      0x243040
#define UI_TITLE       0x6EC8D8
#define UI_FOOTER      0x4A5A6A

#define CLR_TRACE      0x00B4CC
#define CLR_CRIMP      0xCC44AA  /* 测序 */
#define CLR_PAIR       0xCCAA00
#define CLR_PRESS      0xE07040  /* 压接 */
#define CLR_DEBUG      0x44BB44
#define CLR_ABOUT      0x8899BB  /* 关于 */

/* 电池图标：高对比，深色顶栏上更易辨认 */
#define BAT_CLR_OK     0x2EE66A
#define BAT_CLR_LOW    0xFF9A2E
#define BAT_CLR_FRAME  0x8AA0B4

#define SCR_W          240
#define SCR_H          240
#define HDR_H          32   /* 适配 24 点阵标题 */
#define FTR_H          0    /* 无操作提示页脚 */
#define GRID_PAD_H     6
#define GRID_PAD_V     4
#define GRID_GAP       4
#define MENU_COUNT     6
#define MENU_COLS      2
#define MENU_ROWS      3
/* 文字区高度固定，保证图标/文字比例与原先接近 */
#define MENU_TEXT_H    28

#define PRESS_OK_COLOR   0x44BB44
#define PRESS_BAD_COLOR  0x4A5A6A

/* 电池图标尺寸（≤64x32） */
#define BAT_ICON_W     40
#define BAT_ICON_H     16
#define BAT_BODY_W     32
#define BAT_BODY_H     12
#define BAT_TIP_W      3
#define BAT_TIP_H      6
#define BAT_POLL_MS    2000

/* 界面主中文 24x24；示意图密排文字仍用 12 */
#define UI_CN_FONT     hzk24_font_get()
#define UI_ICON_FONT   (&lv_font_montserrat_20)

typedef enum {
    NET_FUNC_TRACE = 0,
    NET_FUNC_CRIMP,   /* 测序 */
    NET_FUNC_PAIR,
    NET_FUNC_PRESS,   /* 压接 */
    NET_FUNC_DEBUG,
    NET_FUNC_ABOUT
} net_func_id_t;

typedef struct {
    const char * name;
    const char * icon_sym;
    uint32_t color;
    lv_obj_t * tile;
    lv_obj_t * icon_lbl;
} menu_item_t;

static lv_obj_t * main_screen;
static lv_obj_t * sub_screens[MENU_COUNT];
static lv_group_t * main_group;
static lv_group_t * sub_group;
static lv_indev_t * keypad_dev;
static lv_indev_t * encoder_dev;
static menu_item_t menu_items[MENU_COUNT];

static lv_obj_t * trace_anim_box;
static lv_obj_t * trace_anim_icon;
static lv_obj_t * trace_anim_dots[3];
static lv_obj_t * trace_start_btn;
static lv_obj_t * trace_start_lbl;
static lv_timer_t * trace_anim_timer;
static uint8_t trace_anim_step;
static lv_obj_t * trace_key_catcher;

/* 对线界面 */
static lv_obj_t * pair_diagram;
static lv_obj_t * pair_key_catcher;
static lv_obj_t * pair_start_btn;
static lv_obj_t * pair_start_lbl;
static pair_scan_result_t pair_result;
static bool pair_has_result;

/* 对线状态色：断路灰 / 联通绿 / 短路红 */
#define PAIR_ST_OPEN_COLOR   0x8A9099
#define PAIR_ST_CONN_COLOR   0x2EBB55
#define PAIR_ST_SHORT_COLOR  0xE04545

/* 测序界面 */
static lv_obj_t * crimp_diagram;
static lv_obj_t * crimp_key_catcher;
static lv_obj_t * crimp_start_btn;
static lv_obj_t * crimp_start_lbl;
static seq_scan_result_t seq_result;
static bool crimp_has_result;

/* 压接界面 */
static lv_obj_t * press_diagram;
static lv_obj_t * press_key_catcher;
static lv_obj_t * press_start_btn;
static lv_obj_t * press_start_lbl;
static uint8_t press_status;
static bool press_has_result;

/* T568B 线序配色：1白橙 2橙 3白绿 4蓝 5白蓝 6绿 7白棕 8棕 */
static const uint32_t PAIR_WIRE_COLOR[8] = {
    0xE8D0A0, 0xE09020, 0xA0E8A0, 0x4080E0,
    0xA0C8F0, 0x30A030, 0xE8C898, 0xA06020
};

/* 顶栏右上角供电状态图标（浮层，跨页面常驻） */
static lv_obj_t * bat_cont;
static lv_obj_t * bat_body;
static lv_obj_t * bat_tip;
static lv_obj_t * bat_fill;
static lv_obj_t * bat_bolt;
static lv_timer_t * bat_timer;

static void style_tile(menu_item_t * item, bool focused);
static void open_sub_screen(net_func_id_t id);
static void back_to_main(void);
static lv_obj_t * create_header(lv_obj_t * parent, const char * text);
static lv_obj_t * create_header_ex(lv_obj_t * parent, const char * text, const lv_font_t * font);
static lv_obj_t * create_sub_screen(net_func_id_t id, const char * title, uint32_t color, const char * icon_sym);
static lv_obj_t * create_trace_screen(void);
static lv_obj_t * create_pair_screen(void);
static lv_obj_t * create_crimp_screen(void);
static lv_obj_t * create_press_screen(void);
static lv_obj_t * create_debug_screen(void);
static lv_obj_t * create_about_screen(void);
static void bind_sub_group(lv_obj_t * scr);
static void bat_icon_create(void);
static void bat_icon_refresh(void);
static void bat_icon_timer_cb(lv_timer_t * t);
static void trace_ui_reset_state(void);
static void pair_ui_reset_state(void);
static void crimp_ui_reset_state(void);
static void press_ui_reset_state(void);

static void style_screen_bg(lv_obj_t * scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_tile(menu_item_t * item, bool focused)
{
    if(!item || !item->tile) return;

    lv_obj_t * tile = item->tile;
    if(focused) {
        lv_obj_set_style_bg_color(tile, lv_color_hex(UI_PANEL_ON), 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(item->color), 0);
        lv_obj_set_style_border_width(tile, 2, 0);
    }
    else {
        lv_obj_set_style_bg_color(tile, lv_color_hex(UI_PANEL), 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(UI_BORDER), 0);
        lv_obj_set_style_border_width(tile, 1, 0);
    }
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_border_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tile, 3, 0);
    lv_obj_set_style_shadow_width(tile, 0, 0);
}

static void refresh_all_tiles(void)
{
    lv_obj_t * focused = lv_group_get_focused(main_group);
    for(int i = 0; i < MENU_COUNT; i++) {
        style_tile(&menu_items[i], menu_items[i].tile == focused);
    }
}

static void group_focus_cb(lv_group_t * g)
{
    LV_UNUSED(g);
    refresh_all_tiles();
}

static void tile_click_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        open_sub_screen((net_func_id_t)(uintptr_t)lv_event_get_user_data(e));
    }
}

static void tile_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ENTER) {
        open_sub_screen((net_func_id_t)(uintptr_t)lv_event_get_user_data(e));
        lv_event_stop_processing(e);
    }
}

static void sub_esc_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) != LV_KEY_ESC) return;

    /* 网络调试：功能页 ESC 回二级菜单；菜单 ESC 再回主屏 */
    if(net_tester_netdbg_is_on_screen()) {
        if(net_tester_netdbg_handle_esc()) {
            lv_event_stop_processing(e);
            return;
        }
    }

    if(lv_scr_act() == sub_screens[NET_FUNC_TRACE]) {
        if(net_tester_trace_is_running()) {
            net_tester_trace_stop();
        }
        trace_ui_reset_state();
    }

    if(lv_scr_act() == sub_screens[NET_FUNC_PAIR]) {
        pair_ui_reset_state();
    }

    if(lv_scr_act() == sub_screens[NET_FUNC_CRIMP]) {
        crimp_ui_reset_state();
    }

    if(lv_scr_act() == sub_screens[NET_FUNC_PRESS]) {
        press_ui_reset_state();
    }

    back_to_main();
}

static void trace_btn_focus_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    if(lv_event_get_code(e) == LV_EVENT_FOCUSED) {
        lv_obj_set_style_border_color(btn, lv_color_hex(UI_TITLE), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
    }
    else if(lv_event_get_code(e) == LV_EVENT_DEFOCUSED) {
        lv_obj_set_style_border_color(btn, lv_color_hex(UI_BORDER), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
    }
}

static void trace_anim_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    if(!trace_anim_box || lv_obj_has_flag(trace_anim_box, LV_OBJ_FLAG_HIDDEN)) return;

    trace_anim_step = (uint8_t)((trace_anim_step + 1) % 3);
    for(int i = 0; i < 3; i++) {
        if(!trace_anim_dots[i]) continue;
        if(i == trace_anim_step) {
            lv_obj_set_style_text_color(trace_anim_dots[i], lv_color_hex(CLR_TRACE), 0);
            lv_obj_set_style_text_opa(trace_anim_dots[i], LV_OPA_COVER, 0);
        }
        else {
            lv_obj_set_style_text_color(trace_anim_dots[i], lv_color_hex(UI_FOOTER), 0);
            lv_obj_set_style_text_opa(trace_anim_dots[i], LV_OPA_40, 0);
        }
    }
}

static void trace_ui_reset_state(void)
{
    if(!trace_anim_box || !trace_start_lbl) return;

    lv_obj_add_flag(trace_anim_box, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(trace_start_lbl, "开始");
}

static void trace_ui_set_searching(bool searching)
{
    if(!trace_anim_box || !trace_start_lbl) return;

    if(searching) {
        lv_obj_clear_flag(trace_anim_box, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(trace_start_lbl, "寻线中");
        trace_anim_step = 0;
        trace_anim_timer_cb(NULL);
    }
    else {
        trace_ui_reset_state();
    }
}

static void trace_toggle_start(void)
{
    if(net_tester_trace_is_running()) {
        net_tester_trace_stop();
        trace_ui_set_searching(false);
    }
    else {
        net_tester_trace_start();
        trace_ui_set_searching(true);
    }
}

static void trace_apply_focus_style(bool focused)
{
    if(!trace_start_btn) return;
    if(focused) {
        lv_obj_set_style_border_color(trace_start_btn, lv_color_hex(UI_TITLE), 0);
        lv_obj_set_style_border_width(trace_start_btn, 2, 0);
    }
    else {
        lv_obj_set_style_border_color(trace_start_btn, lv_color_hex(UI_BORDER), 0);
        lv_obj_set_style_border_width(trace_start_btn, 1, 0);
    }
}

#define TRACE_FOCUS_DELAY_MS  80

static void trace_focus_start_timer_cb(lv_timer_t * t)
{
    if(trace_start_btn && sub_group &&
       lv_scr_act() == sub_screens[NET_FUNC_TRACE]) {
        lv_group_focus_obj(trace_start_btn);
    }
    lv_timer_del(t);
}

static void trace_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ESC) {
        sub_esc_key_cb(e);
    }
}

static bool indev_is_keypad(void)
{
    lv_indev_t * indev = lv_indev_get_act();
    return indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD;
}

static void trace_start_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        trace_btn_focus_cb(e);
        return;
    }

    if(code == LV_EVENT_CLICKED) {
        /* 键盘 Enter 松开时还会再发 CLICKED，忽略以免开始后又立刻停止 */
        if(indev_is_keypad()) return;
        trace_toggle_start();
    }
    else if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ENTER) {
            trace_toggle_start();
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ESC) {
            sub_esc_key_cb(e);
        }
    }
}

static void sub_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ESC) {
        back_to_main();
    }
}

static uint32_t pair_ch_color(uint8_t st)
{
    if(st == PAIR_CH_CONN) {
        return PAIR_ST_CONN_COLOR;
    }
    if(st == PAIR_CH_SHORT) {
        return PAIR_ST_SHORT_COLOR;
    }
    return PAIR_ST_OPEN_COLOR;
}

static void pair_ui_reset_state(void)
{
    net_tester_pair_stop();
    memset(&pair_result, 0, sizeof(pair_result));
    pair_has_result = false;
    if(pair_start_lbl) {
        lv_label_set_text(pair_start_lbl, "开始");
    }
    if(pair_diagram) {
        lv_obj_invalidate(pair_diagram);
    }
}

static void pair_ui_set_waiting(void)
{
    memset(&pair_result, 0, sizeof(pair_result));
    pair_has_result = false;
    if(pair_start_lbl) {
        lv_label_set_text(pair_start_lbl, "测试中");
    }
    if(pair_diagram) {
        lv_obj_invalidate(pair_diagram);
    }
}

static void pair_ui_set_result(const pair_scan_result_t * result)
{
    if(result) {
        memcpy(&pair_result, result, sizeof(pair_result));
    }
    else {
        memset(&pair_result, 0, sizeof(pair_result));
    }
    pair_has_result = true;
    if(pair_start_lbl) {
        lv_label_set_text(pair_start_lbl, "开始");
    }
    if(pair_diagram) {
        lv_obj_invalidate(pair_diagram);
    }
}

static void pair_result_cb(const pair_scan_result_t * result, void * user_data)
{
    LV_UNUSED(user_data);
    pair_ui_set_result(result);
}

static void pair_toggle_start(void)
{
    /* 点击开始 → pair_scan；成功后再进入等待 UI */
    if(net_tester_pair_start()) {
        pair_ui_set_waiting();
    }
}

static void pair_apply_focus_style(bool focused)
{
    if(!pair_start_btn) return;
    if(focused) {
        lv_obj_set_style_border_color(pair_start_btn, lv_color_hex(UI_TITLE), 0);
        lv_obj_set_style_border_width(pair_start_btn, 2, 0);
    }
    else {
        lv_obj_set_style_border_color(pair_start_btn, lv_color_hex(UI_BORDER), 0);
        lv_obj_set_style_border_width(pair_start_btn, 1, 0);
    }
}

static void pair_btn_focus_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED) {
        pair_apply_focus_style(true);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        pair_apply_focus_style(false);
    }
}

static void pair_handle_space(void)
{
    if(!net_tester_pair_is_waiting()) {
        rt_kprintf("[PAIR] 空格无效: 请先点击开始\n");
        
        return;
    }
    net_tester_pair_simulate_async_done();
}

static void pair_diagram_draw_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;

    lv_obj_t * obj = lv_event_get_target(e);
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    /* 横向布局：上方画线，底部圆点+标号 */
    const lv_coord_t w = lv_area_get_width(&coords);
    const lv_coord_t h = lv_area_get_height(&coords);
    const lv_coord_t pin_r = 5;
    const lv_coord_t margin_x = 18;
    const lv_coord_t pitch = (w - margin_x * 2) / 7;
    const lv_coord_t pin_y = coords.y2 - 28;          /* 圆点行 */
    const lv_coord_t num_y = coords.y2 - 14;          /* 标号行 */
    const lv_coord_t line_base = pin_y - pin_r - 2;   /* 连线起点在圆点上方 */

    lv_draw_label_dsc_t title_dsc;
    lv_draw_label_dsc_init(&title_dsc);
    title_dsc.font = hzk12_font_get();
    title_dsc.color = lv_color_hex(CLR_PAIR);
    title_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t title = {coords.x1 + 10, coords.y1 + 2, coords.x2 - 10, coords.y1 + 16};
    lv_draw_label(draw_ctx, &title_dsc, &title, "通道状态", NULL);

    lv_coord_t pin_xs[PAIR_CH_COUNT];
    for(int i = 0; i < PAIR_CH_COUNT; i++) {
        pin_xs[i] = coords.x1 + margin_x + i * pitch;
    }

    if(pair_has_result) {
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.width = 2;
        line_dsc.opa = LV_OPA_COVER;
        line_dsc.round_start = 1;
        line_dsc.round_end = 1;

        /* 联通：自圆点向上的绿色竖线 */
        line_dsc.color = lv_color_hex(PAIR_ST_CONN_COLOR);
        for(int i = 0; i < PAIR_CH_COUNT; i++) {
            if(pair_result.ch_state[i] != PAIR_CH_CONN) {
                continue;
            }
            lv_point_t p1 = {pin_xs[i], line_base};
            lv_point_t p2 = {pin_xs[i], (lv_coord_t)(line_base - 22)};
            lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);
        }

        /* 短路：圆点上方拱形折线（跨度越大拱越高） */
        line_dsc.color = lv_color_hex(PAIR_ST_SHORT_COLOR);
        for(int i = 0; i < PAIR_CH_COUNT; i++) {
            for(int j = i + 1; j < PAIR_CH_COUNT; j++) {
                lv_coord_t arch_h;
                lv_coord_t mid_y;
                lv_point_t p1, pm, p2;

                if(!(pair_result.short_bits[i] & (1u << j))) {
                    continue;
                }
                arch_h = 18 + (lv_coord_t)((j - i) * 8);
                if(arch_h > h - 50) {
                    arch_h = (lv_coord_t)(h - 50);
                }
                if(arch_h < 18) {
                    arch_h = 18;
                }
                mid_y = line_base - arch_h;
                p1.x = pin_xs[i];
                p1.y = line_base;
                pm.x = (lv_coord_t)((pin_xs[i] + pin_xs[j]) / 2);
                pm.y = mid_y;
                p2.x = pin_xs[j];
                p2.y = line_base;
                lv_draw_line(draw_ctx, &line_dsc, &p1, &pm);
                lv_draw_line(draw_ctx, &line_dsc, &pm, &p2);
            }
        }
    }

    lv_draw_rect_dsc_t pin_dsc;
    lv_draw_rect_dsc_init(&pin_dsc);
    pin_dsc.radius = LV_RADIUS_CIRCLE;
    pin_dsc.bg_opa = LV_OPA_COVER;
    pin_dsc.border_width = 1;
    pin_dsc.border_color = lv_color_hex(0x203040);
    pin_dsc.border_opa = LV_OPA_COVER;

    lv_draw_label_dsc_t num_dsc;
    lv_draw_label_dsc_init(&num_dsc);
    num_dsc.font = &lv_font_montserrat_14;
    num_dsc.color = lv_color_hex(UI_FOOTER);
    num_dsc.align = LV_TEXT_ALIGN_CENTER;

    for(int i = 0; i < PAIR_CH_COUNT; i++) {
        lv_coord_t x = pin_xs[i];

        if(pair_has_result) {
            pin_dsc.bg_color = lv_color_hex(pair_ch_color(pair_result.ch_state[i]));
        }
        else {
            pin_dsc.bg_color = lv_color_hex(PAIR_WIRE_COLOR[i]);
        }
        lv_area_t pin = {x - pin_r, pin_y - pin_r, x + pin_r, pin_y + pin_r};
        lv_draw_rect(draw_ctx, &pin_dsc, &pin);

        char num[2] = {(char)('1' + i), '\0'};
        lv_area_t n_area = {x - 8, num_y, x + 8, coords.y2 - 2};
        lv_draw_label(draw_ctx, &num_dsc, &n_area, num, NULL);
    }
}

static void pair_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if(key == LV_KEY_ESC) {
        pair_ui_reset_state();
        back_to_main();
        lv_event_stop_processing(e);
    }
    else if(key == ' ' || key == LV_KEY_NEXT) {
        /* 板端无空格键：EC11 键(NEXT)在等待中模拟异步完成 */
        pair_handle_space();
        lv_event_stop_processing(e);
    }
    else if(key == LV_KEY_ENTER) {
        pair_toggle_start();
        lv_event_stop_processing(e);
    }
}

static void pair_start_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        pair_btn_focus_cb(e);
        return;
    }

    if(code == LV_EVENT_CLICKED) {
        pair_toggle_start();
        return;
    }

    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ENTER) {
            pair_toggle_start();
            lv_event_stop_processing(e);
        }
        else if(key == ' ' || key == LV_KEY_NEXT) {
            pair_handle_space();
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ESC) {
            pair_ui_reset_state();
            back_to_main();
            lv_event_stop_processing(e);
        }
    }
}

static lv_obj_t * create_pair_screen(void)
{
    const lv_font_t * font = UI_CN_FONT;
    lv_obj_t * scr = lv_obj_create(NULL);
    style_screen_bg(scr);

    create_header(scr, "对线");

    lv_obj_t * body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, SCR_W, SCR_H - HDR_H - FTR_H);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* 画布：为底部开始按钮预留高度 */
    const int32_t btn_h = 36;
    const int32_t btn_gap = 6;
    pair_diagram = lv_obj_create(body);
    lv_obj_remove_style_all(pair_diagram);
    lv_obj_set_size(pair_diagram, SCR_W - 8, SCR_H - HDR_H - FTR_H - btn_h - btn_gap - 8);
    lv_obj_align(pair_diagram, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(pair_diagram, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(pair_diagram, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pair_diagram, 3, 0);
    lv_obj_set_style_border_color(pair_diagram, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(pair_diagram, 1, 0);
    lv_obj_clear_flag(pair_diagram, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pair_diagram, pair_diagram_draw_cb, LV_EVENT_DRAW_MAIN, NULL);

    pair_start_btn = lv_btn_create(body);
    lv_obj_remove_style_all(pair_start_btn);
    lv_obj_set_size(pair_start_btn, 80, btn_h);
    lv_obj_align(pair_start_btn, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(pair_start_btn, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(pair_start_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(pair_start_btn, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(pair_start_btn, 1, 0);
    lv_obj_set_style_radius(pair_start_btn, 3, 0);
    lv_obj_add_event_cb(pair_start_btn, pair_start_btn_event_cb, LV_EVENT_ALL, NULL);

    pair_start_lbl = lv_label_create(pair_start_btn);
    lv_label_set_text(pair_start_lbl, "开始");
    lv_obj_set_style_text_font(pair_start_lbl, font, 0);
    lv_obj_set_style_text_color(pair_start_lbl, lv_color_hex(CLR_PAIR), 0);
    lv_obj_center(pair_start_lbl);

    pair_key_catcher = lv_obj_create(scr);
    lv_obj_remove_style_all(pair_key_catcher);
    lv_obj_set_size(pair_key_catcher, SCR_W, SCR_H);
    lv_obj_add_flag(pair_key_catcher, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(pair_key_catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(pair_key_catcher, pair_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(scr, pair_key_catcher);
    pair_apply_focus_style(false);
    pair_ui_reset_state();
    return scr;
}

static void crimp_ui_reset_state(void)
{
    net_tester_crimp_stop();
    memset(&seq_result, 0, sizeof(seq_result));
    for(int i = 0; i < PAIR_CH_COUNT; i++) {
        seq_result.map_to[i] = SEQ_MAP_NONE;
    }
    crimp_has_result = false;
    if(crimp_start_lbl) {
        lv_label_set_text(crimp_start_lbl, "开始");
    }
    if(crimp_diagram) {
        lv_obj_invalidate(crimp_diagram);
    }
}

static void crimp_ui_set_waiting(void)
{
    memset(&seq_result, 0, sizeof(seq_result));
    for(int i = 0; i < PAIR_CH_COUNT; i++) {
        seq_result.map_to[i] = SEQ_MAP_NONE;
    }
    crimp_has_result = false;
    if(crimp_start_lbl) {
        lv_label_set_text(crimp_start_lbl, "测序中");
    }
    if(crimp_diagram) {
        lv_obj_invalidate(crimp_diagram);
    }
}

static void crimp_ui_set_result(const seq_scan_result_t * result)
{
    if(result) {
        memcpy(&seq_result, result, sizeof(seq_result));
    }
    else {
        memset(&seq_result, 0, sizeof(seq_result));
        for(int i = 0; i < PAIR_CH_COUNT; i++) {
            seq_result.map_to[i] = SEQ_MAP_NONE;
        }
    }
    crimp_has_result = true;
    if(crimp_start_lbl) {
        lv_label_set_text(crimp_start_lbl, "开始");
    }
    if(crimp_diagram) {
        lv_obj_invalidate(crimp_diagram);
    }
}

static void crimp_result_cb(const seq_scan_result_t * result, void * user_data)
{
    LV_UNUSED(user_data);
    crimp_ui_set_result(result);
}

static void crimp_toggle_start(void)
{
    if(net_tester_crimp_start()) {
        crimp_ui_set_waiting();
    }
}

static void crimp_apply_focus_style(bool focused)
{
    if(!crimp_start_btn) return;
    if(focused) {
        lv_obj_set_style_border_color(crimp_start_btn, lv_color_hex(UI_TITLE), 0);
        lv_obj_set_style_border_width(crimp_start_btn, 2, 0);
    }
    else {
        lv_obj_set_style_border_color(crimp_start_btn, lv_color_hex(UI_BORDER), 0);
        lv_obj_set_style_border_width(crimp_start_btn, 1, 0);
    }
}

static void crimp_btn_focus_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED) {
        crimp_apply_focus_style(true);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        crimp_apply_focus_style(false);
    }
}

static void crimp_handle_space(void)
{
    if(!net_tester_crimp_is_waiting()) {
        rt_kprintf("[CRIMP] 空格无效: 请先点击开始\n");
        
        return;
    }
    net_tester_crimp_simulate_async_done();
}

static void crimp_diagram_draw_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;

    lv_obj_t * obj = lv_event_get_target(e);
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    const lv_coord_t left_x = coords.x1 + 28;
    const lv_coord_t right_x = coords.x2 - 28;
    const lv_coord_t top_y = coords.y1 + 22;
    const lv_coord_t h = lv_area_get_height(&coords);
    const lv_coord_t pitch = (h - 30) / 7;
    const lv_coord_t pin_r = 4;

    lv_draw_label_dsc_t title_dsc;
    lv_draw_label_dsc_init(&title_dsc);
    title_dsc.font = hzk12_font_get();
    title_dsc.color = lv_color_hex(CLR_CRIMP);
    title_dsc.align = LV_TEXT_ALIGN_CENTER;

    lv_area_t a_title = {coords.x1 + 4, coords.y1 + 2, coords.x1 + 60, coords.y1 + 16};
    lv_draw_label(draw_ctx, &title_dsc, &a_title, "本端", NULL);

    lv_area_t b_title = {coords.x2 - 60, coords.y1 + 2, coords.x2 - 4, coords.y1 + 16};
    lv_draw_label(draw_ctx, &title_dsc, &b_title, "对端", NULL);

    if(crimp_has_result) {
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.width = 2;
        line_dsc.opa = LV_OPA_COVER;
        line_dsc.round_start = 1;
        line_dsc.round_end = 1;

        for(int i = 0; i < PAIR_CH_COUNT; i++) {
            int8_t mid = seq_result.map_to[i];
            if(mid < 0 || mid >= PAIR_CH_COUNT) {
                continue;
            }
            /* 直通绿，错序黄 */
            line_dsc.color = lv_color_hex(mid == i ? PAIR_ST_CONN_COLOR : 0xE0A020);
            lv_point_t p1 = {left_x, (lv_coord_t)(top_y + i * pitch)};
            lv_point_t p2 = {right_x, (lv_coord_t)(top_y + mid * pitch)};
            lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);
        }
    }

    lv_draw_rect_dsc_t pin_dsc;
    lv_draw_rect_dsc_init(&pin_dsc);
    pin_dsc.radius = LV_RADIUS_CIRCLE;
    pin_dsc.bg_opa = LV_OPA_COVER;
    pin_dsc.border_width = 1;
    pin_dsc.border_color = lv_color_hex(0x203040);
    pin_dsc.border_opa = LV_OPA_COVER;

    lv_draw_label_dsc_t num_dsc;
    lv_draw_label_dsc_init(&num_dsc);
    num_dsc.font = &lv_font_montserrat_14;
    num_dsc.color = lv_color_hex(UI_FOOTER);
    num_dsc.align = LV_TEXT_ALIGN_CENTER;

    for(int i = 0; i < PAIR_CH_COUNT; i++) {
        lv_coord_t y = top_y + i * pitch;

        /* 本端针脚：有映射则按直通/错序着色 */
        if(crimp_has_result) {
            int8_t mid = seq_result.map_to[i];
            if(mid < 0) {
                pin_dsc.bg_color = lv_color_hex(PAIR_ST_OPEN_COLOR);
            }
            else if(mid == i) {
                pin_dsc.bg_color = lv_color_hex(PAIR_ST_CONN_COLOR);
            }
            else {
                pin_dsc.bg_color = lv_color_hex(0xE0A020);
            }
        }
        else {
            pin_dsc.bg_color = lv_color_hex(PAIR_WIRE_COLOR[i]);
        }
        lv_area_t pin_l = {left_x - pin_r, y - pin_r, left_x + pin_r, y + pin_r};
        lv_draw_rect(draw_ctx, &pin_dsc, &pin_l);

        /* 对端针脚：线序色 */
        pin_dsc.bg_color = lv_color_hex(PAIR_WIRE_COLOR[i]);
        lv_area_t pin_r_area = {right_x - pin_r, y - pin_r, right_x + pin_r, y + pin_r};
        lv_draw_rect(draw_ctx, &pin_dsc, &pin_r_area);

        char num[2] = {(char)('1' + i), '\0'};
        lv_area_t n_l = {coords.x1 + 2, y - 6, left_x - pin_r - 2, y + 6};
        lv_draw_label(draw_ctx, &num_dsc, &n_l, num, NULL);

        lv_area_t n_r = {right_x + pin_r + 2, y - 6, coords.x2 - 2, y + 6};
        lv_draw_label(draw_ctx, &num_dsc, &n_r, num, NULL);
    }
}

static void crimp_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if(key == LV_KEY_ESC) {
        crimp_ui_reset_state();
        back_to_main();
        lv_event_stop_processing(e);
    }
    else if(key == ' ' || key == LV_KEY_NEXT) {
        /* 板端无空格键：EC11 键(NEXT)在等待中模拟异步完成 */
        crimp_handle_space();
        lv_event_stop_processing(e);
    }
    else if(key == LV_KEY_ENTER) {
        crimp_toggle_start();
        lv_event_stop_processing(e);
    }
}

static void crimp_start_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        crimp_btn_focus_cb(e);
        return;
    }

    if(code == LV_EVENT_CLICKED) {
        crimp_toggle_start();
        return;
    }

    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ENTER) {
            crimp_toggle_start();
            lv_event_stop_processing(e);
        }
        else if(key == ' ' || key == LV_KEY_NEXT) {
            crimp_handle_space();
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ESC) {
            crimp_ui_reset_state();
            back_to_main();
            lv_event_stop_processing(e);
        }
    }
}

static lv_obj_t * create_crimp_screen(void)
{
    const lv_font_t * font = UI_CN_FONT;
    lv_obj_t * scr = lv_obj_create(NULL);
    style_screen_bg(scr);

    create_header(scr, "测序");

    lv_obj_t * body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, SCR_W, SCR_H - HDR_H - FTR_H);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    const int32_t btn_h = 36;
    const int32_t btn_gap = 6;
    crimp_diagram = lv_obj_create(body);
    lv_obj_remove_style_all(crimp_diagram);
    lv_obj_set_size(crimp_diagram, SCR_W - 8, SCR_H - HDR_H - FTR_H - btn_h - btn_gap - 8);
    lv_obj_align(crimp_diagram, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(crimp_diagram, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(crimp_diagram, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(crimp_diagram, 3, 0);
    lv_obj_set_style_border_color(crimp_diagram, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(crimp_diagram, 1, 0);
    lv_obj_clear_flag(crimp_diagram, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(crimp_diagram, crimp_diagram_draw_cb, LV_EVENT_DRAW_MAIN, NULL);

    crimp_start_btn = lv_btn_create(body);
    lv_obj_remove_style_all(crimp_start_btn);
    lv_obj_set_size(crimp_start_btn, 80, btn_h);
    lv_obj_align(crimp_start_btn, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(crimp_start_btn, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(crimp_start_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(crimp_start_btn, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(crimp_start_btn, 1, 0);
    lv_obj_set_style_radius(crimp_start_btn, 3, 0);
    lv_obj_add_event_cb(crimp_start_btn, crimp_start_btn_event_cb, LV_EVENT_ALL, NULL);

    crimp_start_lbl = lv_label_create(crimp_start_btn);
    lv_label_set_text(crimp_start_lbl, "开始");
    lv_obj_set_style_text_font(crimp_start_lbl, font, 0);
    lv_obj_set_style_text_color(crimp_start_lbl, lv_color_hex(CLR_CRIMP), 0);
    lv_obj_center(crimp_start_lbl);

    crimp_key_catcher = lv_obj_create(scr);
    lv_obj_remove_style_all(crimp_key_catcher);
    lv_obj_set_size(crimp_key_catcher, SCR_W, SCR_H);
    lv_obj_add_flag(crimp_key_catcher, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(crimp_key_catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(crimp_key_catcher, crimp_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(scr, crimp_key_catcher);
    crimp_apply_focus_style(false);
    crimp_ui_reset_state();
    return scr;
}

/* ---------- 压接界面 ---------- */

static void press_ui_reset_state(void)
{
    net_tester_press_stop();
    press_status = 0;
    press_has_result = false;
    if(press_start_lbl) {
        lv_label_set_text(press_start_lbl, "开始");
    }
    if(press_diagram) {
        lv_obj_invalidate(press_diagram);
    }
}

static void press_ui_set_waiting(void)
{
    press_status = 0;
    press_has_result = false;
    if(press_start_lbl) {
        lv_label_set_text(press_start_lbl, "测量中");
    }
    if(press_diagram) {
        lv_obj_invalidate(press_diagram);
    }
}

static void press_ui_set_result(uint8_t status)
{
    press_status = status;
    press_has_result = true;
    if(press_start_lbl) {
        lv_label_set_text(press_start_lbl, "开始");
    }
    if(press_diagram) {
        lv_obj_invalidate(press_diagram);
    }
}

static void press_result_cb(uint8_t status, void * user_data)
{
    LV_UNUSED(user_data);
    press_ui_set_result(status);
}

static void press_toggle_start(void)
{
    net_tester_press_start();
    press_ui_set_waiting();
}

static void press_apply_focus_style(bool focused)
{
    if(!press_start_btn) return;
    if(focused) {
        lv_obj_set_style_border_color(press_start_btn, lv_color_hex(UI_TITLE), 0);
        lv_obj_set_style_border_width(press_start_btn, 2, 0);
    }
    else {
        lv_obj_set_style_border_color(press_start_btn, lv_color_hex(UI_BORDER), 0);
        lv_obj_set_style_border_width(press_start_btn, 1, 0);
    }
}

static void press_btn_focus_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED) {
        press_apply_focus_style(true);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        press_apply_focus_style(false);
    }
}

static void press_handle_space(void)
{
    if(!net_tester_press_is_waiting()) {
        rt_kprintf("[PRESS] 空格无效: 请先点击开始\n");
        return;
    }
    net_tester_press_simulate_async_done();
}

static void press_diagram_draw_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;

    lv_obj_t * obj = lv_event_get_target(e);
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    const lv_coord_t w = lv_area_get_width(&coords);
    const lv_coord_t h = lv_area_get_height(&coords);
    /* 横向 8 触点：圆点 + 下方标号 1~8 */
    const lv_coord_t pad = 14;
    const lv_coord_t gap = (w - pad * 2) / 8;
    const lv_coord_t r = 10;
    const lv_coord_t cy = coords.y1 + h / 2 - 6;

    lv_draw_label_dsc_t title_dsc;
    lv_draw_label_dsc_init(&title_dsc);
    title_dsc.font = hzk12_font_get();
    title_dsc.color = lv_color_hex(CLR_PRESS);
    title_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t title_area = {coords.x1 + 10, coords.y1 + 6, coords.x2 - 10, coords.y1 + 20};
    lv_draw_label(draw_ctx, &title_dsc, &title_area, "水晶头压接", NULL);

    lv_draw_rect_dsc_t dot_dsc;
    lv_draw_rect_dsc_init(&dot_dsc);
    dot_dsc.radius = LV_RADIUS_CIRCLE;
    dot_dsc.bg_opa = LV_OPA_COVER;
    dot_dsc.border_width = 1;
    dot_dsc.border_color = lv_color_hex(0x203040);
    dot_dsc.border_opa = LV_OPA_COVER;

    lv_draw_label_dsc_t num_dsc;
    lv_draw_label_dsc_init(&num_dsc);
    num_dsc.font = &lv_font_montserrat_14;
    num_dsc.color = lv_color_hex(UI_FOOTER);
    num_dsc.align = LV_TEXT_ALIGN_CENTER;

    for(int i = 0; i < 8; i++) {
        lv_coord_t cx = coords.x1 + pad + gap / 2 + i * gap;
        bool ok = press_has_result && (press_status & (1u << i));
        /* 无结果时全灰；有结果：牢固绿 / 不牢固灰 */
        dot_dsc.bg_color = lv_color_hex(ok ? PRESS_OK_COLOR : PRESS_BAD_COLOR);
        lv_area_t dot = {cx - r, cy - r, cx + r, cy + r};
        lv_draw_rect(draw_ctx, &dot_dsc, &dot);

        char num[2] = {(char)('1' + i), '\0'};
        lv_area_t n_area = {cx - 8, cy + r + 6, cx + 8, cy + r + 22};
        lv_draw_label(draw_ctx, &num_dsc, &n_area, num, NULL);
    }
}

static void press_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if(key == LV_KEY_ESC) {
        press_ui_reset_state();
        back_to_main();
        lv_event_stop_processing(e);
    }
    else if(key == ' ' || key == LV_KEY_NEXT) {
        press_handle_space();
        lv_event_stop_processing(e);
    }
    else if(key == LV_KEY_ENTER) {
        press_toggle_start();
        lv_event_stop_processing(e);
    }
}

static void press_start_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) {
        press_btn_focus_cb(e);
        return;
    }

    if(code == LV_EVENT_CLICKED) {
        press_toggle_start();
        return;
    }

    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ENTER) {
            press_toggle_start();
            lv_event_stop_processing(e);
        }
        else if(key == ' ' || key == LV_KEY_NEXT) {
            press_handle_space();
            lv_event_stop_processing(e);
        }
        else if(key == LV_KEY_ESC) {
            press_ui_reset_state();
            back_to_main();
            lv_event_stop_processing(e);
        }
    }
}

static lv_obj_t * create_press_screen(void)
{
    const lv_font_t * font = UI_CN_FONT;
    lv_obj_t * scr = lv_obj_create(NULL);
    style_screen_bg(scr);

    create_header(scr, "压接");

    lv_obj_t * body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, SCR_W, SCR_H - HDR_H - FTR_H);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    const int32_t btn_h = 36;
    const int32_t btn_gap = 6;
    press_diagram = lv_obj_create(body);
    lv_obj_remove_style_all(press_diagram);
    lv_obj_set_size(press_diagram, SCR_W - 8, SCR_H - HDR_H - FTR_H - btn_h - btn_gap - 8);
    lv_obj_align(press_diagram, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(press_diagram, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(press_diagram, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(press_diagram, 3, 0);
    lv_obj_set_style_border_color(press_diagram, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(press_diagram, 1, 0);
    lv_obj_clear_flag(press_diagram, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(press_diagram, press_diagram_draw_cb, LV_EVENT_DRAW_MAIN, NULL);

    press_start_btn = lv_btn_create(body);
    lv_obj_remove_style_all(press_start_btn);
    lv_obj_set_size(press_start_btn, 80, btn_h);
    lv_obj_align(press_start_btn, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(press_start_btn, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(press_start_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(press_start_btn, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(press_start_btn, 1, 0);
    lv_obj_set_style_radius(press_start_btn, 3, 0);
    lv_obj_add_event_cb(press_start_btn, press_start_btn_event_cb, LV_EVENT_ALL, NULL);

    press_start_lbl = lv_label_create(press_start_btn);
    lv_label_set_text(press_start_lbl, "开始");
    lv_obj_set_style_text_font(press_start_lbl, font, 0);
    lv_obj_set_style_text_color(press_start_lbl, lv_color_hex(CLR_PRESS), 0);
    lv_obj_center(press_start_lbl);

    press_key_catcher = lv_obj_create(scr);
    lv_obj_remove_style_all(press_key_catcher);
    lv_obj_set_size(press_key_catcher, SCR_W, SCR_H);
    lv_obj_add_flag(press_key_catcher, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(press_key_catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(press_key_catcher, press_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(scr, press_key_catcher);
    press_apply_focus_style(false);
    press_ui_reset_state();
    return scr;
}

static lv_obj_t * create_header_ex(lv_obj_t * parent, const char * text, const lv_font_t * font)
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
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(UI_TITLE), 0);
    lv_obj_center(lbl);
    return hdr;
}

static void bat_icon_refresh(void)
{
    power_status_t st;
    uint32_t color;
    int32_t inner_w;
    int32_t fill_w;

    if(!bat_cont) return;

    power_status_get(&st);
    color = st.low ? BAT_CLR_LOW : BAT_CLR_OK;

    lv_obj_set_style_border_color(bat_body, lv_color_hex(BAT_CLR_FRAME), 0);
    lv_obj_set_style_bg_color(bat_tip, lv_color_hex(BAT_CLR_FRAME), 0);

    if(st.charging) {
        /* 充电：显示闪电，隐藏电量条 */
        lv_obj_add_flag(bat_fill, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(bat_bolt, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(bat_bolt, lv_color_hex(color), 0);
        lv_obj_set_style_border_color(bat_body, lv_color_hex(color), 0);
        lv_obj_set_style_bg_color(bat_tip, lv_color_hex(color), 0);
    }
    else {
        /* 非充电：按 1~4 档显示电量填充 */
        lv_obj_add_flag(bat_bolt, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(bat_fill, LV_OBJ_FLAG_HIDDEN);

        inner_w = BAT_BODY_W - 4;
        if(st.level < 1) st.level = 1;
        if(st.level > 4) st.level = 4;
        fill_w = (inner_w * (int32_t)st.level) / 4;
        if(fill_w < 2) fill_w = 2;

        lv_obj_set_width(bat_fill, fill_w);
        lv_obj_set_style_bg_color(bat_fill, lv_color_hex(color), 0);
        lv_obj_set_style_border_color(bat_body, lv_color_hex(color), 0);
        lv_obj_set_style_bg_color(bat_tip, lv_color_hex(color), 0);
    }
}

static void bat_icon_timer_cb(lv_timer_t * t)
{
    (void)t;
    bat_icon_refresh();
}

static void bat_icon_create(void)
{
    /* 浮于顶层右上角，不拦截输入 */
    bat_cont = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(bat_cont);
    lv_obj_set_size(bat_cont, BAT_ICON_W, BAT_ICON_H);
    lv_obj_align(bat_cont, LV_ALIGN_TOP_RIGHT, -6, (HDR_H - BAT_ICON_H) / 2);
    lv_obj_set_style_bg_opa(bat_cont, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(bat_cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    bat_body = lv_obj_create(bat_cont);
    lv_obj_remove_style_all(bat_body);
    lv_obj_set_size(bat_body, BAT_BODY_W, BAT_BODY_H);
    lv_obj_align(bat_body, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(bat_body, 2, 0);
    lv_obj_set_style_border_width(bat_body, 1, 0);
    lv_obj_set_style_border_opa(bat_body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bat_body, lv_color_hex(BAT_CLR_FRAME), 0);
    lv_obj_set_style_bg_opa(bat_body, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(bat_body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    bat_fill = lv_obj_create(bat_body);
    lv_obj_remove_style_all(bat_fill);
    lv_obj_set_size(bat_fill, BAT_BODY_W / 2, BAT_BODY_H - 4);
    lv_obj_align(bat_fill, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_radius(bat_fill, 1, 0);
    lv_obj_set_style_bg_opa(bat_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bat_fill, lv_color_hex(BAT_CLR_OK), 0);
    lv_obj_clear_flag(bat_fill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    bat_tip = lv_obj_create(bat_cont);
    lv_obj_remove_style_all(bat_tip);
    lv_obj_set_size(bat_tip, BAT_TIP_W, BAT_TIP_H);
    lv_obj_align(bat_tip, LV_ALIGN_LEFT_MID, BAT_BODY_W, 0);
    lv_obj_set_style_radius(bat_tip, 1, 0);
    lv_obj_set_style_bg_opa(bat_tip, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bat_tip, lv_color_hex(BAT_CLR_FRAME), 0);
    lv_obj_clear_flag(bat_tip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    bat_bolt = lv_label_create(bat_cont);
    lv_label_set_text(bat_bolt, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_font(bat_bolt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(bat_bolt, lv_color_hex(BAT_CLR_OK), 0);
    lv_obj_set_style_text_opa(bat_bolt, LV_OPA_COVER, 0);
    lv_obj_align(bat_bolt, LV_ALIGN_LEFT_MID, (BAT_BODY_W - 10) / 2, 0);
    lv_obj_add_flag(bat_bolt, LV_OBJ_FLAG_HIDDEN);

    bat_icon_refresh();
    bat_timer = lv_timer_create(bat_icon_timer_cb, BAT_POLL_MS, NULL);
}

static lv_obj_t * create_header(lv_obj_t * parent, const char * text)
{
    return create_header_ex(parent, text, UI_CN_FONT);
}

static lv_obj_t * create_sub_screen(net_func_id_t id, const char * title, uint32_t color, const char * icon_sym)
{
    const lv_font_t * font = UI_CN_FONT;
    lv_obj_t * scr = lv_obj_create(NULL);
    style_screen_bg(scr);

    create_header(scr, title);

    lv_obj_t * center = lv_obj_create(scr);
    lv_obj_remove_style_all(center);
    lv_obj_set_size(center, SCR_W, SCR_H - HDR_H - FTR_H);
    lv_obj_align(center, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_set_flex_flow(center, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(center, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(center, 10, 0);
    lv_obj_clear_flag(center, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * ico = lv_label_create(center);
    lv_label_set_text(ico, icon_sym);
    lv_obj_set_style_text_font(ico, UI_ICON_FONT, 0);
    lv_obj_set_style_text_color(ico, lv_color_hex(color), 0);

    lv_obj_t * name = lv_label_create(center);
    lv_label_set_text(name, title);
    lv_obj_set_style_text_font(name, font, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(color), 0);

    lv_obj_t * key_catcher = lv_obj_create(scr);
    lv_obj_remove_style_all(key_catcher);
    lv_obj_set_size(key_catcher, SCR_W, SCR_H);
    lv_obj_add_flag(key_catcher, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(key_catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(key_catcher, sub_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(scr, key_catcher);
    LV_UNUSED(id);
    return scr;
}

static lv_obj_t * create_trace_screen(void)
{
    const lv_font_t * font = UI_CN_FONT;
    lv_obj_t * scr = lv_obj_create(NULL);
    style_screen_bg(scr);

    create_header(scr, "寻线");

    lv_obj_t * body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, SCR_W, SCR_H - HDR_H - FTR_H);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, HDR_H);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    trace_anim_box = lv_obj_create(body);
    lv_obj_remove_style_all(trace_anim_box);
    lv_obj_set_size(trace_anim_box, 140, 88);
    lv_obj_align(trace_anim_box, LV_ALIGN_CENTER, 0, -20);
    lv_obj_clear_flag(trace_anim_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(trace_anim_box, LV_OBJ_FLAG_HIDDEN);

    trace_anim_icon = lv_label_create(trace_anim_box);
    lv_label_set_text(trace_anim_icon, LV_SYMBOL_GPS);
    lv_obj_set_style_text_font(trace_anim_icon, UI_ICON_FONT, 0);
    lv_obj_set_style_text_color(trace_anim_icon, lv_color_hex(CLR_TRACE), 0);
    lv_obj_align(trace_anim_icon, LV_ALIGN_TOP_MID, 0, 0);

    for(int i = 0; i < 3; i++) {
        trace_anim_dots[i] = lv_label_create(trace_anim_box);
        lv_label_set_text(trace_anim_dots[i], LV_SYMBOL_BULLET);
        lv_obj_set_style_text_font(trace_anim_dots[i], UI_ICON_FONT, 0);
        lv_obj_set_style_text_color(trace_anim_dots[i], lv_color_hex(UI_FOOTER), 0);
        lv_obj_align(trace_anim_dots[i], LV_ALIGN_BOTTOM_MID, (i - 1) * 22, -2);
    }

    trace_start_btn = lv_btn_create(body);
    lv_obj_remove_style_all(trace_start_btn);
    lv_obj_set_size(trace_start_btn, 96, 36);
    lv_obj_align(trace_start_btn, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_color(trace_start_btn, lv_color_hex(UI_PANEL), 0);
    lv_obj_set_style_bg_opa(trace_start_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trace_start_btn, lv_color_hex(UI_BORDER), 0);
    lv_obj_set_style_border_width(trace_start_btn, 1, 0);
    lv_obj_set_style_radius(trace_start_btn, 3, 0);
    lv_obj_add_event_cb(trace_start_btn, trace_start_btn_event_cb, LV_EVENT_ALL, NULL);

    trace_start_lbl = lv_label_create(trace_start_btn);
    lv_label_set_text(trace_start_lbl, "开始");
    lv_obj_set_style_text_font(trace_start_lbl, font, 0);
    lv_obj_set_style_text_color(trace_start_lbl, lv_color_hex(CLR_TRACE), 0);
    lv_obj_center(trace_start_lbl);

    trace_anim_timer = lv_timer_create(trace_anim_timer_cb, 300, NULL);

    trace_key_catcher = lv_obj_create(scr);
    lv_obj_remove_style_all(trace_key_catcher);
    lv_obj_set_size(trace_key_catcher, SCR_W, SCR_H);
    lv_obj_add_flag(trace_key_catcher, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(trace_key_catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(trace_key_catcher, trace_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(scr, trace_key_catcher);
    trace_apply_focus_style(false);
    trace_ui_reset_state();
    return scr;
}

static lv_obj_t * create_debug_screen(void)
{
    /* 调试入口：网络协议二级菜单 */
    return net_tester_netdbg_create_menu();
}

static lv_obj_t * create_about_screen(void)
{
    lv_obj_t * scr = net_tester_about_create_screen();
    lv_obj_t * catcher = (lv_obj_t *)lv_obj_get_user_data(scr);

    if(catcher) {
        lv_obj_add_event_cb(catcher, sub_key_cb, LV_EVENT_KEY, NULL);
    }
    return scr;
}

static void bind_sub_group(lv_obj_t * scr)
{
    if(sub_group) {
        lv_group_del(sub_group);
        sub_group = NULL;
    }

    sub_group = lv_group_create();
    lv_group_set_wrap(sub_group, true);

    if(scr == sub_screens[NET_FUNC_DEBUG]) {
        lv_group_set_editing(sub_group, false);
        net_tester_netdbg_bind_group(sub_group);
    }
    else if(scr == sub_screens[NET_FUNC_TRACE]) {
        if(net_tester_trace_is_running()) {
            net_tester_trace_stop();
        }
        trace_ui_reset_state();
        trace_apply_focus_style(false);
        if(trace_key_catcher) {
            lv_group_add_obj(sub_group, trace_key_catcher);
            lv_group_focus_obj(trace_key_catcher);
        }
        lv_group_add_obj(sub_group, trace_start_btn);
    }
    else if(scr == sub_screens[NET_FUNC_PAIR]) {
        pair_ui_reset_state();
        pair_apply_focus_style(false);
        if(pair_key_catcher) {
            lv_group_add_obj(sub_group, pair_key_catcher);
            lv_group_focus_obj(pair_key_catcher);
        }
        if(pair_start_btn) {
            lv_group_add_obj(sub_group, pair_start_btn);
            lv_group_focus_obj(pair_start_btn);
        }
    }
    else if(scr == sub_screens[NET_FUNC_CRIMP]) {
        crimp_ui_reset_state();
        crimp_apply_focus_style(false);
        if(crimp_key_catcher) {
            lv_group_add_obj(sub_group, crimp_key_catcher);
            lv_group_focus_obj(crimp_key_catcher);
        }
        if(crimp_start_btn) {
            lv_group_add_obj(sub_group, crimp_start_btn);
            lv_group_focus_obj(crimp_start_btn);
        }
    }
    else if(scr == sub_screens[NET_FUNC_PRESS]) {
        press_ui_reset_state();
        press_apply_focus_style(false);
        if(press_key_catcher) {
            lv_group_add_obj(sub_group, press_key_catcher);
            lv_group_focus_obj(press_key_catcher);
        }
        if(press_start_btn) {
            lv_group_add_obj(sub_group, press_start_btn);
            lv_group_focus_obj(press_start_btn);
        }
    }
    else {
        lv_obj_t * catcher = (lv_obj_t *)lv_obj_get_user_data(scr);
        if(catcher) {
            lv_group_add_obj(sub_group, catcher);
            lv_group_focus_obj(catcher);
        }
    }

    lv_indev_set_group(keypad_dev, sub_group);
    if(encoder_dev) lv_indev_set_group(encoder_dev, sub_group);
}

static void open_sub_screen(net_func_id_t id)
{
    if(id < 0 || id >= MENU_COUNT || !sub_screens[id]) return;

    bind_sub_group(sub_screens[id]);
    lv_scr_load(sub_screens[id]);

    if(id == NET_FUNC_TRACE) {
        if(keypad_dev) lv_indev_reset(keypad_dev, NULL);
        if(encoder_dev) lv_indev_reset(encoder_dev, NULL);
        lv_timer_t * focus_timer = lv_timer_create(trace_focus_start_timer_cb, TRACE_FOCUS_DELAY_MS, NULL);
        lv_timer_set_repeat_count(focus_timer, 1);
    }
}

static void back_to_main(void)
{
    if(net_tester_trace_is_running()) {
        net_tester_trace_stop();
    }
    trace_ui_reset_state();
    pair_ui_reset_state();
    crimp_ui_reset_state();
    press_ui_reset_state();

    net_tester_netdbg_on_leave();

    if(sub_group) {
        lv_group_del(sub_group);
        sub_group = NULL;
    }

    lv_indev_set_group(keypad_dev, main_group);
    if(encoder_dev) lv_indev_set_group(encoder_dev, main_group);
    lv_scr_load(main_screen);
    refresh_all_tiles();
}

static lv_obj_t * create_menu_tile(lv_obj_t * parent, net_func_id_t id, int col, int row)
{
    const lv_font_t * font = UI_CN_FONT;

    static const char * names[MENU_COUNT] = {
        "寻线", "测序", "对线", "压接", "调试", "关于"
    };
    static const char * icons[MENU_COUNT] = {
        LV_SYMBOL_GPS,       /* 寻线 */
        LV_SYMBOL_USB,       /* 测序 */
        LV_SYMBOL_LOOP,      /* 对线 */
        LV_SYMBOL_CHARGE,    /* 压接 */
        LV_SYMBOL_LIST,      /* 调试 */
        LV_SYMBOL_HOME,      /* 关于 */
    };
    static const uint32_t colors[MENU_COUNT] = {
        CLR_TRACE, CLR_CRIMP, CLR_PAIR, CLR_PRESS, CLR_DEBUG, CLR_ABOUT
    };

    int32_t grid_w = SCR_W - GRID_PAD_H * 2;
    int32_t grid_h = SCR_H - HDR_H - FTR_H - GRID_PAD_V * 2;
    int32_t tile_w = (grid_w - GRID_GAP) / MENU_COLS;
    int32_t tile_h = (grid_h - GRID_GAP * (MENU_ROWS - 1)) / MENU_ROWS;
    int32_t text_h = MENU_TEXT_H;
    int32_t icon_h = tile_h - text_h;
    int32_t x;
    int32_t y = row * (tile_h + GRID_GAP);

    /* 末行单独一项时水平居中，保持与其它按钮同尺寸 */
    if(row == MENU_ROWS - 1 && col < 0) {
        x = (grid_w - tile_w) / 2;
    }
    else {
        x = col * (tile_w + GRID_GAP);
    }

    menu_items[id].name = names[id];
    menu_items[id].icon_sym = icons[id];
    menu_items[id].color = colors[id];

    lv_obj_t * tile = lv_btn_create(parent);
    lv_obj_remove_style_all(tile);
    lv_obj_set_size(tile, tile_w, tile_h);
    lv_obj_set_pos(tile, x, y);
    lv_obj_set_style_pad_all(tile, 0, 0);
    lv_obj_set_style_pad_row(tile, 0, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    /* 图标区：随 tile_h 缩放，文字区高度固定以保住比例观感 */
    lv_obj_t * ico_box = lv_obj_create(tile);
    lv_obj_remove_style_all(ico_box);
    lv_obj_set_size(ico_box, tile_w, icon_h);
    lv_obj_align(ico_box, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(ico_box, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * icon_lbl = lv_label_create(ico_box);
    lv_label_set_text(icon_lbl, icons[id]);
    lv_obj_set_style_text_font(icon_lbl, UI_ICON_FONT, 0);
    lv_obj_set_style_text_color(icon_lbl, lv_color_hex(colors[id]), 0);
    lv_obj_align(icon_lbl, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * txt_box = lv_obj_create(tile);
    lv_obj_remove_style_all(txt_box);
    lv_obj_set_size(txt_box, tile_w, text_h);
    lv_obj_align(txt_box, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(txt_box, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(txt_box);
    lv_label_set_text(label, names[id]);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(colors[id]), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 1);

    lv_obj_add_event_cb(tile, tile_click_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);
    lv_obj_add_event_cb(tile, tile_key_cb, LV_EVENT_KEY, (void *)(uintptr_t)id);

    menu_items[id].tile = tile;
    menu_items[id].icon_lbl = icon_lbl;
    style_tile(&menu_items[id], false);

    lv_group_add_obj(main_group, tile);
    return tile;
}

/* LVGL v8 KEYPAD：LV_KEY_NEXT 固定切焦点。编辑态 remap，避免未退出编辑就跳控件 */
static uint32_t debug_key_remap_cb(uint32_t key)
{
    if(key != LV_KEY_NEXT) {
        return key;
    }

    if(!net_tester_netdbg_is_on_screen()) {
        return key;
    }

    return net_tester_netdbg_key_remap(key);
}

static void create_main_screen(void)
{
    main_screen = lv_obj_create(NULL);
    style_screen_bg(main_screen);

    create_header_ex(main_screen, "网络测试仪", UI_CN_FONT);

    int32_t grid_w = SCR_W - GRID_PAD_H * 2;
    int32_t grid_h = SCR_H - HDR_H - FTR_H - GRID_PAD_V * 2;

    lv_obj_t * grid = lv_obj_create(main_screen);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, grid_w, grid_h);
    lv_obj_set_pos(grid, GRID_PAD_H, HDR_H + GRID_PAD_V);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    create_menu_tile(grid, NET_FUNC_TRACE, 0, 0);
    create_menu_tile(grid, NET_FUNC_CRIMP, 1, 0);  /* 测序 */
    create_menu_tile(grid, NET_FUNC_PAIR, 0, 1);
    create_menu_tile(grid, NET_FUNC_PRESS, 1, 1);  /* 压接 */
    create_menu_tile(grid, NET_FUNC_DEBUG, 0, 2);
    create_menu_tile(grid, NET_FUNC_ABOUT, 1, 2);  /* 关于 */

    lv_group_focus_obj(menu_items[0].tile);
    refresh_all_tiles();
}

void net_tester_ui_init(lv_indev_t * keypad, lv_indev_t * encoder)
{
    keypad_dev = keypad;
    encoder_dev = encoder;

    main_group = lv_group_create();
    lv_group_set_wrap(main_group, true);
    lv_group_set_focus_cb(main_group, group_focus_cb);

    create_main_screen();

    sub_screens[NET_FUNC_TRACE] = create_trace_screen();
    sub_screens[NET_FUNC_CRIMP] = create_crimp_screen();
    sub_screens[NET_FUNC_PAIR]  = create_pair_screen();
    sub_screens[NET_FUNC_PRESS] = create_press_screen();
    sub_screens[NET_FUNC_DEBUG] = create_debug_screen();
    sub_screens[NET_FUNC_ABOUT] = create_about_screen();

    net_tester_pair_set_result_cb(pair_result_cb, NULL);
    net_tester_crimp_set_result_cb(crimp_result_cb, NULL);
    net_tester_press_set_result_cb(press_result_cb, NULL);

    net_tester_netdbg_set_leave_cb(back_to_main);

    /* 调试页编辑态：Tab 走控件内部光标/数位，不切焦点 */
    lv_port_indev_set_key_remap_cb(debug_key_remap_cb);

    lv_indev_set_group(keypad_dev, main_group);
    if(encoder_dev) lv_indev_set_group(encoder_dev, main_group);
    lv_scr_load(main_screen);

    /* 顶栏右上角供电状态图标 */
    bat_icon_create();
}
