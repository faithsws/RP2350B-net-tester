$ErrorActionPreference = "Stop"
$GCC = "C:\Program Files\Arm\GNU Toolchain mingw-w64-x86_64-arm-none-eabi\bin"
$SDK = "D:\000_git_repos\98_MISC\网络巡线对线仪\02_software\RP2350B-RT-Thread\packages\raspberrypi-pico-rp2350-sdk-latest"
Set-Location $SDK

$CFLAGS = @(
    "-mcpu=cortex-m33",
    "-mthumb",
    "-march=armv8-m.main+fp+dsp",
    "-mfloat-abi=softfp",
    "-mcmse",
    "-g",
    "-O3",
    "-Wl,--build-id=none",
    "--specs=nosys.specs",
    "-nostartfiles",
    "-Wl,--script=src/rp2350/boot_stage2/boot_stage2.ld",
    "-Wl,-Map=bs2_default.elf.map"
)

& "$GCC\arm-none-eabi-gcc.exe" @CFLAGS -c src/rp2350/boot_stage2/compile_time_choice.S -o bs2_default.o
& "$GCC\arm-none-eabi-gcc.exe" @CFLAGS bs2_default.o -o bs2_default.elf
& "$GCC\arm-none-eabi-objcopy.exe" -Obinary bs2_default.elf bs2_default.bin
python src/rp2350/boot_stage2/pad_checksum -s 0xffffffff bs2_default.bin bs2_default_padded_checksummed.S
Write-Host "boot stage2 done: $(Test-Path bs2_default_padded_checksummed.S)"
