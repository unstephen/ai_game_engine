#!/bin/bash
# =============================================================================
# 构建脚本 (Linux/macOS)
# =============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo ""
echo "========================================"
echo "  Engine Build Script"
echo "========================================"
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 默认参数
BUILD_TYPE="Release"
BUILD_DIR="build"
CLEAN=0
RUN=0
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# 显示帮助
show_help() {
    echo "用法: ./Build.sh [选项]"
    echo ""
    echo "选项:"
    echo "  -debug       Debug 构建"
    echo "  -release     Release 构建 (默认)"
    echo "  -clean       清理后构建"
    echo "  -run         构建后运行示例"
    echo "  -j N         并行任务数 (默认: $JOBS)"
    echo "  -help, -h    显示帮助"
    echo ""
    exit 0
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -release)
            BUILD_TYPE="Release"
            shift
            ;;
        -clean)
            CLEAN=1
            shift
            ;;
        -run)
            RUN=1
            shift
            ;;
        -j)
            JOBS="$2"
            shift 2
            ;;
        -help|-h)
            show_help
            ;;
        *)
            echo -e "${RED}[ERROR] 未知参数: $1${NC}"
            exit 1
            ;;
    esac
done

# 检查构建目录
if [[ ! -d "$BUILD_DIR" ]]; then
    echo -e "${RED}[ERROR] 构建目录不存在: $BUILD_DIR${NC}"
    echo -e "${YELLOW}[INFO] 请先运行 ./GenerateProjectFiles.sh${NC}"
    exit 1
fi

# 清理
if [[ $CLEAN -eq 1 ]]; then
    echo -e "${YELLOW}[INFO] 清理构建目录...${NC}"
    cmake --build "$BUILD_DIR" --target clean
    echo ""
fi

# 构建
echo -e "${BLUE}[INFO] 开始构建 ($BUILD_TYPE)...${NC}"
echo -e "${BLUE}[INFO] 并行任务数: $JOBS${NC}"
echo ""

cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$JOBS"

if [[ $? -ne 0 ]]; then
    echo ""
    echo -e "${RED}[ERROR] 构建失败${NC}"
    exit 1
fi

echo ""
echo "========================================"
echo -e "${GREEN}  构建完成！${NC}"
echo "========================================"
echo ""
echo "输出目录: $BUILD_DIR/bin"
echo ""

# 运行示例
if [[ $RUN -eq 1 ]]; then
    echo -e "${BLUE}[INFO] 运行示例...${NC}"
    
    if [[ -f "$BUILD_DIR/bin/Sample_Triangle" ]]; then
        "$BUILD_DIR/bin/Sample_Triangle"
    elif [[ -f "$BUILD_DIR/bin/$BUILD_TYPE/Sample_Triangle" ]]; then
        "$BUILD_DIR/bin/$BUILD_TYPE/Sample_Triangle"
    else
        echo -e "${YELLOW}[WARN] 示例程序未找到${NC}"
    fi
fi