/*
 * CH390D 板级移植：RP2350B + 硬件 SPI0
 *
 * 接线:
 *   GPIO0 - MISO
 *   GPIO1 - CS
 *   GPIO2 - SCLK
 *   GPIO3 - MOSI
 *   GPIO4 - INT
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_spi.h>
#include <ch390.h>

#ifdef BSP_USING_CH390

#define DBG_TAG "drv.ch390.port"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static const rt_uint8_t ch390_mac[6] = { 0x02, 0x12, 0x34, 0x56, 0x78, 0x01 };

static int rt_hw_ch390_port_init(void)
{
    struct ch390_config config;
    rt_err_t ret;

    ret = rt_hw_spi_device_attach(BSP_CH390_SPI_BUS_NAME,
                                  BSP_CH390_SPI_DEVICE_NAME,
                                  BSP_CH390_CS_PIN);
    if (ret != RT_EOK)
    {
        LOG_E("attach %s to %s failed: %d",
              BSP_CH390_SPI_DEVICE_NAME, BSP_CH390_SPI_BUS_NAME, ret);
        return (int)ret;
    }

    config.spi_device_name = BSP_CH390_SPI_DEVICE_NAME;
    config.netif_name = BSP_CH390_NETIF_NAME;
    config.rst_pin = BSP_CH390_RST_PIN;
    config.int_pin = BSP_CH390_INT_PIN;
    config.mac = ch390_mac;
    config.spi_max_hz = BSP_CH390_SPI_MAX_HZ;
    config.poll_interval_ms = BSP_CH390_POLL_INTERVAL_MS;

    return ch390_attach(&config);
}
INIT_DEVICE_EXPORT(rt_hw_ch390_port_init);

#endif /* BSP_USING_CH390 */
