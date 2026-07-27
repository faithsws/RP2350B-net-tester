# HZK 字模嵌入配置（scons --target=cmake 不会覆盖此文件）

get_filename_component(HZK_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

# 字模原始文件路径（工程根目录下的 HZK16 / HZK12）
set(HZK16_SOURCE_FILE "${HZK_PROJECT_ROOT}/HZK16")
set(HZK12_SOURCE_FILE "${HZK_PROJECT_ROOT}/HZK12")

# Flash 链接地址
#   0       = 紧接主程序之后自动排列（推荐，bin 无填充间隙）
#   非 0    = 强制指定地址（如 0x10100000，bin 会包含中间填充）
set(HZK16_LINK_ADDR 0)
set(HZK12_LINK_ADDR 0)

# RP2350 GPIO 协处理器与 gpio_get(SIO) 不同步，统一关闭
add_compile_definitions(PICO_USE_GPIO_COPROCESSOR=0)

# 用本地 config 覆盖 SDK 默认 pico2（30 GPIO），强制 RP2350B / 48 GPIO
set(_RP2350B_PICO_CFG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/board/pico_cfg_override")
file(MAKE_DIRECTORY "${_RP2350B_PICO_CFG_DIR}/pico")
file(WRITE "${_RP2350B_PICO_CFG_DIR}/pico/config_autogen.h"
"// 本文件由 custom.cmake 生成，优先于 SDK pico_base_headers
#include \"boards/solderparty_rp2350_stamp_xl.h\"
#include \"cmsis/rename_exceptions.h\"
")
include_directories(BEFORE "${_RP2350B_PICO_CFG_DIR}")
add_compile_definitions(PICO_RP2350A=0)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/hzk_embed.cmake)
hzk_embed_all(${CMAKE_PROJECT_NAME}.elf)

# scons --target=cmake 偶发漏扫新文件，强制加入网络调试源
if(TARGET rtt_LVGL-net-tester)
    target_sources(rtt_LVGL-net-tester PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/applications/lvgl/net_tester/net_tester_netdbg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/applications/lvgl/net_tester/net_tester_netops.c
    )
endif()

# CherryUSB Device CDC ACM（scons --target=cmake 未重新生成时由此强制接入）
if(TARGET rtt_Drivers)
    set(_CHERRYUSB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../components/drivers/usb/cherryusb")
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/board/ports/cherryusb
        ${_CHERRYUSB_ROOT}
        ${_CHERRYUSB_ROOT}/common
        ${_CHERRYUSB_ROOT}/core
        ${_CHERRYUSB_ROOT}/class/cdc
        ${_CHERRYUSB_ROOT}/class/hub
        ${_CHERRYUSB_ROOT}/osal
        ${_CHERRYUSB_ROOT}/port/rp2040
    )
    target_sources(rtt_Drivers PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/board/ports/cherryusb/cherryusb_cdc.c
        ${_CHERRYUSB_ROOT}/core/usbd_core.c
        ${_CHERRYUSB_ROOT}/osal/usb_osal_rtthread.c
        ${_CHERRYUSB_ROOT}/port/rp2040/usb_dc_rp2040.c
        ${_CHERRYUSB_ROOT}/class/cdc/usbd_cdc_acm.c
        ${_CHERRYUSB_ROOT}/platform/rtthread/rt_usbd_serial.c
    )
endif()

# FAL + 片内 Flash + INI 参数（末尾 param 分区）
if(TARGET rtt_Drivers)
    set(_FAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../components/fal")
    set(_DRV_FLASH "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/Drivers/drv_flash.c")
    set(_HW_FLASH "${CMAKE_CURRENT_SOURCE_DIR}/packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_flash/flash.c")
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/board/ports/fal
        ${CMAKE_CURRENT_SOURCE_DIR}/board/ports/param
        ${_FAL_ROOT}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_xip_cache/include
    )
    # 单核：写 Flash 时假定 core1 安全
    add_compile_definitions(PICO_FLASH_ASSUME_CORE1_SAFE=1)
    target_sources(rtt_Drivers PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/board/ports/param/net_param.c
        ${_DRV_FLASH}
        ${_FAL_ROOT}/src/fal.c
        ${_FAL_ROOT}/src/fal_flash.c
        ${_FAL_ROOT}/src/fal_partition.c
        ${_FAL_ROOT}/src/fal_rtt.c
    )
    if(TARGET rtt_raspberrypi-pico-rp2350-sdk)
        target_sources(rtt_raspberrypi-pico-rp2350-sdk PRIVATE
            ${_HW_FLASH}
            ${CMAKE_CURRENT_SOURCE_DIR}/packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_xip_cache/xip_cache.c
        )
    endif()
endif()
