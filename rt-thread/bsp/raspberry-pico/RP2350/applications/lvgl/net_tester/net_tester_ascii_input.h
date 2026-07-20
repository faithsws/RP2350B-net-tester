#ifndef NET_TESTER_ASCII_INPUT_H
#define NET_TESTER_ASCII_INPUT_H

#include <lvgl.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_ASCII_MAX_LEN              16
#define NET_TESTER_ASCII_KEY_TAB         ((uint32_t)'\x02')  /* 光标模式：短/长按 */
#define NET_TESTER_ASCII_KEY_TAB_NEXT    ((uint32_t)'\x03')  /* 修改/插入模式：下一字符 */

typedef enum {
    NET_ASCII_MODE_FOCUS = 0,
    NET_ASCII_MODE_CURSOR,
    NET_ASCII_MODE_MODIFY,
    NET_ASCII_MODE_INSERT,
} net_tester_ascii_mode_t;

typedef struct {
    lv_obj_t * root;
    lv_obj_t * slot_lbl[NET_ASCII_MAX_LEN];
    char text[NET_ASCII_MAX_LEN + 1];
    uint8_t len;
    uint8_t cursor;
    net_tester_ascii_mode_t mode;
    lv_timer_t * blink_timer;
    lv_timer_t * tab_timer;
    bool blink_on;
    bool tab_waiting;
} net_tester_ascii_input_t;

lv_obj_t * net_tester_ascii_input_create(lv_obj_t * parent, net_tester_ascii_input_t * ctx);
bool net_tester_ascii_input_is_active(const net_tester_ascii_input_t * ctx);
bool net_tester_ascii_input_wants_tab_key(const net_tester_ascii_input_t * ctx);
bool net_tester_ascii_input_wants_tab_next(const net_tester_ascii_input_t * ctx);
void net_tester_ascii_input_cancel_active(net_tester_ascii_input_t * ctx);
void net_tester_ascii_input_handle_esc(net_tester_ascii_input_t * ctx);
void net_tester_ascii_input_refresh_focus(net_tester_ascii_input_t * ctx, bool focused);
const char * net_tester_ascii_input_get_value(const net_tester_ascii_input_t * ctx);
/* 设置文本内容（截断至 NET_ASCII_MAX_LEN） */
void net_tester_ascii_input_set_value(net_tester_ascii_input_t * ctx, const char * text);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_ASCII_INPUT_H */
