#!/bin/bash
# =============================================================================
# 环境设置脚本 (Linux/macOS)
# 用于安装依赖和配置开发环境
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
echo "  Engine Environment Setup"
echo "========================================"
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 检测平台
detect_platform() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="Linux"
        PKG_MANAGER=""
        
        if command -v apt &> /dev/null; then
            PKG_MANAGER="apt"
        elif command -v dnf &> /dev/null; then
            PKG_MANAGER="dnf"
        elif command -v yum &> /dev/null; then
            PKG_MANAGER="yum"
        elif command -v pacman &> /dev/null; then
            PKG_MANAGER="pacman"
        fi
        
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macOS"
        PKG_MANAGER="brew"
    else
        echo -e "${RED}[ERROR] 不支持的平台: $OSTYPE${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}[INFO] 平台: $PLATFORM${NC}"
    echo -e "${GREEN}[INFO] 包管理器: $PKG_MANAGER${NC}"
}

# 检查并安装依赖
check_cmake() {
    echo ""
    echo -e "${BLUE}[INFO] 检查 CMake...${NC}"
    
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
        echo -e "${GREEN}[OK] CMake 版本: $CMAKE_VERSION${NC}"
    else
        echo -e "${YELLOW}[WARN] CMake 未找到${NC}"
        
        read -p "是否安装 CMake? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_cmake
        fi
    fi
}

install_cmake() {
    echo -e "${BLUE}[INFO] 安装 CMake...${NC}"
    
    case $PKG_MANAGER in
        apt)
            sudo apt update
            sudo apt install -y cmake
            ;;
        dnf)
            sudo dnf install -y cmake
            ;;
        yum)
            sudo yum install -y cmake
            ;;
        pacman)
            sudo pacman -S --noconfirm cmake
            ;;
        brew)
            brew install cmake
            ;;
        *)
            echo -e "${RED}[ERROR] 未知的包管理器${NC}"
            echo -e "${YELLOW}[INFO] 请手动安装 CMake${NC}"
            ;;
    esac
}

check_compiler() {
    echo ""
    echo -e "${BLUE}[INFO] 检查 C++ 编译器...${NC}"
    
    if [[ "$PLATFORM" == "Linux" ]]; then
        if command -v g++ &> /dev/null; then
            GCC_VERSION=$(g++ --version | head -n1)
            echo -e "${GREEN}[OK] GCC: $GCC_VERSION${NC}"
        elif command -v clang++ &> /dev/null; then
            CLANG_VERSION=$(clang++ --version | head -n1)
            echo -e "${GREEN}[OK] Clang: $CLANG_VERSION${NC}"
        else
            echo -e "${YELLOW}[WARN] 未找到 C++ 编译器${NC}"
            
            read -p "是否安装 GCC? (y/n): " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                install_gcc
            fi
        fi
        
    elif [[ "$PLATFORM" == "macOS" ]]; then
        if command -v clang++ &> /dev/null; then
            CLANG_VERSION=$(clang++ --version | head -n1)
            echo -e "${GREEN}[OK] Clang: $CLANG_VERSION${NC}"
        else
            echo -e "${YELLOW}[WARN] 未找到 Clang${NC}"
            echo -e "${YELLOW}[INFO] 运行: xcode-select --install${NC}"
        fi
    fi
}

install_gcc() {
    echo -e "${BLUE}[INFO] 安装 GCC...${NC}"
    
    case $PKG_MANAGER in
        apt)
            sudo apt update
            sudo apt install -y build-essential g++
            ;;
        dnf)
            sudo dnf install -y gcc-c++
            ;;
        yum)
            sudo yum install -y gcc-c++
            ;;
        pacman)
            sudo pacman -S --noconfirm gcc
            ;;
        *)
            echo -e "${RED}[ERROR] 未知的包管理器${NC}"
            ;;
    esac
}

check_git() {
    echo ""
    echo -e "${BLUE}[INFO] 检查 Git...${NC}"
    
    if command -v git &> /dev/null; then
        GIT_VERSION=$(git --version | cut -d' ' -f3)
        echo -e "${GREEN}[OK] Git 版本: $GIT_VERSION${NC}"
    else
        echo -e "${YELLOW}[WARN] Git 未找到${NC}"
        
        read -p "是否安装 Git? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_git
        fi
    fi
}

install_git() {
    echo -e "${BLUE}[INFO] 安装 Git...${NC}"
    
    case $PKG_MANAGER in
        apt)
            sudo apt install -y git
            ;;
        dnf)
            sudo dnf install -y git
            ;;
        yum)
            sudo yum install -y git
            ;;
        pacman)
            sudo pacman -S --noconfirm git
            ;;
        brew)
            brew install git
            ;;
    esac
}

check_ninja() {
    echo ""
    echo -e "${BLUE}[INFO] 检查 Ninja (可选)...${NC}"
    
    if command -v ninja &> /dev/null; then
        NINJA_VERSION=$(ninja --version)
        echo -e "${GREEN}[OK] Ninja 版本: $NINJA_VERSION${NC}"
    else
        echo -e "${YELLOW}[INFO] Ninja 未安装 (可选，推荐)${NC}"
        echo -e "${YELLOW}[INFO] 安装: sudo apt install ninja-build 或 brew install ninja${NC}"
    fi
}

check_vulkan() {
    echo ""
    echo -e "${BLUE}[INFO] 检查 Vulkan (可选)...${NC}"
    
    if [[ "$PLATFORM" == "Linux" ]]; then
        if command -v vulkaninfo &> /dev/null; then
            echo -e "${GREEN}[OK] Vulkan 已安装${NC}"
        else
            echo -e "${YELLOW}[INFO] Vulkan 未安装 (可选，用于 Vulkan 后端)${NC}"
            echo -e "${YELLOW}[INFO] 安装: sudo apt install vulkan-tools libvulkan-dev${NC}"
        fi
    elif [[ "$PLATFORM" == "macOS" ]]; then
        if [ -d "/usr/local/lib/libMoltenVK.dylib" ] || [ -d "/opt/homebrew/lib/libMoltenVK.dylib" ]; then
            echo -e "${GREEN}[OK] MoltenVK 已安装${NC}"
        else
            echo -e "${YELLOW}[INFO] MoltenVK 未安装 (可选，用于 Vulkan 后端)${NC}"
            echo -e "${YELLOW}[INFO] 安装: brew install molten-vk${NC}"
        fi
    fi
}

show_complete() {
    echo ""
    echo "========================================"
    echo -e "${GREEN}  环境检查完成${NC}"
    echo "========================================"
    echo ""
    echo "下一步:"
    echo "  1. 运行 ./GenerateProjectFiles.sh 生成工程"
    echo "  2. cd build && make -j\$(nproc)"
    echo ""
}

# 主流程
main() {
    detect_platform
    check_cmake
    check_compiler
    check_git
    check_ninja
    check_vulkan
    show_complete
}

main