/*
 * CherryUSB 板级配置（RP2350B USB Device CDC）
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include <rtthread.h>

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF(...) rt_kprintf(__VA_ARGS__)

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_ERROR
#endif

#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE 4
#endif

/* RP2350 链接脚本无 .noncacheable，放入普通 RAM 即可 */
#define USB_NOCACHE_RAM_SECTION

#define usb_malloc(size) rt_malloc(size)
#define usb_free(ptr)    rt_free(ptr)

/* ================= USB Device Stack Configuration ================ */

#ifndef CONFIG_USBDEV_REQUEST_BUFFER_LEN
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512
#endif

/* 模板描述符 API（回调方式）需要此宏 */
#define CONFIG_USBDEV_ADVANCE_DESC

#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS 1
#endif

#ifndef CONFIG_USBDEV_EP_NUM
#define CONFIG_USBDEV_EP_NUM 16
#endif

#ifndef CONFIG_USBDEV_MAX_CDC_ACM_CLASS
#define CONFIG_USBDEV_MAX_CDC_ACM_CLASS 1
#endif

#ifndef CONFIG_USBDEV_SERIAL_RX_BUFSIZE
#define CONFIG_USBDEV_SERIAL_RX_BUFSIZE 2048
#endif

/* rt_usbd_serial 设备名缓冲区长度依赖此宏 */
#ifndef CONFIG_USBHOST_DEV_NAMELEN
#define CONFIG_USBHOST_DEV_NAMELEN 16
#endif

#endif /* CHERRYUSB_CONFIG_H */
