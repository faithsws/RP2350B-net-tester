/*
 * FAL 分区表：在片内 Flash 末尾预留参数区
 *
 * Flash 布局（2MB，偏移相对片内 Flash 起点）：
 *   0x000000 ~ 0x1F0000 : 固件 / 字模等（约 1984KB）
 *   0x1F0000 ~ 0x1F4000 : param 参数区（16KB，4×4KB 扇区）
 *
 * UF2 镜像当前约 1.03MB，不会覆盖 param 区。
 */
#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtthread.h>

extern const struct fal_flash_dev _onchip_flash;

/* Flash 设备表 */
#define FAL_FLASH_DEV_TABLE \
{                           \
    &_onchip_flash,         \
}

#ifdef FAL_PART_HAS_TABLE_CFG
#define FAL_PART_TABLE                                                              \
{                                                                                   \
    {FAL_PART_MAGIC_WORD, "param", "onchip_flash", 0x1F0000, 16 * 1024, 0},         \
}
#endif

#endif /* _FAL_CFG_H_ */
