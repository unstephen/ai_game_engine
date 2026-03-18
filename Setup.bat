@echo off
REM =============================================================================
REM 环境设置脚本 (Windows)
REM 用于安装依赖和配置开发环境
REM =============================================================================

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   Engine Environment Setup
echo ========================================
echo.

REM 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM 检查管理员权限
net session >nul 2>&1
if errorlevel 1 (
    echo [WARN] 建议以管理员身份运行以安装系统依赖
    echo.
)

REM 检查 Visual Studio
echo [INFO] 检查 Visual Studio...

set "VS_FOUND=0"
set "VS_VERSION="

for %%v in (2022 2019) do (
    for %%e in (Enterprise Professional Community) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\%%v\%%e\Common7\IDE\devenv.exe" (
            set "VS_FOUND=1"
            set "VS_VERSION=%%v"
            set "VS_EDITION=%%e"
        )
    )
)

if "%VS_FOUND%"=="1" (
    echo [OK] 找到 Visual Studio !VS_VERSION! !VS_EDITION!
) else (
    echo [ERROR] 未找到 Visual Studio
    echo [INFO] 请安装 Visual Studio 2019 或 2022
    echo [INFO] 下载地址: https://visualstudio.microsoft.com/downloads/
    exit /b 1
)

REM 检查 CMake
echo.
echo [INFO] 检查 CMake...

where cmake >nul 2>&1
if errorlevel 1 (
    echo [WARN] CMake 未找到
    echo [INFO] 正在检查是否可以安装...
    
    REM 检查 winget
    where winget >nul 2>&1
    if not errorlevel 1 (
        echo [INFO] 使用 winget 安装 CMake...
        winget install Kitware.CMake
    ) else (
        echo [INFO] 请手动安装 CMake: https://cmake.org/download/
    )
) else (
    for /f "tokens=3" %%v in ('cmake --version 2^>^&1 ^| findstr /i "cmake version"') do (
        echo [OK] CMake 版本: %%v
    )
)

REM 检查 Git
echo.
echo [INFO] 检查 Git...

where git >nul 2>&1
if errorlevel 1 (
    echo [WARN] Git 未找到
    echo [INFO] 请安装 Git: https://git-scm.com/downloads
) else (
    for /f "tokens=3" %%v in ('git --version 2^>^&1') do (
        echo [OK] Git 版本: %%v
    )
)

REM 检查 Windows SDK
echo.
echo [INFO] 检查 Windows SDK...

set "SDK_FOUND=0"
for /f "delims=" %%d in ('dir /b /ad "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*" 2^>nul') do (
    set "SDK_VERSION=%%d"
    set "SDK_FOUND=1"
)

if "%SDK_FOUND%"=="1" (
    echo [OK] Windows SDK 版本: !SDK_VERSION!
) else (
    echo [WARN] Windows SDK 未找到
    echo [INFO] 请在 Visual Studio Installer 中安装 Windows SDK
)

REM 检查 vcpkg (可选)
echo.
echo [INFO] 检查 vcpkg (可选)...

if exist "vcpkg\vcpkg.exe" (
    echo [OK] vcpkg 已安装
) else (
    echo [INFO] vcpkg 未安装 (可选)
    echo [INFO] 如需安装，运行: git clone https://github.com/Microsoft/vcpkg.git
)

echo.
echo ========================================
echo   环境检查完成
echo ========================================
echo.
echo 下一步:
echo   1. 运行 GenerateProjectFiles.bat 生成工程
echo   2. 打开 build\Engine.sln 开始开发
echo.

exit /b 0