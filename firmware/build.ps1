<#
.SYNOPSIS
  Oracle OS 固件构建/烧录脚本(唯一入口)。

.DESCRIPTION
  取代了以前散落的 6 个脚本(build.bat / build.ps1 / build_firmware.ps1 /
  compile.ps1 / flash.bat 等)。那些脚本把 IDF 路径、工具链路径、项目路径和
  COM 口全写死在各自文件里,换机器必然失败,而且几份的路径互相矛盾。
  它们已归档到 _attic/build-scripts/(仅本地保留)。

  本脚本不写死任何路径:
    - 项目路径     : 用脚本自身位置推导($PSScriptRoot)
    - ESP-IDF 路径 : 读 $env:IDF_PATH,没有则按常见位置搜索
    - espp 路径    : 交给 CMakeLists.txt 处理($env:ESPP_PATH)
    - COM 口       : -Port 参数,默认 COM8

.PARAMETER Action
  build    只编译(默认)
  flash    编译并烧录
  monitor  只看串口(Ctrl+] 退出)
  fm       烧录后立刻进串口监视
  clean    删掉 build 目录重新来

.PARAMETER Port
  串口号,默认 COM8。板子自带 Type-C 原生 USB(VID 303A)。

.EXAMPLE
  .\build.ps1
  .\build.ps1 -Action flash -Port COM8
  .\build.ps1 -Action fm
#>
[CmdletBinding()]
param(
    [ValidateSet('build', 'flash', 'monitor', 'fm', 'clean')]
    [string]$Action = 'build',

    [string]$Port = 'COM8'
)

$ErrorActionPreference = 'Stop'

# ESP-IDF 里的 Python 默认按系统 ANSI 代码页输出。串口报错(比如"端口被占用")
# 里带中文/特殊字符时会触发 UnicodeEncodeError,把真实原因盖成一堆 traceback。
# 这两个变量是踩过坑的,别删。
$env:PYTHONIOENCODING = 'utf-8'
$env:PYTHONUTF8 = '1'

$projectDir = Join-Path $PSScriptRoot 'esp32s3_knob_idf'
if (-not (Test-Path (Join-Path $projectDir 'CMakeLists.txt'))) {
    throw "找不到固件工程: $projectDir"
}

# ---- 定位 ESP-IDF ------------------------------------------------------------
function Resolve-IdfPath {
    if ($env:IDF_PATH -and (Test-Path (Join-Path $env:IDF_PATH 'tools\idf.py'))) {
        return $env:IDF_PATH
    }
    foreach ($p in @(
            'D:\esp\esp-idf',
            'C:\esp\esp-idf',
            'C:\Espressif\frameworks\esp-idf-v5.5.1',
            "$env:USERPROFILE\esp\esp-idf")) {
        if (Test-Path (Join-Path $p 'tools\idf.py')) { return $p }
    }
    return $null
}

# idf.py 已经在 PATH 里(比如从 "ESP-IDF PowerShell" 启动的)就直接用,
# 否则 dot-source export.ps1 把环境导进当前会话。
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    $idfPath = Resolve-IdfPath
    if (-not $idfPath) {
        $msg = '找不到 ESP-IDF。请任选一种:' + [Environment]::NewLine +
        '  1) 用开始菜单里的 "ESP-IDF 5.5 PowerShell" 打开终端,再跑本脚本' + [Environment]::NewLine +
        '  2) 设置环境变量后重跑:  $env:IDF_PATH = "D:\esp\esp-idf"' + [Environment]::NewLine +
        '本项目验证过的版本是 ESP-IDF v5.5.1。'
        throw $msg
    }
    $exportPs1 = Join-Path $idfPath 'export.ps1'
    if (-not (Test-Path $exportPs1)) { throw "缺少 $exportPs1" }
    Write-Host "激活 ESP-IDF: $idfPath" -ForegroundColor DarkGray
    . $exportPs1 | Out-Null
}

if (-not $env:ESPP_PATH) {
    Write-Host "提示: 未设 ESPP_PATH,CMake 会自动搜索常见位置。" -ForegroundColor DarkYellow
    Write-Host "      如果报找不到 espp,设置后重跑: `$env:ESPP_PATH = `"D:\esp\espp`"" -ForegroundColor DarkYellow
}

Push-Location $projectDir
try {
    Write-Host ""
    Write-Host "Oracle OS 固件 | $Action | $projectDir" -ForegroundColor Cyan
    Write-Host ""

    switch ($Action) {
        'build' {
            idf.py build
            if ($LASTEXITCODE -ne 0) { throw "编译失败" }
            idf.py size
        }
        'flash' {
            idf.py -p $Port flash
            if ($LASTEXITCODE -ne 0) { throw "烧录失败(端口 $Port)" }
        }
        'monitor' {
            idf.py -p $Port monitor
        }
        'fm' {
            idf.py -p $Port flash monitor
            if ($LASTEXITCODE -ne 0) { throw "烧录失败(端口 $Port)" }
        }
        'clean' {
            if (Test-Path 'build') {
                Remove-Item -Recurse -Force 'build'
                Write-Host "build/ 已删除" -ForegroundColor Green
            }
            else {
                Write-Host "build/ 本来就不存在" -ForegroundColor DarkGray
            }
        }
    }

    Write-Host ""
    Write-Host "完成: $Action" -ForegroundColor Green
}
finally {
    Pop-Location
}
