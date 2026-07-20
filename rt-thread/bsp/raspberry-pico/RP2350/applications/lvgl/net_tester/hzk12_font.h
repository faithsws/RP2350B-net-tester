/*
 * 字体适配：板级 HZK12 / HZK16，兼容模拟器接口
 */
#ifndef HZK12_FONT_H
#define HZK12_FONT_H

#include <lvgl.h>
#include "lv_font_hzk.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline bool hzk12_font_init(void)
{
    return lv_font_hzk_init() == RT_EOK;
}

static inline const lv_font_t * hzk12_font_get(void)
{
    return &lv_font_hzk12;
}

static inline const lv_font_t * hzk16_font_get(void)
{
    return &lv_font_hzk16;
}

/* 界面主字体：HZK12 软件放大到 24x24（无需独立 HZK24 字库文件） */
static inline const lv_font_t * hzk24_font_get(void)
{
    return &lv_font_hzk24;
}

static inline void hzk12_font_deinit(void)
{
}

#ifdef __cplusplus
}
#endif

#endif /* HZK12_FONT_H */
