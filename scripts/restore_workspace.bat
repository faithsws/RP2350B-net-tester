@echo off
setlocal
set ROOT=%~dp0..
set OUTDIR=%~1
if "%OUTDIR%"=="" set OUTDIR=%ROOT%\..\RP2350B-RT-Thread

echo 恢复工程到: %OUTDIR%
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0restore_workspace.ps1" -OutDir "%OUTDIR%" -Build
if errorlevel 1 (
    echo.
    echo 恢复/编译失败。可先只恢复不编译:
    echo   powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0restore_workspace.ps1" -OutDir "%OUTDIR%" -SkipBuild
    exit /b 1
)
exit /b 0
