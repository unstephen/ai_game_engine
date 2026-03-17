# 引擎编译说明

## 项目概述

自研 3D 游戏引擎 - 目标超越 Unreal Engine

**当前版本**: 0.1.0  
**C++ 标准**: C++20  
**构建系统**: CMake 3.20+

---

## 系统要求

### Windows (主要支持平台)

| 组件 | 最低版本 | 推荐版本 |
|------|----------|----------|
| Windows | Windows 10 (1809+) | Windows 11 |
| Visual Studio | 2019 16.11 | 2022 17.x |
| Windows SDK | 10.0.19041.0 | 10.0.22621.0 |
| CMake | 3.20 | 3.25+ |

### 硬件要求

- **GPU**: 支持 DirectX 12 Feature Level 11.0 的显卡
- **内存**: 8GB RAM (推荐 16GB)
- **显存**: 2GB VRAM (推荐 4GB+)

---

## 快速开始

### 1. 克隆项目

```bash
git clone <repository-url>
cd engine
```

### 2. 配置 CMake

#### Visual Studio 2022 (推荐)

```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
```

#### Visual Studio 2019

```cmd
cmake -B build -G "Visual Studio 16 2019" -A x64
```

#### Ninja (需要已安装)

```cmd
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### 3. 编译项目

```cmd
cmake --build build --config Release
```

### 4. 运行示例

```cmd
.\build\bin\Release\Sample_Triangle.exe
```

---

## CMake 选项

| 选项 | 默认值 | 描述 |
|------|--------|------|
| `ENGINE_BUILD_EDITOR` | OFF | 构建编辑器 |
| `ENGINE_BUILD_SAMPLES` | ON | 构建示例项目 |
| `ENGINE_RHI_D3D12` | ON | 启用 D3D12 后端 |
| `ENGINE_RHI_VULKAN` | OFF | 启用 Vulkan 后端 |
| `ENGINE_DEBUG_LAYER` | ON | 启用调试层 |
| `ENGINE_WARNINGS_AS_ERRORS` | OFF | 警告视为错误 |
| `ENGINE_BUILD_TESTS` | ON | 构建测试 |

### 示例配置

```cmd
# 调试构建 + 调试层
cmake -B build -DENGINE_DEBUG_LAYER=ON -DCMAKE_BUILD_TYPE=Debug

# 发布构建 + 警告作为错误
cmake -B build -DENGINE_WARNINGS_AS_ERRORS=ON -DCMAKE_BUILD_TYPE=Release

# 禁用测试
cmake -B build -DENGINE_BUILD_TESTS=OFF
```

---

## 项目结构

```
engine/
├── CMakeLists.txt          # 主 CMake 配置
├── BUILD.md                # 本文档
├── Source/
│   └── Runtime/
│       ├── Core/           # 核心系统
│       │   ├── Public/     # 公共头文件
│       │   └── Private/    # 实现文件
│       ├── Window/         # 窗口管理
│       └── RHI/            # 渲染硬件接口
│           ├── Public/     # RHI 接口
│           └── D3D12/      # D3D12 后端
├── Samples/
│   └── Triangle/           # 三角形示例
└── Tests/                  # 单元测试
```

---

## 构建目标

### 库

| 目标 | 描述 |
|------|------|
| `EngineCore` | 核心系统库 |
| `EngineWindow` | 窗口管理库 |
| `EngineRHI` | RHI 接口库 |
| `EngineRHID3D12` | D3D12 后端库 |

### 示例

| 目标 | 描述 |
|------|------|
| `Sample_Triangle` | 三角形渲染示例 |

### 测试

| 目标 | 描述 |
|------|------|
| `Test_D3D12Device` | D3D12 设备测试 |
| `Test_D3D12Buffer` | D3D12 缓冲区测试 |
| `Test_D3D12Texture` | D3D12 纹理测试 |

---

## 运行测试

### 前提条件

需要安装 Google Test。如果未安装，测试将被禁用。

#### 使用 vcpkg 安装 Google Test

```cmd
vcpkg install gtest:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg路径]/scripts/buildsystems/vcpkg.cmake
```

#### 手动安装

1. 从 https://github.com/google/googletest 下载
2. 编译并安装到系统路径

### 运行测试

```cmd
cd build
ctest -C Release --output-on-failure
```

或直接运行测试可执行文件：

```cmd
.\build\bin\Release\Test_D3D12Device.exe
```

---

## 依赖库

### Windows SDK (必需)

项目依赖以下 Windows SDK 组件：

- `d3d12.lib` - Direct3D 12
- `dxgi.lib` - DirectX Graphics Infrastructure
- `dxguid.lib` - DirectX GUID
- `user32.lib` - Windows 用户界面

### 可选依赖

| 库 | 用途 | 安装方式 |
|----|------|----------|
| Google Test | 单元测试 | vcpkg / 手动 |
| DirectX Headers | 跨平台 D3D12 头文件 | vcpkg |
| DirectX 12 Agility SDK | 最新 D3D12 特性 | vcpkg |

---

## 常见问题

### 1. 找不到 d3d12.lib

**原因**: Windows SDK 未安装或版本过低

**解决方案**:
1. 安装 Windows SDK 10.0.19041.0 或更高版本
2. 在 Visual Studio Installer 中确保勾选 "Windows SDK"

### 2. D3D12 调试层无法启用

**原因**: 未安装图形工具

**解决方案**:
```powershell
# 以管理员身份运行
dism /online /add-capability /capabilityname:Tools.Graphics.DirectX~~~~0.0.1.0
```

### 3. CMake 配置失败

**原因**: CMake 版本过低

**解决方案**: 升级 CMake 到 3.20 或更高版本

### 4. 编译错误: 找不到头文件

**原因**: 包含路径配置问题

**解决方案**: 确保使用 CMake 生成项目，不要直接打开 .sln 文件

---

## 开发建议

### IDE 配置

#### Visual Studio

1. 启用 C++20 标准: 项目属性 → C/C++ → 语言 → C++ 语言标准 → ISO C++20 标准
2. 启用多处理器编译: 项目属性 → C/C++ → 常规 → 多处理器编译 → 是

#### Visual Studio Code

推荐安装扩展：
- C/C++ (Microsoft)
- CMake Tools
- CMake

### 调试配置

1. 启用 D3D12 调试层:
   ```cpp
   DeviceDesc desc;
   desc.enableDebug = true;
   desc.enableValidation = true;
   ```

2. 使用 PIX for Windows 进行图形调试:
   https://devblogs.microsoft.com/pix/

---

## 持续集成

### GitHub Actions 示例

```yaml
name: Build

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Configure CMake
        run: cmake -B build -G "Visual Studio 17 2022" -A x64
        
      - name: Build
        run: cmake --build build --config Release
        
      - name: Test
        run: cd build && ctest -C Release --output-on-failure
```

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 0.1.0 | 2024-01 | 初始版本，D3D12 基础设施 |

---

## 联系方式

如有问题，请提交 Issue 或联系开发团队。