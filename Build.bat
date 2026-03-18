@echo off
REM =============================================================================
REM 构建脚本 (Windows)
REM =============================================================================

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   Engine Build Script
echo ========================================
echo.

REM 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM 默认参数
set "BUILD_TYPE=Release"
set "BUILD_DIR=build"
set "CLEAN=0"
set "RUN=0"

REM 解析命令行参数
:parse_args
if "%~1"=="" goto :end_parse_args
if /i "%~1"=="-debug" set "BUILD_TYPE=Debug"
if /i "%~1"=="-release" set "BUILD_TYPE=Release"
if /i "%~1"=="-clean" set "CLEAN=1"
if /i "%~1"=="-run" set "RUN=1"
if /i "%~1"=="-help" goto :show_help
if /i "%~1"=="-h" goto :show_help
shift
goto :parse_args
:end_parse_args

REM 检查构建目录
if not exist "%BUILD_DIR%" (
    echo [ERROR] 构建目录不存在: %BUILD_DIR%
    echo [INFO] 请先运行 GenerateProjectFiles.bat
    exit /b 1
)

REM 清理
if "%CLEAN%"=="1" (
    echo [INFO] 清理构建目录...
    cmake --build "%BUILD_DIR%" --target clean
    echo.
)

REM 构建
echo [INFO] 开始构建 (%BUILD_TYPE%)...
echo.

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -- /m

if errorlevel 1 (
    echo.
    echo [ERROR] 构建失败
    exit /b 1
)

echo.
echo ========================================
echo   构建完成！
echo ========================================
echo.
echo 输出目录: %BUILD_DIR%\bin\%BUILD_TYPE%
echo.

REM 运行示例
if "%RUN%"=="1" (
    echo [INFO] 运行示例...
    if exist "%BUILD_DIR%\bin\%BUILD_TYPE%\Sample_Triangle.exe" (
        "%BUILD_DIR%\bin\%BUILD_TYPE%\Sample_Triangle.exe"
    ) else (
        echo [WARN] 示例程序未找到
    )
)

exit /b 0

:show_help
echo.
echo 用法: Build.bat [选项]
echo.
echo 选项:
echo   -debug       Debug 构建
echo   -release     Release 构建 (默认)
echo   -clean       清理后构建
echo   -run         构建后运行示例
echo   -help, -h    显示帮助
echo.
exit /b 0