#ifndef NET_TESTER_IP_INPUT_H
#define NET_TESTER_IP_INPUT_H

#include <lvgl.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_TESTER_IP_KEY_DIGIT_NEXT  ((uint32_t)'\x01')  /* Tab 重映射：编辑态切换数位 */

typedef struct {
    lv_obj_t * root;
    lv_obj_t * digit_lbl[12];
    bool editing;
    uint8_t cur_digit;
    uint8_t digits[12];
    lv_timer_t * blink_timer;
    bool blink_on;
} net_tester_ip_input_t;

lv_obj_t * net_tester_ip_input_create(lv_obj_t * parent, net_tester_ip_input_t * ctx);
void net_tester_ip_input_enter_edit(net_tester_ip_input_t * ctx);
bool net_tester_ip_input_exit_edit(net_tester_ip_input_t * ctx);
bool net_tester_ip_input_is_editing(const net_tester_ip_input_t * ctx);
void net_tester_ip_input_cancel_edit(net_tester_ip_input_t * ctx);
void net_tester_ip_input_handle_key(net_tester_ip_input_t * ctx, uint32_t key);
void net_tester_ip_input_refresh_focus(net_tester_ip_input_t * ctx, bool focused);
bool net_tester_ip_input_get_value(const net_tester_ip_input_t * ctx, char * out, size_t out_sz);
/* 从 "a.b.c.d" 设置数位；失败返回 false */
bool net_tester_ip_input_set_value(net_tester_ip_input_t * ctx, const char * ip_str);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_IP_INPUT_H */
