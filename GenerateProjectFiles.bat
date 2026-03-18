@echo off
REM =============================================================================
REM 工程文件生成脚本 (Windows)
REM 参考 UE GenerateProjectFiles.bat
REM =============================================================================

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   Engine Project File Generator
echo ========================================
echo.

REM 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM 默认参数
set "VS_VERSION=2022"
set "BUILD_TYPE=Release"
set "GENERATOR="
set "OUTPUT_DIR=build"
set "CLEAN=0"

REM 解析命令行参数
:parse_args
if "%~1"=="" goto :end_parse_args
if /i "%~1"=="-vs2019" set "VS_VERSION=2019"
if /i "%~1"=="-vs2022" set "VS_VERSION=2022"
if /i "%~1"=="-debug" set "BUILD_TYPE=Debug"
if /i "%~1"=="-release" set "BUILD_TYPE=Release"
if /i "%~1"=="-clean" set "CLEAN=1"
if /i "%~1"=="-help" goto :show_help
if /i "%~1"=="-h" goto :show_help
shift
goto :parse_args
:end_parse_args

REM 清理旧工程
if "%CLEAN%"=="1" (
    echo [INFO] 清理旧工程文件...
    if exist "%OUTPUT_DIR%" rmdir /s /q "%OUTPUT_DIR%"
    echo [INFO] 清理完成
    echo.
)

REM 检测 Visual Studio
echo [INFO] 检测 Visual Studio %VS_VERSION%...

if "%VS_VERSION%"=="2022" (
    set "GENERATOR=Visual Studio 17 2022"
    set "VS_DIR=Microsoft Visual Studio\2022"
)

if "%VS_VERSION%"=="2019" (
    set "GENERATOR=Visual Studio 16 2019"
    set "VS_DIR=Microsoft Visual Studio\2019"
)

REM 检查 VS 是否安装
set "VS_FOUND=0"
for %%d in (Enterprise Professional Community) do (
    if exist "%ProgramFiles%\!VS_DIR!\%%d\Common7\IDE\devenv.exe" (
        set "VS_FOUND=1"
        set "VS_EDITION=%%d"
    )
)

if "%VS_FOUND%"=="0" (
    echo [ERROR] Visual Studio %VS_VERSION% 未找到
    echo [INFO] 请安装 Visual Studio %VS_VERSION% 或使用 -vs2019 参数
    exit /b 1
)

echo [INFO] 找到 Visual Studio %VS_VERSION% !VS_EDITION!
echo.

REM 检查 CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake 未找到
    echo [INFO] 请安装 CMake 并添加到 PATH
    exit /b 1
)

echo [INFO] CMake 版本：
cmake --version
echo.

REM 创建输出目录
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

REM 生成工程文件
echo [INFO] 生成 Visual Studio %VS_VERSION% 解决方案...
echo [INFO] 生成器: %GENERATOR%
echo [INFO] 输出目录: %OUTPUT_DIR%
echo.

cmake -S . -B "%OUTPUT_DIR%" ^
    -G "%GENERATOR%" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DENGINE_BUILD_SAMPLES=ON ^
    -DENGINE_RHI_D3D12=ON ^
    -DENGINE_DEBUG_LAYER=ON

if errorlevel 1 (
    echo.
    echo [ERROR] 工程文件生成失败
    exit /b 1
)

echo.
echo ========================================
echo   生成完成！
echo ========================================
echo.
echo 解决方案位置: %OUTPUT_DIR%\Engine.sln
echo.
echo 使用方法:
echo   1. 打开 %OUTPUT_DIR%\Engine.sln
echo   2. 选择构建配置 (Debug/Release)
echo   3. 按 F7 或 Ctrl+Shift+B 构建
echo.
echo 命令行构建:
echo   cmake --build %OUTPUT_DIR% --config Release
echo.

REM 尝试打开 VS
set "OPEN_VS=0"
set /p "OPEN_VS=是否打开 Visual Studio? (Y/N): "
if /i "%OPEN_VS%"=="Y" (
    echo [INFO] 启动 Visual Studio...
    start "" "%ProgramFiles%\!VS_DIR!\!VS_EDITION!\Common7\IDE\devenv.exe" "%OUTPUT_DIR%\Engine.sln"
)

exit /b 0

:show_help
echo.
echo 用法: GenerateProjectFiles.bat [选项]
echo.
echo 选项:
echo   -vs2019      使用 Visual Studio 2019
echo   -vs2022      使用 Visual Studio 2022 (默认)
echo   -debug       Debug 构建配置
echo   -release     Release 构建配置 (默认)
echo   -clean       清理旧工程文件后重新生成
echo   -help, -h    显示帮助信息
echo.
echo 示例:
echo   GenerateProjectFiles.bat
echo   GenerateProjectFiles.bat -vs2019 -debug
echo   GenerateProjectFiles.bat -clean
echo.
exit /b 0