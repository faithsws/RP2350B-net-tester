#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__

#include <stdint.h>
#include <rtdevice.h>

#define LCD_WIDTH   240
#define LCD_HEIGHT  240

#define LCD_CS_PIN   9
#define LCD_CLK_PIN  10
#define LCD_MOSI_PIN 11
#define LCD_RST_PIN  12
#define LCD_DC_PIN   13
/* LCD 背光使能：GPIO20，高电平有效 */
#define LCD_BL_PIN   20

#define LCD_SPI_BUS  "spi1"
#define LCD_SPI_DEV  "spi11"

#define VERTICAL   0
#define HORIZONTAL 1

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint8_t scan_dir;
} lcd_attributes_t;

extern lcd_attributes_t lcd_attr;

/* RGB565 本地色值（小端 CPU 内存序），刷屏时由驱动按 SPI 大端发出 */
#define LCD_COLOR_BLACK   0x0000
#define LCD_COLOR_WHITE   0xFFFF
#define LCD_COLOR_RED     0xF800
#define LCD_COLOR_GREEN   0x07E0
#define LCD_COLOR_BLUE    0x001F
#define LCD_COLOR_YELLOW  0xFFE0
#define LCD_COLOR_CYAN    0x07FF
#define LCD_COLOR_MAGENTA 0xF81F

int lcd_hw_init(void);
void lcd_backlight_on(void);
void lcd_backlight_off(void);
void lcd_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void lcd_flush_area(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end,
                    const uint16_t *color);
/* 纯色填充整屏 / 区域（color 为 CPU 序 RGB565，内部会按 ST7789 字节序发送） */
void lcd_fill_color(uint16_t color);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#endif /* __DRV_LCD_H__ */
