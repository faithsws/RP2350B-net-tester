CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR cortex-m33)
#SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_C_COMPILER "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc.exe")
SET(CMAKE_ASM_COMPILER "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc.exe")
SET(CMAKE_C_FLAGS " -march=armv8-m.main+fp+dsp -mcpu=cortex-m33 -mthumb -ffunction-sections -fdata-sections -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -mcmse -Dgcc -O3 -gdwarf-2 -g")
SET(CMAKE_ASM_FLAGS " -c -march=armv8-m.main+fp+dsp -mcpu=cortex-m33 -mthumb -ffunction-sections -fdata-sections -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -mcmse -x assembler-with-cpp -Wa,-mimplicit-it=always -gdwarf-2")
SET(CMAKE_C_COMPILER_WORKS TRUE)

SET(CMAKE_CXX_COMPILER "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-g++.exe")
SET(CMAKE_CXX_FLAGS " -march=armv8-m.main+fp+dsp -mcpu=cortex-m33 -mthumb -ffunction-sections -fdata-sections -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -mcmse -Dgcc -O3 -gdwarf-2 -g  -Wl,--build-id=none --specs=nosys.specs ")
SET(CMAKE_CXX_COMPILER_WORKS TRUE)

SET(CMAKE_OBJCOPY "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-objcopy.exe")
SET(CMAKE_SIZE "C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-size.exe")

SET(CMAKE_EXE_LINKER_FLAGS " -march=armv8-m.main+fp+dsp -mcpu=cortex-m33 -mthumb -ffunction-sections -fdata-sections -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -mcmse -Wl,--gc-sections,-Map=rt-thread.map,-cref,-u,Reset_Handler -T ${CMAKE_SOURCE_DIR}/link.ld -Wl,--gc-sections,--print-memory-usage")

SET(CMAKE_C_STANDARD 11)
SET(CMAKE_CXX_STANDARD 17)

PROJECT(rtthread C CXX ASM)

INCLUDE_DIRECTORIES(
	.
	applications
	board/ports
	applications/lvgl/demo
	applications/lvgl/net_tester
	applications/lvgl
	board/ports/battery
	board/ports/charger
	board/ports/lcd
	board/ports/ec11
	board/ports/hzk
	board
	board/ports/pwm
	board/ports/mux4051
	board/ports/hc238
	../../../components/drivers/spi
	packages/lvgl-v8.3-latest/env_support/rt-thread
	packages/lvgl-v8.3-latest/src
	packages/lvgl-v8.3-latest/src/core
	packages/lvgl-v8.3-latest/src/draw
	packages/lvgl-v8.3-latest/src/extra
	packages/lvgl-v8.3-latest/src/font
	packages/lvgl-v8.3-latest/src/hal
	packages/lvgl-v8.3-latest/src/misc
	packages/lvgl-v8.3-latest/src/widgets
	packages/lvgl-v8.3-latest/src/draw/arm2d
	packages/lvgl-v8.3-latest/src/draw/renesas
	packages/lvgl-v8.3-latest/src/draw/sdl
	packages/lvgl-v8.3-latest/src/draw/stm32_dma2d
	packages/lvgl-v8.3-latest/src/draw/sw
	packages/lvgl-v8.3-latest/src/draw/swm341_dma2d
	packages/lvgl-v8.3-latest/src/draw/nxp/pxp
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite
	packages/lvgl-v8.3-latest/src/extra/layouts
	packages/lvgl-v8.3-latest/src/extra/libs
	packages/lvgl-v8.3-latest/src/extra/others
	packages/lvgl-v8.3-latest/src/extra/themes
	packages/lvgl-v8.3-latest/src/extra/widgets
	packages/lvgl-v8.3-latest/src/extra/layouts/flex
	packages/lvgl-v8.3-latest/src/extra/layouts/grid
	packages/lvgl-v8.3-latest/src/extra/libs/bmp
	packages/lvgl-v8.3-latest/src/extra/libs/ffmpeg
	packages/lvgl-v8.3-latest/src/extra/libs/freetype
	packages/lvgl-v8.3-latest/src/extra/libs/fsdrv
	packages/lvgl-v8.3-latest/src/extra/libs/gif
	packages/lvgl-v8.3-latest/src/extra/libs/png
	packages/lvgl-v8.3-latest/src/extra/libs/qrcode
	packages/lvgl-v8.3-latest/src/extra/libs/rlottie
	packages/lvgl-v8.3-latest/src/extra/libs/sjpg
	packages/lvgl-v8.3-latest/src/extra/libs/tiny_ttf
	packages/lvgl-v8.3-latest/src/extra/others/fragment
	packages/lvgl-v8.3-latest/src/extra/others/gridnav
	packages/lvgl-v8.3-latest/src/extra/others/ime
	packages/lvgl-v8.3-latest/src/extra/others/imgfont
	packages/lvgl-v8.3-latest/src/extra/others/monkey
	packages/lvgl-v8.3-latest/src/extra/others/msg
	packages/lvgl-v8.3-latest/src/extra/others/snapshot
	packages/lvgl-v8.3-latest/src/extra/themes/basic
	packages/lvgl-v8.3-latest/src/extra/themes/default
	packages/lvgl-v8.3-latest/src/extra/themes/mono
	packages/lvgl-v8.3-latest/src/extra/widgets/animimg
	packages/lvgl-v8.3-latest/src/extra/widgets/calendar
	packages/lvgl-v8.3-latest/src/extra/widgets/chart
	packages/lvgl-v8.3-latest/src/extra/widgets/colorwheel
	packages/lvgl-v8.3-latest/src/extra/widgets/imgbtn
	packages/lvgl-v8.3-latest/src/extra/widgets/keyboard
	packages/lvgl-v8.3-latest/src/extra/widgets/led
	packages/lvgl-v8.3-latest/src/extra/widgets/list
	packages/lvgl-v8.3-latest/src/extra/widgets/menu
	packages/lvgl-v8.3-latest/src/extra/widgets/meter
	packages/lvgl-v8.3-latest/src/extra/widgets/msgbox
	packages/lvgl-v8.3-latest/src/extra/widgets/span
	packages/lvgl-v8.3-latest/src/extra/widgets/spinbox
	packages/lvgl-v8.3-latest/src/extra/widgets/spinner
	packages/lvgl-v8.3-latest/src/extra/widgets/tabview
	packages/lvgl-v8.3-latest/src/extra/widgets/tileview
	packages/lvgl-v8.3-latest/src/extra/widgets/win
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/cmsis/stub/CMSIS/Core/Include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/cmsis/stub/CMSIS/Device/RP2350/Include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/boards/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/cmsis/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_base/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_irq/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_boot_lock/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_sync/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_sync_spin_lock/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_platform_compiler/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_platform_panic/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_platform_sections/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/boot_bootrom_headers/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_bootrom/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_flash/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_stdio/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_timer/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_gpio/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_uart/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_resets/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_stdio_uart/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_runtime/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_runtime_init/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_time_adapter/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_divider/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_clocks/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_ticks/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_pll/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_vreg/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_watchdog/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_xosc/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_double/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_float/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_malloc/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_printf/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_spi/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_dma/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_adc/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_pwm/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_flash/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_unique_id/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_stdlib_headers/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_divider_headers/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_binary_info/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/boot_picobin_headers/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/boot_picoboot_headers/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2350/hardware_regs/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2350/pico_platform/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2350/hardware_structs/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2350/boot_stage2/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_base_headers/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/hardware_claim/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_time/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_sync/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_util/include
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_dcp/include
	../../../include
	../../../libcpu/arm/common
	../../../libcpu/arm/cortex-m33
	../../../components/drivers/include
	../../../components/drivers/phy
	../../../components/drivers/smp_call
	../../../components/finsh
	../../../components/libc/compilers/common/include
	../../../components/libc/compilers/newlib
	../../../components/libc/posix/io/epoll
	../../../components/libc/posix/io/eventfd
	../../../components/libc/posix/io/poll
	../../../components/libc/posix/ipc
	../../../components/net/lwip/lwip-2.1.2/src/include
	../../../components/net/lwip/lwip-2.1.2/src/include/netif
	../../../components/net/lwip/lwip-2.1.2/src/include/compat/posix
	../../../components/net/lwip/port
	../../../components/net/netdev/include
	../../../components/net/utest
	../libraries/Drivers
)

ADD_DEFINITIONS(
	-DLIB_BOOT_STAGE2_HEADERS=1
	-DLIB_PICO_ATOMIC=1
	-DLIB_PICO_BIT_OPS=1
	-DLIB_PICO_BIT_OPS_PICO=1
	-DLIB_PICO_CLIB_INTERFACE=1
	-DLIB_PICO_CRT0=1
	-DLIB_PICO_CXX_OPTIONS=1
	-DLIB_PICO_DIVIDER=1
	-DLIB_PICO_DIVIDER_COMPILER=1
	-DLIB_PICO_DOUBLE=1
	-DLIB_PICO_DOUBLE_PICO=1
	-DLIB_PICO_FLASH=1
	-DLIB_PICO_FLOAT=1
	-DLIB_PICO_FLOAT_PICO=1
	-DLIB_PICO_FLOAT_PICO_VFP=1
	-DLIB_PICO_INT64_OPS=1
	-DLIB_PICO_INT64_OPS_COMPILER=1
	-DLIB_PICO_MALLOC=1
	-DLIB_PICO_MEM_OPS=1
	-DLIB_PICO_MEM_OPS_COMPILER=1
	-DLIB_PICO_NEWLIB_INTERFACE=1
	-DLIB_PICO_PLATFORM=1
	-DLIB_PICO_PLATFORM_COMPILER=1
	-DLIB_PICO_PLATFORM_PANIC=1
	-DLIB_PICO_PLATFORM_SECTIONS=1
	-DLIB_PICO_PRINTF=1
	-DLIB_PICO_PRINTF_PICO=1
	-DLIB_PICO_RUNTIME=1
	-DLIB_PICO_RUNTIME_INIT=1
	-DLIB_PICO_STANDARD_BINARY_INFO=1
	-DLIB_PICO_STANDARD_LINK=1
	-DLIB_PICO_STDIO=1
	-DLIB_PICO_STDIO_UART=1
	-DLIB_PICO_STDLIB=1
	-DLIB_PICO_SYNC=1
	-DLIB_PICO_SYNC_CRITICAL_SECTION=1
	-DLIB_PICO_SYNC_MUTEX=1
	-DLIB_PICO_SYNC_SEM=1
	-DLIB_PICO_TIME=1
	-DLIB_PICO_TIME_ADAPTER=1
	-DLIB_PICO_UTIL=1
	-DPICO_32BIT=1
	-DPICO_BUILD=1
	-DPICO_COPY_TO_RAM=0
	-DPICO_CXX_ENABLE_EXCEPTIONS=0
	-DPICO_NO_FLASH=0
	-DPICO_NO_HARDWARE=0
	-DPICO_ON_DEVICE=1
	-DPICO_RP2350=1
	-DPICO_USE_BLOCKED_RAM=0
	-DPICO_BOARD="solderparty_rp2350_stamp_xl"
	-DLIB_CMSIS_CORE=1
	-D__RTTHREAD__
	-DRT_USING_NEWLIBC
	-DRT_USING_LIBC
	-D_POSIX_C_SOURCE=1
)

# Library source files
SET(RT_APPLICATIONS_SOURCES
	applications/main.c
)

SET(RT_CPU_SOURCES
	../../../libcpu/arm/common/atomic_arm.c
	../../../libcpu/arm/common/div0.c
	../../../libcpu/arm/common/showmem.c
	../../../libcpu/arm/cortex-m33/context_gcc.S
	../../../libcpu/arm/cortex-m33/cpuport.c
	../../../libcpu/arm/cortex-m33/syscall_gcc.S
	../../../libcpu/arm/cortex-m33/trustzone.c
)

SET(RT_DEVICEDRIVERS_SOURCES
	../../../components/drivers/core/device.c
	../../../components/drivers/ipc/completion_comm.c
	../../../components/drivers/ipc/completion_up.c
	../../../components/drivers/ipc/condvar.c
	../../../components/drivers/ipc/dataqueue.c
	../../../components/drivers/ipc/pipe.c
	../../../components/drivers/ipc/ringblk_buf.c
	../../../components/drivers/ipc/ringbuffer.c
	../../../components/drivers/ipc/waitqueue.c
	../../../components/drivers/ipc/workqueue.c
	../../../components/drivers/pin/dev_pin.c
	../../../components/drivers/serial/dev_serial.c
	../../../components/drivers/spi/ch390.c
	../../../components/drivers/spi/dev_spi.c
	../../../components/drivers/spi/dev_spi_core.c
)

SET(RT_DRIVERS_SOURCES
	board/board.c
	board/ports/battery/battery_adc.c
	board/ports/battery/power_status.c
	board/ports/boost_pwr.c
	board/ports/ch390_port.c
	board/ports/ch390_net_cmd.c
	board/ports/net_tools_cmd.c
	board/ports/ssdp_listen.c
	board/ports/link_blink_cmd.c
	board/ports/arp_cmd.c
	board/ports/eth_info_cmd.c
	board/ports/charger/charge_status.c
	board/ports/charger/charger_detect.c
	board/ports/charger/charger_port.c
	board/ports/ec11/ec11.c
	board/ports/hc238/ctrl_h_hc238.c
	board/ports/hc238/ctrl_l_hc238.c
	board/ports/hc238/hc238.c
	board/ports/hc238/hc238_port.c
	board/ports/hc238/pair_judge.c
	board/ports/hc238/seq_judge.c
	board/ports/hzk/hzk_core.c
	board/ports/hzk/hzk_port.c
	board/ports/hzk/hzk_uni2gb.c
	board/ports/lcd/drv_lcd.c
	board/ports/mux4051/link_adc.c
	board/ports/mux4051/link_mux4051.c
	board/ports/mux4051/mux4051_port.c
	board/ports/mux4051/press_scan.c
	board/ports/mux4051/rx_adc.c
	board/ports/mux4051/rx_mux4051.c
	board/ports/mux4051/tmux4051.c
	board/ports/mux4051/tx_mux4051.c
	board/ports/power_hold.c
	board/ports/pwm/sc_link_pwm.c
	../libraries/Drivers/drv_gpio.c
	../libraries/Drivers/drv_spi.c
	../libraries/Drivers/drv_uart.c
)

SET(RT_FINSH_SOURCES
	../../../components/finsh/shell.c
	../../../components/finsh/msh.c
	../../../components/finsh/msh_parse.c
	../../../components/finsh/cmd.c
)

SET(RT_KERNEL_SOURCES
	../../../src/clock.c
	../../../src/components.c
	../../../src/cpu_up.c
	../../../src/defunct.c
	../../../src/idle.c
	../../../src/ipc.c
	../../../src/irq.c
	../../../src/kservice.c
	../../../src/mem.c
	../../../src/mempool.c
	../../../src/object.c
	../../../src/scheduler_comm.c
	../../../src/scheduler_up.c
	../../../src/thread.c
	../../../src/timer.c
)

SET(RT_LIBC_SOURCES
	../../../components/libc/compilers/common/cctype.c
	../../../components/libc/compilers/common/cstdlib.c
	../../../components/libc/compilers/common/cstring.c
	../../../components/libc/compilers/common/ctime.c
	../../../components/libc/compilers/common/cunistd.c
	../../../components/libc/compilers/common/cwchar.c
	../../../components/libc/compilers/newlib/syscalls.c
	../../../src/klibc/kerrno.c
	../../../src/klibc/kstdio.c
	../../../src/klibc/kstring.c
	../../../src/klibc/rt_vsnprintf_tiny.c
	../../../src/klibc/rt_vsscanf.c
)

SET(RT_LVGL_SOURCES
	packages/lvgl-v8.3-latest/src/core/lv_disp.c
	packages/lvgl-v8.3-latest/src/core/lv_event.c
	packages/lvgl-v8.3-latest/src/core/lv_group.c
	packages/lvgl-v8.3-latest/src/core/lv_indev.c
	packages/lvgl-v8.3-latest/src/core/lv_indev_scroll.c
	packages/lvgl-v8.3-latest/src/core/lv_obj.c
	packages/lvgl-v8.3-latest/src/core/lv_obj_class.c
	packages/lvgl-v8.3-latest/src/core/lv_obj_draw.c
	packages/lvgl-v8.3-latest/src/core/lv_obj_pos.c
	packages/lvgl-v8.3-latest/src/core/lv_obj_scroll.c
	packages/lvgl-v8.3-latest/src/core/lv_obj_style.c
	packages/lvgl-v8.3-latest/src/core/lv_obj_style_gen.c
	packages/lvgl-v8.3-latest/src/core/lv_obj_tree.c
	packages/lvgl-v8.3-latest/src/core/lv_refr.c
	packages/lvgl-v8.3-latest/src/core/lv_theme.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_arc.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_img.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_label.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_layer.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_line.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_mask.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_rect.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_transform.c
	packages/lvgl-v8.3-latest/src/draw/lv_draw_triangle.c
	packages/lvgl-v8.3-latest/src/draw/lv_img_buf.c
	packages/lvgl-v8.3-latest/src/draw/lv_img_cache.c
	packages/lvgl-v8.3-latest/src/draw/lv_img_decoder.c
	packages/lvgl-v8.3-latest/src/extra/lv_extra.c
	packages/lvgl-v8.3-latest/src/font/lv_font.c
	packages/lvgl-v8.3-latest/src/font/lv_font_dejavu_16_persian_hebrew.c
	packages/lvgl-v8.3-latest/src/font/lv_font_fmt_txt.c
	packages/lvgl-v8.3-latest/src/font/lv_font_loader.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_10.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_12.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_12_subpx.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_14.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_16.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_18.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_20.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_22.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_24.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_26.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_28.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_28_compressed.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_30.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_32.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_34.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_36.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_38.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_40.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_42.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_44.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_46.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_48.c
	packages/lvgl-v8.3-latest/src/font/lv_font_montserrat_8.c
	packages/lvgl-v8.3-latest/src/font/lv_font_simsun_16_cjk.c
	packages/lvgl-v8.3-latest/src/font/lv_font_unscii_16.c
	packages/lvgl-v8.3-latest/src/font/lv_font_unscii_8.c
	packages/lvgl-v8.3-latest/src/hal/lv_hal_disp.c
	packages/lvgl-v8.3-latest/src/hal/lv_hal_indev.c
	packages/lvgl-v8.3-latest/src/hal/lv_hal_tick.c
	packages/lvgl-v8.3-latest/src/misc/lv_anim.c
	packages/lvgl-v8.3-latest/src/misc/lv_anim_timeline.c
	packages/lvgl-v8.3-latest/src/misc/lv_area.c
	packages/lvgl-v8.3-latest/src/misc/lv_async.c
	packages/lvgl-v8.3-latest/src/misc/lv_bidi.c
	packages/lvgl-v8.3-latest/src/misc/lv_color.c
	packages/lvgl-v8.3-latest/src/misc/lv_fs.c
	packages/lvgl-v8.3-latest/src/misc/lv_gc.c
	packages/lvgl-v8.3-latest/src/misc/lv_ll.c
	packages/lvgl-v8.3-latest/src/misc/lv_log.c
	packages/lvgl-v8.3-latest/src/misc/lv_lru.c
	packages/lvgl-v8.3-latest/src/misc/lv_math.c
	packages/lvgl-v8.3-latest/src/misc/lv_mem.c
	packages/lvgl-v8.3-latest/src/misc/lv_printf.c
	packages/lvgl-v8.3-latest/src/misc/lv_style.c
	packages/lvgl-v8.3-latest/src/misc/lv_style_gen.c
	packages/lvgl-v8.3-latest/src/misc/lv_templ.c
	packages/lvgl-v8.3-latest/src/misc/lv_timer.c
	packages/lvgl-v8.3-latest/src/misc/lv_tlsf.c
	packages/lvgl-v8.3-latest/src/misc/lv_txt.c
	packages/lvgl-v8.3-latest/src/misc/lv_txt_ap.c
	packages/lvgl-v8.3-latest/src/misc/lv_utils.c
	packages/lvgl-v8.3-latest/src/widgets/lv_arc.c
	packages/lvgl-v8.3-latest/src/widgets/lv_bar.c
	packages/lvgl-v8.3-latest/src/widgets/lv_btn.c
	packages/lvgl-v8.3-latest/src/widgets/lv_btnmatrix.c
	packages/lvgl-v8.3-latest/src/widgets/lv_canvas.c
	packages/lvgl-v8.3-latest/src/widgets/lv_checkbox.c
	packages/lvgl-v8.3-latest/src/widgets/lv_dropdown.c
	packages/lvgl-v8.3-latest/src/widgets/lv_img.c
	packages/lvgl-v8.3-latest/src/widgets/lv_label.c
	packages/lvgl-v8.3-latest/src/widgets/lv_line.c
	packages/lvgl-v8.3-latest/src/widgets/lv_objx_templ.c
	packages/lvgl-v8.3-latest/src/widgets/lv_roller.c
	packages/lvgl-v8.3-latest/src/widgets/lv_slider.c
	packages/lvgl-v8.3-latest/src/widgets/lv_switch.c
	packages/lvgl-v8.3-latest/src/widgets/lv_table.c
	packages/lvgl-v8.3-latest/src/widgets/lv_textarea.c
	packages/lvgl-v8.3-latest/src/draw/arm2d/lv_gpu_arm2d.c
	packages/lvgl-v8.3-latest/src/draw/renesas/lv_gpu_d2_draw_label.c
	packages/lvgl-v8.3-latest/src/draw/renesas/lv_gpu_d2_ra6m3.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_arc.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_bg.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_composite.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_img.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_label.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_layer.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_line.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_mask.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_polygon.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_rect.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_stack_blur.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_texture_cache.c
	packages/lvgl-v8.3-latest/src/draw/sdl/lv_draw_sdl_utils.c
	packages/lvgl-v8.3-latest/src/draw/stm32_dma2d/lv_gpu_stm32_dma2d.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_arc.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_blend.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_dither.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_gradient.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_img.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_layer.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_letter.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_line.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_polygon.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_rect.c
	packages/lvgl-v8.3-latest/src/draw/sw/lv_draw_sw_transform.c
	packages/lvgl-v8.3-latest/src/draw/swm341_dma2d/lv_gpu_swm341_dma2d.c
	packages/lvgl-v8.3-latest/src/draw/nxp/pxp/lv_draw_pxp.c
	packages/lvgl-v8.3-latest/src/draw/nxp/pxp/lv_draw_pxp_blend.c
	packages/lvgl-v8.3-latest/src/draw/nxp/pxp/lv_gpu_nxp_pxp.c
	packages/lvgl-v8.3-latest/src/draw/nxp/pxp/lv_gpu_nxp_pxp_osa.c
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite/lv_draw_vglite.c
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite/lv_draw_vglite_arc.c
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite/lv_draw_vglite_blend.c
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite/lv_draw_vglite_line.c
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite/lv_draw_vglite_rect.c
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite/lv_vglite_buf.c
	packages/lvgl-v8.3-latest/src/draw/nxp/vglite/lv_vglite_utils.c
	packages/lvgl-v8.3-latest/src/extra/layouts/flex/lv_flex.c
	packages/lvgl-v8.3-latest/src/extra/layouts/grid/lv_grid.c
	packages/lvgl-v8.3-latest/src/extra/libs/bmp/lv_bmp.c
	packages/lvgl-v8.3-latest/src/extra/libs/ffmpeg/lv_ffmpeg.c
	packages/lvgl-v8.3-latest/src/extra/libs/freetype/lv_freetype.c
	packages/lvgl-v8.3-latest/src/extra/libs/fsdrv/lv_fs_fatfs.c
	packages/lvgl-v8.3-latest/src/extra/libs/fsdrv/lv_fs_littlefs.c
	packages/lvgl-v8.3-latest/src/extra/libs/fsdrv/lv_fs_posix.c
	packages/lvgl-v8.3-latest/src/extra/libs/fsdrv/lv_fs_stdio.c
	packages/lvgl-v8.3-latest/src/extra/libs/fsdrv/lv_fs_win32.c
	packages/lvgl-v8.3-latest/src/extra/libs/gif/gifdec.c
	packages/lvgl-v8.3-latest/src/extra/libs/gif/lv_gif.c
	packages/lvgl-v8.3-latest/src/extra/libs/png/lodepng.c
	packages/lvgl-v8.3-latest/src/extra/libs/png/lv_png.c
	packages/lvgl-v8.3-latest/src/extra/libs/qrcode/lv_qrcode.c
	packages/lvgl-v8.3-latest/src/extra/libs/qrcode/qrcodegen.c
	packages/lvgl-v8.3-latest/src/extra/libs/rlottie/lv_rlottie.c
	packages/lvgl-v8.3-latest/src/extra/libs/sjpg/lv_sjpg.c
	packages/lvgl-v8.3-latest/src/extra/libs/sjpg/tjpgd.c
	packages/lvgl-v8.3-latest/src/extra/libs/tiny_ttf/lv_tiny_ttf.c
	packages/lvgl-v8.3-latest/src/extra/others/fragment/lv_fragment.c
	packages/lvgl-v8.3-latest/src/extra/others/fragment/lv_fragment_manager.c
	packages/lvgl-v8.3-latest/src/extra/others/gridnav/lv_gridnav.c
	packages/lvgl-v8.3-latest/src/extra/others/ime/lv_ime_pinyin.c
	packages/lvgl-v8.3-latest/src/extra/others/imgfont/lv_imgfont.c
	packages/lvgl-v8.3-latest/src/extra/others/monkey/lv_monkey.c
	packages/lvgl-v8.3-latest/src/extra/others/msg/lv_msg.c
	packages/lvgl-v8.3-latest/src/extra/others/snapshot/lv_snapshot.c
	packages/lvgl-v8.3-latest/src/extra/themes/basic/lv_theme_basic.c
	packages/lvgl-v8.3-latest/src/extra/themes/default/lv_theme_default.c
	packages/lvgl-v8.3-latest/src/extra/themes/mono/lv_theme_mono.c
	packages/lvgl-v8.3-latest/src/extra/widgets/animimg/lv_animimg.c
	packages/lvgl-v8.3-latest/src/extra/widgets/calendar/lv_calendar.c
	packages/lvgl-v8.3-latest/src/extra/widgets/calendar/lv_calendar_header_arrow.c
	packages/lvgl-v8.3-latest/src/extra/widgets/calendar/lv_calendar_header_dropdown.c
	packages/lvgl-v8.3-latest/src/extra/widgets/chart/lv_chart.c
	packages/lvgl-v8.3-latest/src/extra/widgets/colorwheel/lv_colorwheel.c
	packages/lvgl-v8.3-latest/src/extra/widgets/imgbtn/lv_imgbtn.c
	packages/lvgl-v8.3-latest/src/extra/widgets/keyboard/lv_keyboard.c
	packages/lvgl-v8.3-latest/src/extra/widgets/led/lv_led.c
	packages/lvgl-v8.3-latest/src/extra/widgets/list/lv_list.c
	packages/lvgl-v8.3-latest/src/extra/widgets/menu/lv_menu.c
	packages/lvgl-v8.3-latest/src/extra/widgets/meter/lv_meter.c
	packages/lvgl-v8.3-latest/src/extra/widgets/msgbox/lv_msgbox.c
	packages/lvgl-v8.3-latest/src/extra/widgets/span/lv_span.c
	packages/lvgl-v8.3-latest/src/extra/widgets/spinbox/lv_spinbox.c
	packages/lvgl-v8.3-latest/src/extra/widgets/spinner/lv_spinner.c
	packages/lvgl-v8.3-latest/src/extra/widgets/tabview/lv_tabview.c
	packages/lvgl-v8.3-latest/src/extra/widgets/tileview/lv_tileview.c
	packages/lvgl-v8.3-latest/src/extra/widgets/win/lv_win.c
)

SET(RT_LVGL-DEMO_SOURCES
	applications/lvgl/demo/lv_demo.c
)

SET(RT_LVGL-NET-TESTER_SOURCES
	applications/lvgl/net_tester/net_tester_ascii_input.c
	applications/lvgl/net_tester/net_tester_about.c
	applications/lvgl/net_tester/net_tester_crimp.c
	applications/lvgl/net_tester/net_tester_press.c
	applications/lvgl/net_tester/net_tester_ip_input.c
	applications/lvgl/net_tester/net_tester_netdbg.c
	applications/lvgl/net_tester/net_tester_netops.c
	applications/lvgl/net_tester/net_tester_pair.c
	applications/lvgl/net_tester/net_tester_trace.c
	applications/lvgl/net_tester/net_tester_ui.c
)

SET(RT_LVGL-PORT_SOURCES
	applications/lvgl/lv_font_hzk.c
	applications/lvgl/lv_port_disp.c
	applications/lvgl/lv_port_indev.c
	packages/lvgl-v8.3-latest/env_support/rt-thread/lv_rt_thread_port.c
)

SET(RT_LWIP_SOURCES
	../../../components/net/lwip/lwip-2.1.2/src/api/api_lib.c
	../../../components/net/lwip/lwip-2.1.2/src/api/api_msg.c
	../../../components/net/lwip/lwip-2.1.2/src/api/err.c
	../../../components/net/lwip/lwip-2.1.2/src/api/if_api.c
	../../../components/net/lwip/lwip-2.1.2/src/api/netbuf.c
	../../../components/net/lwip/lwip-2.1.2/src/api/netdb.c
	../../../components/net/lwip/lwip-2.1.2/src/api/netifapi.c
	../../../components/net/lwip/lwip-2.1.2/src/api/sockets.c
	../../../components/net/lwip/lwip-2.1.2/src/api/tcpip.c
	../../../components/net/lwip/lwip-2.1.2/src/apps/ping/ping.c
	../../../components/net/lwip/lwip-2.1.2/src/core/altcp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/altcp_alloc.c
	../../../components/net/lwip/lwip-2.1.2/src/core/altcp_tcp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/def.c
	../../../components/net/lwip/lwip-2.1.2/src/core/dns.c
	../../../components/net/lwip/lwip-2.1.2/src/core/inet_chksum.c
	../../../components/net/lwip/lwip-2.1.2/src/core/init.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ip.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/autoip.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/dhcp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/etharp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/icmp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/igmp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/ip4.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/ip4_addr.c
	../../../components/net/lwip/lwip-2.1.2/src/core/ipv4/ip4_frag.c
	../../../components/net/lwip/lwip-2.1.2/src/core/memp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/netif.c
	../../../components/net/lwip/lwip-2.1.2/src/core/pbuf.c
	../../../components/net/lwip/lwip-2.1.2/src/core/raw.c
	../../../components/net/lwip/lwip-2.1.2/src/core/stats.c
	../../../components/net/lwip/lwip-2.1.2/src/core/sys.c
	../../../components/net/lwip/lwip-2.1.2/src/core/tcp.c
	../../../components/net/lwip/lwip-2.1.2/src/core/tcp_in.c
	../../../components/net/lwip/lwip-2.1.2/src/core/tcp_out.c
	../../../components/net/lwip/lwip-2.1.2/src/core/timeouts.c
	../../../components/net/lwip/lwip-2.1.2/src/core/udp.c
	../../../components/net/lwip/lwip-2.1.2/src/netif/ethernet.c
	../../../components/net/lwip/lwip-2.1.2/src/netif/lowpan6.c
	../../../components/net/lwip/port/ethernetif.c
	../../../components/net/lwip/port/sys_arch.c
)

SET(RT_RASPBERRYPI-PICO-RP2350-SDK_SOURCES
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/cmsis/stub/CMSIS/Device/RP2350/Source/system_RP2350.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_stdlib/stdlib.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_gpio/gpio.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2350/pico_platform/platform.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_platform_panic/panic.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/hardware_claim/claim.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_sync/sync.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_sync_spin_lock/sync_spin_lock.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_irq/irq.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_irq/irq_handler_chain.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_sync/sem.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_sync/lock_core.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_sync/mutex.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_sync/critical_section.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_time/time.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_time/timeout_helper.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_timer/timer.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_util/datetime.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_util/pheap.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/common/pico_util/queue.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_uart/uart.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_clocks/clocks.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_pll/pll.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_vreg/vreg.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_watchdog/watchdog.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_ticks/ticks.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_bootrom/bootrom.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_bootrom/bootrom_lock.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_boot_lock/boot_lock.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_flash/flash.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_unique_id/unique_id.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_xosc/xosc.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_divider/divider.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_runtime/runtime.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_runtime_init/runtime_init.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_runtime_init/runtime_init_clocks.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_runtime_init/runtime_init_stack_guard.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_bit_ops/bit_ops_aeabi.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_divider/divider_compiler.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_double/double_math.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_double/double_aeabi_dcp.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_double/double_fma_dcp.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_double/double_sci_m33.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_double/double_conv_m33.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_float/float_math.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_float/float_conv32_vfp.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_float/float_common_m33.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_float/float_sci_m33_vfp.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_malloc/malloc.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_atomic/atomic.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_cxx_options/new_delete.cpp
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_standard_binary_info/standard_binary_info.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_printf/printf.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_crt0/crt0.S
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_clib_interface/newlib_interface.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_stdio/stdio.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/pico_stdio_uart/stdio_uart.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_spi/spi.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_dma/dma.c
	packages/raspberrypi-pico-rp2350-sdk-latest/src/rp2_common/hardware_adc/adc.c
	packages/raspberrypi-pico-rp2350-sdk-latest/bs2_default_padded_checksummed.S
)

SET(RT_SAL_SOURCES
	../../../components/net/netdev/src/netdev.c
	../../../components/net/netdev/src/netdev_ipaddr.c
)

SET(RT_UTC_UTEST_SOURCES
)

SET(RT_UTESTCASES_SOURCES
)

# Library search paths
# Library local macro definitions
SET(RT_DEVICEDRIVERS_DEFINES
	__RT_IPC_SOURCE__
)

SET(RT_KERNEL_DEFINES
	__RT_KERNEL_SOURCE__
)

# Library dependencies
SET(RT_LIBC_LIBS
	c
	m
)

# Libraries
ADD_LIBRARY(rtt_CPU OBJECT ${RT_CPU_SOURCES})
ADD_LIBRARY(rtt_DeviceDrivers OBJECT ${RT_DEVICEDRIVERS_SOURCES})
ADD_LIBRARY(rtt_Drivers OBJECT ${RT_DRIVERS_SOURCES})
ADD_LIBRARY(rtt_Finsh OBJECT ${RT_FINSH_SOURCES})
ADD_LIBRARY(rtt_Kernel OBJECT ${RT_KERNEL_SOURCES})
ADD_LIBRARY(rtt_Libc OBJECT ${RT_LIBC_SOURCES})
ADD_LIBRARY(rtt_LVGL OBJECT ${RT_LVGL_SOURCES})
ADD_LIBRARY(rtt_LVGL-demo OBJECT ${RT_LVGL-DEMO_SOURCES})
ADD_LIBRARY(rtt_LVGL-net-tester OBJECT ${RT_LVGL-NET-TESTER_SOURCES})
ADD_LIBRARY(rtt_LVGL-port OBJECT ${RT_LVGL-PORT_SOURCES})
ADD_LIBRARY(rtt_lwIP OBJECT ${RT_LWIP_SOURCES})
ADD_LIBRARY(rtt_raspberrypi-pico-rp2350-sdk OBJECT ${RT_RASPBERRYPI-PICO-RP2350-SDK_SOURCES})
ADD_LIBRARY(rtt_SAL OBJECT ${RT_SAL_SOURCES})

# Interface libraries
ADD_LIBRARY(rtt_utc_UTest INTERFACE)
ADD_LIBRARY(rtt_utestcases INTERFACE)

# Private macros
TARGET_COMPILE_DEFINITIONS(rtt_DeviceDrivers PRIVATE ${RT_DEVICEDRIVERS_DEFINES})
TARGET_COMPILE_DEFINITIONS(rtt_Kernel PRIVATE ${RT_KERNEL_DEFINES})

# Interface library search paths
TARGET_LINK_LIBRARIES(rtt_Libc INTERFACE ${RT_LIBC_LIBS})

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME}.elf ${RT_APPLICATIONS_SOURCES})
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME}.elf
	rtt_CPU
	rtt_DeviceDrivers
	rtt_Drivers
	rtt_Finsh
	rtt_Kernel
	rtt_Libc
	rtt_LVGL
	rtt_LVGL-demo
	rtt_LVGL-net-tester
	rtt_LVGL-port
	rtt_lwIP
	rtt_raspberrypi-pico-rp2350-sdk
	rtt_SAL
	rtt_utc_UTest
	rtt_utestcases
)

ADD_CUSTOM_COMMAND(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD 
	COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf rtthread.bin
	COMMAND ${CMAKE_SIZE} ${CMAKE_PROJECT_NAME}.elf
	COMMAND "D:/temp/picotool/picotool/picotool.exe" uf2 convert ${CMAKE_PROJECT_NAME}.elf rtthread.uf2 --family rp2350-arm-s --abs-block
)

# if custom.cmake is exist, add it
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/custom.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/custom.cmake)
endif()
