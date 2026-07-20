/*
 * ST7789 240x240 SPI LCD 驱动 (SPI1)
 * 背光：GPIO20，高电平点亮
 */
#include "drv_lcd.h"
#include <drv_spi.h>
#include <string.h>
#include <hardware/gpio.h>

#define DBG_TAG "drv.lcd"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

lcd_attributes_t lcd_attr;

static struct rt_spi_device *lcd_spi;
static rt_bool_t lcd_ready = RT_FALSE;

static void lcd_bl_hw_set(rt_bool_t on)
{
    /* 直接走 pico-sdk，保证 pad ISO 解除且电平可靠 */
    gpio_init(LCD_BL_PIN);
    gpio_set_dir(LCD_BL_PIN, GPIO_OUT);
    gpio_put(LCD_BL_PIN, on ? 1 : 0);
}

static void lcd_gpio_init(void)
{
    rt_pin_mode(LCD_RST_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DC_PIN, PIN_MODE_OUTPUT);

    rt_pin_write(LCD_RST_PIN, PIN_HIGH);
    rt_pin_write(LCD_DC_PIN, PIN_LOW);

    /* 背光高电平有效：GPIO 配置后立即点亮，避免后续初始化失败导致一直灭 */
    lcd_bl_hw_set(RT_TRUE);
}

static void lcd_write_cmd(uint8_t cmd)
{
    rt_pin_write(LCD_DC_PIN, PIN_LOW);
    rt_spi_send(lcd_spi, &cmd, 1);
}

static void lcd_write_data8(uint8_t data)
{
    rt_pin_write(LCD_DC_PIN, PIN_HIGH);
    rt_spi_send(lcd_spi, &data, 1);
}

static void lcd_write_data16(uint16_t data)
{
    uint8_t buf[2];

    buf[0] = (uint8_t)(data >> 8);
    buf[1] = (uint8_t)(data & 0xFF);
    rt_pin_write(LCD_DC_PIN, PIN_HIGH);
    rt_spi_send(lcd_spi, buf, 2);
}

static void lcd_reset(void)
{
    rt_pin_write(LCD_RST_PIN, PIN_HIGH);
    rt_thread_mdelay(10);
    rt_pin_write(LCD_RST_PIN, PIN_LOW);
    rt_thread_mdelay(10);
    rt_pin_write(LCD_RST_PIN, PIN_HIGH);
    rt_thread_mdelay(120);
}

static void lcd_set_scan_dir(uint8_t scan_dir)
{
    uint8_t mem_access = 0x00;

    lcd_attr.scan_dir = scan_dir;
    if (scan_dir == HORIZONTAL)
    {
        lcd_attr.width = LCD_HEIGHT;
        lcd_attr.height = LCD_WIDTH;
        mem_access = 0x70;
    }
    else
    {
        lcd_attr.width = LCD_WIDTH;
        lcd_attr.height = LCD_HEIGHT;
        mem_access = 0x00;
    }

    lcd_write_cmd(0x36);
    lcd_write_data8(mem_access);
}

static void lcd_init_reg(void)
{
    lcd_write_cmd(0x3A);
    lcd_write_data8(0x05);

    lcd_write_cmd(0xB2);
    lcd_write_data8(0x0C);
    lcd_write_data8(0x0C);
    lcd_write_data8(0x00);
    lcd_write_data8(0x33);
    lcd_write_data8(0x33);

    lcd_write_cmd(0xB7);
    lcd_write_data8(0x35);

    lcd_write_cmd(0xBB);
    lcd_write_data8(0x19);

    lcd_write_cmd(0xC0);
    lcd_write_data8(0x2C);

    lcd_write_cmd(0xC2);
    lcd_write_data8(0x01);

    lcd_write_cmd(0xC3);
    lcd_write_data8(0x12);

    lcd_write_cmd(0xC4);
    lcd_write_data8(0x20);

    lcd_write_cmd(0xC6);
    lcd_write_data8(0x0F);

    lcd_write_cmd(0xD0);
    lcd_write_data8(0xA4);
    lcd_write_data8(0xA1);

    lcd_write_cmd(0xE0);
    lcd_write_data8(0xD0);
    lcd_write_data8(0x04);
    lcd_write_data8(0x0D);
    lcd_write_data8(0x11);
    lcd_write_data8(0x13);
    lcd_write_data8(0x2B);
    lcd_write_data8(0x3F);
    lcd_write_data8(0x54);
    lcd_write_data8(0x4C);
    lcd_write_data8(0x18);
    lcd_write_data8(0x0D);
    lcd_write_data8(0x0B);
    lcd_write_data8(0x1F);
    lcd_write_data8(0x23);

    lcd_write_cmd(0xE1);
    lcd_write_data8(0xD0);
    lcd_write_data8(0x04);
    lcd_write_data8(0x0C);
    lcd_write_data8(0x11);
    lcd_write_data8(0x13);
    lcd_write_data8(0x2C);
    lcd_write_data8(0x3F);
    lcd_write_data8(0x44);
    lcd_write_data8(0x51);
    lcd_write_data8(0x2F);
    lcd_write_data8(0x1F);
    lcd_write_data8(0x1F);
    lcd_write_data8(0x20);
    lcd_write_data8(0x23);

    lcd_write_cmd(0x21);
    lcd_write_cmd(0x11);
    rt_thread_mdelay(120);
    lcd_write_cmd(0x29);
}

void lcd_backlight_on(void)
{
    /* GPIO20 高电平点亮背光 */
    lcd_bl_hw_set(RT_TRUE);
}

void lcd_backlight_off(void)
{
    lcd_bl_hw_set(RT_FALSE);
}

void lcd_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    lcd_write_cmd(0x2A);
    lcd_write_data16(x_start);
    lcd_write_data16(x_end - 1);

    lcd_write_cmd(0x2B);
    lcd_write_data16(y_start);
    lcd_write_data16(y_end - 1);

    lcd_write_cmd(0x2C);
}

/* ST7789 SPI 要高字节先发；LE 主机上的 RGB565 需交换后再送 */
static inline uint16_t lcd_rgb565_to_spi(uint16_t color)
{
    return (uint16_t)((color << 8) | (color >> 8));
}

void lcd_flush_area(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end,
                    const uint16_t *color)
{
    uint32_t pixel_count;

    if (color == RT_NULL)
    {
        return;
    }

    if (x_end <= x_start || y_end <= y_start)
    {
        return;
    }

    pixel_count = (uint32_t)(x_end - x_start) * (uint32_t)(y_end - y_start);
    lcd_set_window(x_start, y_start, x_end, y_end);
    rt_pin_write(LCD_DC_PIN, PIN_HIGH);
    /* LVGL 开启 LV_COLOR_16_SWAP 后缓冲已是 SPI 字节序，此处直接发送 */
    rt_spi_send(lcd_spi, color, pixel_count * sizeof(uint16_t));
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t line[LCD_WIDTH];
    uint16_t spi_color;
    uint16_t row;
    uint16_t col;
    uint16_t x_end;
    uint16_t y_end;

    if (!lcd_ready || lcd_spi == RT_NULL || w == 0 || h == 0)
    {
        return;
    }

    if (x >= lcd_attr.width || y >= lcd_attr.height)
    {
        return;
    }

    if ((uint32_t)x + w > lcd_attr.width)
    {
        w = (uint16_t)(lcd_attr.width - x);
    }
    if ((uint32_t)y + h > lcd_attr.height)
    {
        h = (uint16_t)(lcd_attr.height - y);
    }

    spi_color = lcd_rgb565_to_spi(color);
    for (col = 0; col < w; col++)
    {
        line[col] = spi_color;
    }

    x_end = (uint16_t)(x + w);
    y_end = (uint16_t)(y + h);
    lcd_set_window(x, y, x_end, y_end);
    rt_pin_write(LCD_DC_PIN, PIN_HIGH);
    for (row = 0; row < h; row++)
    {
        rt_spi_send(lcd_spi, line, (rt_size_t)w * sizeof(uint16_t));
    }
}

void lcd_fill_color(uint16_t color)
{
    lcd_fill_rect(0, 0, lcd_attr.width, lcd_attr.height, color);
}

int lcd_hw_init(void)
{
    struct rt_spi_configuration cfg;
    rt_err_t ret;

    if (lcd_ready)
    {
        return RT_EOK;
    }

    lcd_gpio_init();

    ret = rt_hw_spi_device_attach(LCD_SPI_BUS, LCD_SPI_DEV, LCD_CS_PIN);
    if (ret != RT_EOK)
    {
        LOG_E("attach %s failed: %d", LCD_SPI_DEV, ret);
        return (int)ret;
    }

    lcd_spi = (struct rt_spi_device *)rt_device_find(LCD_SPI_DEV);
    if (lcd_spi == RT_NULL)
    {
        LOG_E("spi device %s not found", LCD_SPI_DEV);
        return -RT_ENOSYS;
    }

    cfg.data_width = 8;
    cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.max_hz = 40 * 1000 * 1000;
    ret = rt_spi_configure(lcd_spi, &cfg);
    if (ret != RT_EOK)
    {
        LOG_E("spi configure failed: %d", ret);
        return (int)ret;
    }

    lcd_reset();
    lcd_set_scan_dir(VERTICAL);
    lcd_init_reg();
    lcd_backlight_on();

    lcd_ready = RT_TRUE;
    LOG_I("ST7789 %dx%d init ok", lcd_attr.width, lcd_attr.height);
    return RT_EOK;
}
