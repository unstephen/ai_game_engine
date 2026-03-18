#!/bin/bash
# =============================================================================
# 工程文件生成脚本 (Linux/macOS)
# 参考 UE GenerateProjectFiles.sh
# =============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo ""
echo "========================================"
echo "  Engine Project File Generator"
echo "========================================"
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 默认参数
BUILD_TYPE="Release"
OUTPUT_DIR="build"
GENERATOR="Unix Makefiles"
CLEAN=0
NINJA=0
XCODE=0
PLATFORM=""

# 显示帮助
show_help() {
    echo "用法: ./GenerateProjectFiles.sh [选项]"
    echo ""
    echo "选项:"
    echo "  -debug         Debug 构建配置"
    echo "  -release       Release 构建配置 (默认)"
    echo "  -ninja         使用 Ninja 构建系统"
    echo "  -xcode         生成 Xcode 工程 (仅 macOS)"
    echo "  -clean         清理旧工程文件后重新生成"
    echo "  -help, -h      显示帮助信息"
    echo ""
    echo "示例:"
    echo "  ./GenerateProjectFiles.sh"
    echo "  ./GenerateProjectFiles.sh -debug -ninja"
    echo "  ./GenerateProjectFiles.sh -clean"
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
        -ninja)
            NINJA=1
            shift
            ;;
        -xcode)
            XCODE=1
            shift
            ;;
        -clean)
            CLEAN=1
            shift
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

# 检测平台
detect_platform() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="Linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macOS"
    else
        echo -e "${RED}[ERROR] 不支持的平台: $OSTYPE${NC}"
        exit 1
    fi
    echo -e "${GREEN}[INFO] 检测到平台: $PLATFORM${NC}"
}

# 检查 CMake
check_cmake() {
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}[ERROR] CMake 未找到${NC}"
        echo -e "${YELLOW}[INFO] 请安装 CMake:${NC}"
        echo "  Ubuntu/Debian: sudo apt install cmake"
        echo "  macOS: brew install cmake"
        exit 1
    fi
    
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    echo -e "${GREEN}[INFO] CMake 版本: $CMAKE_VERSION${NC}"
}

# 检查编译器
check_compiler() {
    if [[ "$PLATFORM" == "Linux" ]]; then
        if command -v g++ &> /dev/null; then
            GCC_VERSION=$(g++ --version | head -n1)
            echo -e "${GREEN}[INFO] GCC: $GCC_VERSION${NC}"
        elif command -v clang++ &> /dev/null; then
            CLANG_VERSION=$(clang++ --version | head -n1)
            echo -e "${GREEN}[INFO] Clang: $CLANG_VERSION${NC}"
        else
            echo -e "${RED}[ERROR] 未找到 C++ 编译器${NC}"
            exit 1
        fi
    elif [[ "$PLATFORM" == "macOS" ]]; then
        if command -v clang++ &> /dev/null; then
            CLANG_VERSION=$(clang++ --version | head -n1)
            echo -e "${GREEN}[INFO] Clang: $CLANG_VERSION${NC}"
        else
            echo -e "${RED}[ERROR] 未找到 C++ 编译器${NC}"
            exit 1
        fi
    fi
}

# 检查 Ninja
check_ninja() {
    if [[ $NINJA -eq 1 ]]; then
        if command -v ninja &> /dev/null; then
            GENERATOR="Ninja"
            echo -e "${GREEN}[INFO] 使用 Ninja 构建系统${NC}"
        else
            echo -e "${YELLOW}[WARN] Ninja 未找到，使用默认生成器${NC}"
            echo -e "${YELLOW}[INFO] 安装: sudo apt install ninja-build 或 brew install ninja${NC}"
            NINJA=0
        fi
    fi
}

# 检查 Xcode
check_xcode() {
    if [[ $XCODE -eq 1 ]]; then
        if [[ "$PLATFORM" != "macOS" ]]; then
            echo -e "${RED}[ERROR] Xcode 工程仅在 macOS 上可用${NC}"
            exit 1
        fi
        
        if command -v xcodebuild &> /dev/null; then
            GENERATOR="Xcode"
            echo -e "${GREEN}[INFO] 生成 Xcode 工程${NC}"
        else
            echo -e "${RED}[ERROR] Xcode 命令行工具未安装${NC}"
            echo -e "${YELLOW}[INFO] 运行: xcode-select --install${NC}"
            exit 1
        fi
    fi
}

# 清理旧工程
clean_build() {
    if [[ $CLEAN -eq 1 ]]; then
        echo -e "${YELLOW}[INFO] 清理旧工程文件...${NC}"
        rm -rf "$OUTPUT_DIR"
        echo -e "${GREEN}[INFO] 清理完成${NC}"
        echo ""
    fi
}

# 生成工程文件
generate_project() {
    echo ""
    echo -e "${BLUE}[INFO] 生成工程文件...${NC}"
    echo -e "${BLUE}[INFO] 生成器: $GENERATOR${NC}"
    echo -e "${BLUE}[INFO] 输出目录: $OUTPUT_DIR${NC}"
    echo -e "${BLUE}[INFO] 构建类型: $BUILD_TYPE${NC}"
    echo ""
    
    # 构建参数
    CMAKE_ARGS=(
        -S .
        -B "$OUTPUT_DIR"
        -G "$GENERATOR"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DENGINE_BUILD_SAMPLES=ON
        -DENGINE_RHI_VULKAN=OFF
        -DENGINE_DEBUG_LAYER=ON
    )
    
    # 平台特定参数
    if [[ "$PLATFORM" == "Linux" ]]; then
        # Linux 不支持 D3D12
        CMAKE_ARGS+=(-DENGINE_RHI_D3D12=OFF)
    elif [[ "$PLATFORM" == "macOS" ]]; then
        # macOS 不支持 D3D12
        CMAKE_ARGS+=(-DENGINE_RHI_D3D12=OFF)
    fi
    
    cmake "${CMAKE_ARGS[@]}"
    
    if [[ $? -ne 0 ]]; then
        echo ""
        echo -e "${RED}[ERROR] 工程文件生成失败${NC}"
        exit 1
    fi
}

# 显示完成信息
show_complete() {
    echo ""
    echo "========================================"
    echo -e "${GREEN}  生成完成！${NC}"
    echo "========================================"
    echo ""
    echo "构建目录: $OUTPUT_DIR"
    echo ""
    echo "使用方法:"
    echo "  cd $OUTPUT_DIR"
    echo "  make -j\$(nproc)"
    echo ""
    echo "或者:"
    echo "  cmake --build $OUTPUT_DIR --config $BUILD_TYPE"
    echo ""
    
    if [[ "$GENERATOR" == "Xcode" ]]; then
        echo "Xcode 工程: $OUTPUT_DIR/Engine.xcodeproj"
        echo "打开 Xcode: open $OUTPUT_DIR/Engine.xcodeproj"
        echo ""
    fi
}

# 主流程
main() {
    detect_platform
    check_cmake
    check_compiler
    check_ninja
    check_xcode
    clean_build
    generate_project
    show_complete
}

main