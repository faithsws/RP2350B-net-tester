#ifndef LV_PORT_INDEV_H__
#define LV_PORT_INDEV_H__

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 在 KEYPAD 交给 LVGL group 前重映射按键（用于编辑态吞掉 LV_KEY_NEXT） */
typedef uint32_t (*lv_port_key_remap_cb_t)(uint32_t key);

void lv_port_indev_init(void);
void lv_port_indev_set_key_remap_cb(lv_port_key_remap_cb_t cb);
lv_group_t *lv_port_indev_get_group(void);
lv_indev_t *lv_port_indev_get_keypad(void);
lv_indev_t *lv_port_indev_get_encoder(void);

#ifdef __cplusplus
}
#endif

#endif
