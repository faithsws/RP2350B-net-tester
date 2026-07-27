/*
 * RP2350B 板载 USB CDC ACM：枚举为虚拟串口，并自动把 FinSH 切到 usb-acm0
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>

#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "hardware/regs/addressmap.h"

#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

#define USBD_VID           0x2E8A
#define USBD_PID           0x000A
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

#define USB_CDC_DEV_NAME "usb-acm0"

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02)
};

static const uint8_t device_quality_descriptor[] = {
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 }, /* Langid */
    "Faithsws",                   /* Manufacturer */
    "Net Tester CDC",             /* Product */
    "RP2350B",                    /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor cdc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

static rt_sem_t g_cdc_evt_sem = RT_NULL;
static volatile rt_uint8_t g_cdc_configured = 0;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;

    switch (event) {
    case USBD_EVENT_CONFIGURED:
        g_cdc_configured = 1;
        if (g_cdc_evt_sem) {
            rt_sem_release(g_cdc_evt_sem);
        }
        break;
    case USBD_EVENT_DISCONNECTED:
    case USBD_EVENT_RESET:
        g_cdc_configured = 0;
        if (g_cdc_evt_sem) {
            rt_sem_release(g_cdc_evt_sem);
        }
        break;
    default:
        break;
    }
}

extern void usbd_cdc_acm_serial_init(uint8_t busid, uint8_t in_ep, uint8_t out_ep);

static void usb_cdc_finsh_thread(void *parameter)
{
    (void)parameter;

    while (1) {
        rt_sem_take(g_cdc_evt_sem, RT_WAITING_FOREVER);

        if (g_cdc_configured) {
            /* 给主机枚举/打开串口一点时间 */
            rt_thread_mdelay(50);
            if (!g_cdc_configured) {
                continue;
            }
            /* 先打日志再切控制台，避免切到 CDC 后 UART 看不到 */
            rt_kprintf("\n[usb] FinSH -> %s\n", USB_CDC_DEV_NAME);
            finsh_set_device(USB_CDC_DEV_NAME);
            rt_console_set_device(USB_CDC_DEV_NAME);
        } else {
            finsh_set_device(RT_CONSOLE_DEVICE_NAME);
            rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
            rt_kprintf("\n[usb] FinSH -> %s\n", RT_CONSOLE_DEVICE_NAME);
        }
    }
}

static int cherryusb_cdc_init(void)
{
    g_cdc_evt_sem = rt_sem_create("cdc_evt", 0, RT_IPC_FLAG_FIFO);
    if (g_cdc_evt_sem == RT_NULL) {
        return -RT_ENOMEM;
    }

    usbd_desc_register(0, &cdc_descriptor);
    usbd_cdc_acm_serial_init(0, CDC_IN_EP, CDC_OUT_EP);
    usbd_initialize(0, USBCTRL_REGS_BASE, usbd_event_handler);

    rt_thread_t tid = rt_thread_create("usb_finsh",
                                       usb_cdc_finsh_thread,
                                       RT_NULL,
                                       1024,
                                       25,
                                       10);
    if (tid) {
        rt_thread_startup(tid);
    }

    rt_kprintf("[usb] CDC ACM init, wait host (device %s)\n", USB_CDC_DEV_NAME);
    return RT_EOK;
}
INIT_APP_EXPORT(cherryusb_cdc_init);

/* 手动切换：UART / USB */
static int cdc_acm_enter(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    finsh_set_device(USB_CDC_DEV_NAME);
    rt_console_set_device(USB_CDC_DEV_NAME);
    return 0;
}
MSH_CMD_EXPORT(cdc_acm_enter, switch FinSH to USB CDC);

static int cdc_acm_exit(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    finsh_set_device(RT_CONSOLE_DEVICE_NAME);
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
    return 0;
}
MSH_CMD_EXPORT(cdc_acm_exit, switch FinSH back to UART);
