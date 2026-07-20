/*
 * LVGL 显示对接 - ST7789 240x240 (LVGL v8)
 */
#include <lvgl.h>
#include "drv_lcd.h"

#define DISP_BUF_SIZE (LCD_WIDTH * 20)

static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t disp_buf1[DISP_BUF_SIZE];

static void lcd_fb_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    lcd_flush_area((uint16_t)area->x1, (uint16_t)area->y1,
                   (uint16_t)area->x2 + 1, (uint16_t)area->y2 + 1,
                   (const uint16_t *)color_p);
    lv_disp_flush_ready(drv);
}

void lv_port_disp_init(void)
{
    if (lcd_hw_init() != RT_EOK)
    {
        return;
    }

    lv_disp_draw_buf_init(&disp_buf, disp_buf1, RT_NULL, DISP_BUF_SIZE);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = lcd_fb_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);
}
