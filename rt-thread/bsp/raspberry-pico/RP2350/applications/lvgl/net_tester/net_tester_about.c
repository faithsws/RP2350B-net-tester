/*
 * 关于页
 */
#include "net_tester_about.h"

#include "hzk12_font.h"
#include "lv_qrcode.h"

#include <stdio.h>
#include <string.h>

#include <pico/unique_id.h>

LV_FONT_DECLARE(lv_font_montserrat_14)
LV_FONT_DECLARE(lv_font_montserrat_20)

#define ABOUT_UI_BG        0x0E1218
#define ABOUT_UI_TITLE     0x6EC8D8
#define ABOUT_UI_TEXT      0xA8B8C8
#define ABOUT_HDR_H        32
/*
 * 网址约 version5（37 模）。画布取 37*3=111，使 scale 整除、margin=0，
 * 避免 LVGL qrcode 余数挤在右/下导致白边不对称。
 * 外框再加等宽 quiet zone。
 */
#define ABOUT_QR_MODULES   37
#define ABOUT_QR_SCALE     3
#define ABOUT_QR_SIZE      (ABOUT_QR_MODULES * ABOUT_QR_SCALE) /* 111 */
#define ABOUT_QR_PAD       8
#define ABOUT_QR_FRAME     (ABOUT_QR_SIZE + ABOUT_QR_PAD * 2)  /* 127 */
#define ABOUT_SCR_W        240
#define ABOUT_SCR_H        240

#define ABOUT_UID_HEX_LEN  (PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2)

static void about_style_screen_bg(lv_obj_t * scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(ABOUT_UI_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

static void about_create_header(lv_obj_t * parent)
{
    const lv_font_t * font = hzk24_font_get();
    lv_obj_t * hdr = lv_obj_create(parent);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, ABOUT_SCR_W, ABOUT_HDR_H);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * title = lv_label_create(hdr);
    lv_label_set_text(title, "关于");
    lv_obj_set_style_text_font(title, font, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(ABOUT_UI_TITLE), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);
}

static void about_uid_to_hex(char *out, uint32_t out_len)
{
    if(out == NULL || out_len < (ABOUT_UID_HEX_LEN + 1U)) {
        return;
    }

    /* RP2350：通过 bootrom GET_SYS_INFO 读取芯片唯一 ID */
    pico_get_unique_board_id_string(out, out_len);
}

static lv_obj_t * about_add_info_line(lv_obj_t * parent, const char * text)
{
    lv_obj_t * lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(ABOUT_UI_TEXT), 0);
    return lbl;
}

lv_obj_t * net_tester_about_create_screen(void)
{
    char uid[ABOUT_UID_HEX_LEN + 1];
    char line[48];
    lv_obj_t * scr;
    lv_obj_t * body;
    lv_obj_t * qr;
    lv_obj_t * info;
    lv_obj_t * credit;
    lv_obj_t * key_catcher;
    const lv_font_t * cn_font = hzk24_font_get();
    const char * url = NET_TESTER_ABOUT_URL;

    scr = lv_obj_create(NULL);
    about_style_screen_bg(scr);
    about_create_header(scr);

    body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, ABOUT_SCR_W, ABOUT_SCR_H - ABOUT_HDR_H);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, ABOUT_HDR_H);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

#if LV_USE_QRCODE
    /* 白底外框：四周等宽 quiet zone，QR 画布尺寸整除模块数 */
    lv_obj_t * qr_frame = lv_obj_create(body);
    lv_obj_remove_style_all(qr_frame);
    lv_obj_set_size(qr_frame, ABOUT_QR_FRAME, ABOUT_QR_FRAME);
    lv_obj_align(qr_frame, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(qr_frame, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(qr_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(qr_frame, 2, 0);
    lv_obj_clear_flag(qr_frame, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    qr = lv_qrcode_create(qr_frame, ABOUT_QR_SIZE,
                          lv_color_hex(0x000000),
                          lv_color_hex(0xFFFFFF));
    lv_obj_center(qr);
    lv_qrcode_update(qr, url, (uint32_t)strlen(url));
#else
    qr = lv_label_create(body);
    lv_label_set_text(qr, "QR N/A");
    lv_obj_set_style_text_color(qr, lv_color_hex(ABOUT_UI_TEXT), 0);
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 20);
#endif

    /* 底部明文：版本（同一行）+ UID */
    about_uid_to_hex(uid, sizeof(uid));

    info = lv_obj_create(body);
    lv_obj_remove_style_all(info);
    lv_obj_set_size(info, ABOUT_SCR_W - 8, 40);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -34);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(info, 2, 0);
    lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    snprintf(line, sizeof(line), "SW:%s  HW:%s",
             NET_TESTER_SW_VERSION, NET_TESTER_HW_VERSION);
    about_add_info_line(info, line);
    snprintf(line, sizeof(line), "UID:%s", uid);
    about_add_info_line(info, line);

    /* Faithsws 用西文字体，出品用点阵汉字 */
    credit = lv_obj_create(body);
    lv_obj_remove_style_all(credit);
    lv_obj_set_size(credit, ABOUT_SCR_W, 26);
    lv_obj_align(credit, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_flex_flow(credit, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(credit, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(credit, 2, 0);
    lv_obj_clear_flag(credit, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    {
        lv_obj_t * en = lv_label_create(credit);
        lv_label_set_text(en, "Faithsws");
        lv_obj_set_style_text_font(en, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(en, lv_color_hex(ABOUT_UI_TITLE), 0);

        lv_obj_t * cn = lv_label_create(credit);
        lv_label_set_text(cn, "出品");
        lv_obj_set_style_text_font(cn, cn_font, 0);
        lv_obj_set_style_text_color(cn, lv_color_hex(ABOUT_UI_TITLE), 0);
    }

    key_catcher = lv_obj_create(scr);
    lv_obj_remove_style_all(key_catcher);
    lv_obj_set_size(key_catcher, ABOUT_SCR_W, ABOUT_SCR_H);
    lv_obj_add_flag(key_catcher, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(key_catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(scr, key_catcher);

    return scr;
}
