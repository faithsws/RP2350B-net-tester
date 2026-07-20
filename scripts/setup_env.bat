@echo off
setlocal
set ROOT=%~dp0..
set TOOLS=%ROOT%\tools

if not exist "%TOOLS%" mkdir "%TOOLS%"

if not exist "%TOOLS%\ninja.exe" (
    echo 下载 ninja ...
    curl -L --noproxy "*" -o "%TOOLS%\ninja-win.zip" "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip"
    if errorlevel 1 (
        echo ninja 下载失败，请手动下载并解压到 tools\ninja.exe
        exit /b 1
    )
    powershell -Command "Expand-Archive -Force '%TOOLS%\ninja-win.zip' -DestinationPath '%TOOLS%'"
)

pip show scons >nul 2>&1
if errorlevel 1 (
    echo 安装 scons ...
    pip install scons -i https://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn
)

echo 环境检查:
where arm-none-eabi-gcc
"%TOOLS%\ninja.exe" --version
scons --version
echo 完成
