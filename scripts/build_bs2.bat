@echo off
setlocal
set GCC=C:\Program Files\Arm\GNU Toolchain mingw-w64-x86_64-arm-none-eabi\bin
set SDK=%~dp0..\packages\raspberrypi-pico-rp2350-sdk-latest
cd /d "%SDK%"

"%GCC%\arm-none-eabi-gcc.exe" -mcpu=cortex-m33 -mthumb -march=armv8-m.main+fp+dsp -mfloat-abi=softfp -mcmse -g -O3 -Wl,--build-id=none --specs=nosys.specs -nostartfiles -Wl,--script=src/rp2350/boot_stage2/boot_stage2.ld -DPICO_RP2350=1 -DPICO_32BIT=1 -DPICO_BUILD=1 -DPICO_ON_DEVICE=1 -DPICO_NO_HARDWARE=0 -DLIB_BOOT_STAGE2_HEADERS=1 -Isrc/rp2350/boot_stage2/include -Isrc/rp2350/boot_stage2/asminclude -Isrc/common/pico_base_headers/include -Isrc/boards/include -Isrc/rp2350/pico_platform/include -Isrc/rp2350/hardware_regs/include -Isrc/rp2_common/cmsis/include -Isrc/rp2_common/pico_platform_compiler/include -Isrc/rp2_common/pico_platform_panic/include -Isrc/rp2_common/pico_platform_sections/include -c src/rp2350/boot_stage2/compile_time_choice.S -o bs2_default.o
if errorlevel 1 exit /b 1

"%GCC%\arm-none-eabi-gcc.exe" -mcpu=cortex-m33 -mthumb -march=armv8-m.main+fp+dsp -mfloat-abi=softfp -mcmse -g -O3 --specs=nosys.specs -nostartfiles -Wl,--script=src/rp2350/boot_stage2/boot_stage2.ld bs2_default.o -o bs2_default.elf
if errorlevel 1 exit /b 1

"%GCC%\arm-none-eabi-objcopy.exe" -Obinary bs2_default.elf bs2_default.bin
if errorlevel 1 exit /b 1

python src/rp2350/boot_stage2/pad_checksum -s 0xffffffff bs2_default.bin bs2_default_padded_checksummed.S
if errorlevel 1 exit /b 1

echo boot stage2 build OK
