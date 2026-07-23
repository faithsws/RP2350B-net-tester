@echo off
setlocal
set ROOT=%~dp0
set BSP=%ROOT%rt-thread\bsp\raspberry-pico\RP2350
set TOOLCHAIN=C:\Program Files\Arm\GNU Toolchain mingw-w64-x86_64-arm-none-eabi\bin
set NINJA=%ROOT%tools\ninja.exe

if not exist "%NINJA%" (
    echo [ERROR] 未找到 ninja.exe，请先运行 scripts\setup_env.bat
    exit /b 1
)

echo [1/3] 构建 RP2350 boot stage2 ...
call "%ROOT%scripts\build_bs2.bat"
if errorlevel 1 exit /b 1

echo [2/3] 生成 CMake 工程 ...
set RTT_ROOT=%ROOT%rt-thread
set RTT_EXEC_PATH=%TOOLCHAIN%
cd /d "%BSP%"
scons --target=cmake >nul 2>&1

echo [3/3] CMake 编译 ...
if exist build-ninja rmdir /s /q build-ninja
cmake -S . -B build-ninja -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake ^
    -DCMAKE_MAKE_PROGRAM="%NINJA%"
if errorlevel 1 exit /b 1

cmake --build build-ninja
if errorlevel 1 exit /b 1

echo.
echo 编译成功: %BSP%\build-ninja\rtthread.elf
exit /b 0
