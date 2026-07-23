#Requires -Version 5.1
<#
.SYNOPSIS
  按 deps.json 恢复可编译的 RP2350B 网络巡线对线仪工程。

.DESCRIPTION
  1) 检出固定 commit 的 RT-Thread / Pico RP2350 SDK / LVGL
  2) 覆盖本仓库 BSP 增量与驱动补丁
  3) 复制 HZK 字库与构建脚本
  4) 可选：安装环境并执行 build.bat

.EXAMPLE
  .\scripts\restore_workspace.ps1
  .\scripts\restore_workspace.ps1 -OutDir D:\work\RP2350B-RT-Thread -Build
  .\scripts\restore_workspace.ps1 -OutDir ..\RP2350B-RT-Thread -SkipBuild
#>
[CmdletBinding()]
param(
    [string]$OutDir = "",
    [switch]$SkipBuild,
    [string]$ArmGccRoot = ""
)

$ErrorActionPreference = "Stop"
$Build = -not $SkipBuild

function Write-Step([string]$msg) {
    Write-Host ""
    Write-Host "==== $msg ====" -ForegroundColor Cyan
}

function Ensure-Dir([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

function Invoke-GitCheckoutCommit {
    param(
        [string]$Url,
        [string]$FallbackUrl,
        [string]$Commit,
        [string]$Dest
    )

    if (Test-Path -LiteralPath (Join-Path $Dest ".git")) {
        Write-Host "已存在仓库: $Dest ，切换到 $Commit"
        Push-Location $Dest
        try {
            git fetch --all --tags 2>$null | Out-Null
            git checkout --force $Commit 2>&1 | Out-Host
            if ($LASTEXITCODE -ne 0) {
                git fetch origin $Commit 2>&1 | Out-Host
                git checkout --force $Commit 2>&1 | Out-Host
            }
            if ($LASTEXITCODE -ne 0) { throw "无法 checkout $Commit @ $Dest" }
        } finally {
            Pop-Location
        }
        return
    }

    Ensure-Dir (Split-Path -Parent $Dest)
    if (Test-Path -LiteralPath $Dest) {
        Remove-Item -LiteralPath $Dest -Recurse -Force
    }

    $urls = @($Url)
    if ($FallbackUrl) { $urls += $FallbackUrl }

    $ok = $false
    foreach ($u in $urls) {
        Write-Host "克隆 $u -> $Dest"
        New-Item -ItemType Directory -Force -Path $Dest | Out-Null
        Push-Location $Dest
        try {
            git init | Out-Null
            git remote add origin $u 2>$null | Out-Null
            git fetch --depth 1 origin $Commit 2>&1 | Out-Host
            if ($LASTEXITCODE -eq 0) {
                git checkout --force FETCH_HEAD 2>&1 | Out-Host
                if ($LASTEXITCODE -eq 0) { $ok = $true; break }
            }
            # 深度不足时退回浅完整拉取再 checkout
            Write-Host "depth-1 失败，尝试完整 fetch..."
            git fetch origin 2>&1 | Out-Host
            git checkout --force $Commit 2>&1 | Out-Host
            if ($LASTEXITCODE -eq 0) { $ok = $true; break }
        } catch {
            Write-Host "尝试 $u 失败: $_" -ForegroundColor Yellow
        } finally {
            Pop-Location
        }
        if (-not $ok -and (Test-Path -LiteralPath $Dest)) {
            Remove-Item -LiteralPath $Dest -Recurse -Force
        }
    }

    if (-not $ok) {
        throw "克隆失败: $Url (commit=$Commit)"
    }
}

function New-JunctionSafe([string]$Link, [string]$Target) {
    if (Test-Path -LiteralPath $Link) {
        cmd /c "rmdir `"$Link`"" | Out-Null
        if (Test-Path -LiteralPath $Link) {
            Remove-Item -LiteralPath $Link -Force -Recurse -ErrorAction SilentlyContinue
        }
    }
    Ensure-Dir (Split-Path -Parent $Link)
    cmd /c "mklink /J `"$Link`" `"$Target`"" | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "创建 junction 失败: $Link -> $Target" }
}

function Find-ArmGccRoot {
    param([string]$Preferred)
    if ($Preferred -and (Test-Path -LiteralPath (Join-Path $Preferred "bin\arm-none-eabi-gcc.exe"))) {
        return (Resolve-Path $Preferred).Path
    }
    $cmd = Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue
    if ($cmd) {
        return (Split-Path (Split-Path $cmd.Source -Parent) -Parent)
    }
    $default = "C:\Program Files\Arm\GNU Toolchain mingw-w64-x86_64-arm-none-eabi"
    if (Test-Path -LiteralPath (Join-Path $default "bin\arm-none-eabi-gcc.exe")) {
        return $default
    }
    return $null
}

function Patch-ToolchainPaths {
    param([string]$Workspace, [string]$GccRoot)
    if (-not $GccRoot) { return }

    $posix = ($GccRoot -replace '\\', '/')
    $files = @(
        (Join-Path $Workspace "rt-thread\bsp\raspberry-pico\RP2350\cmake\arm-gcc-toolchain.cmake"),
        (Join-Path $Workspace "rt-thread\bsp\raspberry-pico\RP2350\CMakeLists.txt"),
        (Join-Path $Workspace "scripts\build_bs2.bat"),
        (Join-Path $Workspace "build.bat")
    )

    foreach ($f in $files) {
        if (-not (Test-Path -LiteralPath $f)) { continue }
        $text = Get-Content -LiteralPath $f -Raw -Encoding UTF8
        $new = $text
        # cmake TOOLCHAIN_ROOT / hardcoded Program Files path
        $new = [regex]::Replace($new,
            'C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi',
            $posix)
        $new = [regex]::Replace($new,
            'C:\\Program Files\\Arm\\GNU Toolchain mingw-w64-x86_64-arm-none-eabi',
            $GccRoot)
        if ($new -ne $text) {
            Set-Content -LiteralPath $f -Value $new -Encoding UTF8 -NoNewline
            Write-Host "已改写工具链路径: $f"
        }
    }
}

# ---------- 路径 ----------
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
if (-not $OutDir) {
    $OutDir = Join-Path (Split-Path $RepoRoot -Parent) "RP2350B-RT-Thread"
}
$OutDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)

$depsPath = Join-Path $RepoRoot "deps.json"
if (-not (Test-Path -LiteralPath $depsPath)) {
    throw "缺少 deps.json: $depsPath"
}
$deps = Get-Content -LiteralPath $depsPath -Raw -Encoding UTF8 | ConvertFrom-Json

Write-Step "恢复工程到: $OutDir"
Write-Host "仓库增量: $RepoRoot"
Ensure-Dir $OutDir

# ---------- 1. RT-Thread ----------
Write-Step "检出 RT-Thread $($deps.'rt-thread'.version) @ $($deps.'rt-thread'.commit.Substring(0,8))"
$rtt = Join-Path $OutDir "rt-thread"
Invoke-GitCheckoutCommit -Url $deps.'rt-thread'.url `
    -FallbackUrl $deps.'rt-thread'.url_fallback `
    -Commit $deps.'rt-thread'.commit `
    -Dest $rtt

# ---------- 2. Pico RP2350 SDK 包 ----------
Write-Step "检出 Pico RP2350 SDK $($deps.pico_rp2350_sdk.version) @ $($deps.pico_rp2350_sdk.commit.Substring(0,8))"
$sdkName = $deps.pico_rp2350_sdk.package_name
$sdkDest = Join-Path $OutDir "packages\$sdkName"
Invoke-GitCheckoutCommit -Url $deps.pico_rp2350_sdk.url `
    -FallbackUrl $null `
    -Commit $deps.pico_rp2350_sdk.commit `
    -Dest $sdkDest

# ---------- 3. 覆盖本仓库增量 ----------
Write-Step "覆盖 BSP 增量与驱动补丁"
$overlayRt = Join-Path $RepoRoot "rt-thread"
robocopy $overlayRt $rtt /E /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
# robocopy 退出码 0-7 成功
if ($LASTEXITCODE -ge 8) { throw "robocopy 覆盖 rt-thread 失败, code=$LASTEXITCODE" }

# ---------- 4. LVGL ----------
Write-Step "检出 LVGL $($deps.lvgl.version) @ $($deps.lvgl.commit.Substring(0,8))"
$pkgDir = Join-Path $rtt "bsp\raspberry-pico\RP2350\packages"
Ensure-Dir $pkgDir
$lvglDest = Join-Path $pkgDir $deps.lvgl.package_name
Invoke-GitCheckoutCommit -Url $deps.lvgl.url `
    -FallbackUrl $deps.lvgl.url_fallback `
    -Commit $deps.lvgl.commit `
    -Dest $lvglDest

# ---------- 5. SDK junction ----------
Write-Step "链接 Pico SDK 包到 BSP packages"
$sdkLink = Join-Path $pkgDir $sdkName
New-JunctionSafe -Link $sdkLink -Target $sdkDest

# ---------- 6. 字库与构建脚本 ----------
Write-Step "复制字库与构建脚本"
Copy-Item -LiteralPath (Join-Path $RepoRoot "assets\HZK12") -Destination (Join-Path $OutDir "HZK12") -Force
Copy-Item -LiteralPath (Join-Path $RepoRoot "assets\HZK16") -Destination (Join-Path $OutDir "HZK16") -Force
Copy-Item -LiteralPath (Join-Path $RepoRoot "build.bat") -Destination (Join-Path $OutDir "build.bat") -Force
Ensure-Dir (Join-Path $OutDir "scripts")
Copy-Item -LiteralPath (Join-Path $RepoRoot "scripts\setup_env.bat") -Destination (Join-Path $OutDir "scripts\setup_env.bat") -Force
Copy-Item -LiteralPath (Join-Path $RepoRoot "scripts\build_bs2.bat") -Destination (Join-Path $OutDir "scripts\build_bs2.bat") -Force
Copy-Item -LiteralPath (Join-Path $RepoRoot "scripts\pair_scan_analyze.py") -Destination (Join-Path $OutDir "scripts\pair_scan_analyze.py") -Force -ErrorAction SilentlyContinue
Copy-Item -LiteralPath (Join-Path $RepoRoot "scripts\seq_scan_analyze.py") -Destination (Join-Path $OutDir "scripts\seq_scan_analyze.py") -Force -ErrorAction SilentlyContinue
Copy-Item -LiteralPath (Join-Path $RepoRoot "VERSIONS.md") -Destination (Join-Path $OutDir "VERSIONS.md") -Force
Copy-Item -LiteralPath (Join-Path $RepoRoot "deps.json") -Destination (Join-Path $OutDir "deps.json") -Force

# ---------- 7. 工具链路径 ----------
Write-Step "检测 ARM GCC 工具链"
$gccRoot = Find-ArmGccRoot -Preferred $ArmGccRoot
if (-not $gccRoot) {
    Write-Host "警告: 未找到 arm-none-eabi-gcc，请安装 Arm GNU Toolchain 后重试。" -ForegroundColor Yellow
} else {
    Write-Host "ARM GCC: $gccRoot"
    Patch-ToolchainPaths -Workspace $OutDir -GccRoot $gccRoot
}

# ---------- 8. 编译 ----------
if ($Build) {
    Write-Step "准备环境 (ninja / scons)"
    Push-Location $OutDir
    try {
        cmd /c "scripts\setup_env.bat"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "setup_env 有警告/失败，继续尝试编译..." -ForegroundColor Yellow
        }

        Write-Step "编译固件"
        cmd /c "build.bat"
        if ($LASTEXITCODE -ne 0) {
            throw "build.bat 失败 (exit=$LASTEXITCODE)"
        }

        $elf = Join-Path $OutDir "rt-thread\bsp\raspberry-pico\RP2350\build-ninja\rtthread.elf"
        if (-not (Test-Path -LiteralPath $elf)) {
            throw "未找到产物: $elf"
        }
        Write-Host ""
        Write-Host "编译成功: $elf" -ForegroundColor Green
    } finally {
        Pop-Location
    }
} else {
    Write-Host ""
    Write-Host "已跳过编译。可手动执行:" -ForegroundColor Yellow
    Write-Host "  cd `"$OutDir`""
    Write-Host "  scripts\setup_env.bat"
    Write-Host "  build.bat"
}

Write-Step "完成"
Write-Host "工作区: $OutDir"
