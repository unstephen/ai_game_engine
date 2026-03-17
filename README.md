# Engine

自研 3D 游戏引擎 - 目标超越 Unreal Engine

## 项目结构

```
engine/
├── Source/                     # 源代码
│   ├── Runtime/                # 运行时核心模块
│   │   ├── Core/               # 核心系统（内存、数学、容器）
│   │   ├── RHI/                # 渲染硬件接口
│   │   │   ├── D3D12/          # DirectX 12 后端
│   │   │   ├── Vulkan/         # Vulkan 后端（Phase 2）
│   │   │   └── Metal/          # Metal 后端（Phase 3）
│   │   ├── Renderer/           # 渲染器（Forward+/Deferred）
│   │   ├── Asset/              # 资源系统
│   │   ├── Window/             # 窗口管理
│   │   └── Platform/           # 平台抽象层
│   │
│   ├── Editor/                 # 编辑器（Phase 3+）
│   │   ├── EditorApp/          # 编辑器应用
│   │   ├── Viewport/           # 视口
│   │   └── Tools/              # 编辑器工具
│   │
│   └── Programs/               # 独立程序
│       ├── ShaderCompiler/     # 着色器编译器
│       └── AssetCooker/        # 资源烘焙工具
│
├── Build/                      # 构建配置
│   ├── CMake/                  # CMake 模块
│   └── Scripts/                # 构建脚本
│
├── CMake/                      # CMake 配置
│   ├── CMakeLists.txt          # 主配置文件
│   └── Find*.cmake             # 查找模块
│
├── Docs/                       # 文档
│   ├── Architecture/           # 架构文档
│   ├── API/                    # API 文档
│   └── Tutorials/              # 教程
│
├── Extras/                     # 额外内容
│   ├── ThirdParty/             # 第三方库
│   └── Samples/                # 示例项目
│
└── Plugins/                    # 插件系统
    └── [PluginName]/           # 插件目录
```

## 快速开始

### 前置要求

- **操作系统**: Windows 10/11 (64-bit)
- **编译器**: MSVC 2022 (v143+) 或 Clang 15+
- **CMake**: 3.20+
- **DirectX 12**: Windows SDK 10.0.19041.0+
- **Git**: 2.30+

### 构建步骤

```bash
# 1. 克隆仓库
git clone https://github.com/your-org/engine.git
cd engine

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置 CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# 4. 构建
cmake --build . --config Debug

# 5. 运行示例
./Debug/Samples/Triangle.exe
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `ENGINE_BUILD_EDITOR` | OFF | 构建编辑器 |
| `ENGINE_BUILD_SAMPLES` | ON | 构建示例项目 |
| `ENGINE_RHI_D3D12` | ON | 启用 D3D12 后端 |
| `ENGINE_RHI_VULKAN` | OFF | 启用 Vulkan 后端 |
| `ENGINE_DEBUG_LAYER` | ON | 启用调试层 |

## 开发路线图

| Phase | 时间 | 目标 | 状态 |
|-------|------|------|------|
| **Phase 1** | 2 周 | RHI 核心框架 + D3D12 后端 + 三角形渲染 | 🟡 进行中 |
| **Phase 2** | 4 周 | 资源系统 + 描述符管理 + 纹理支持 | ⬜ 待开始 |
| **Phase 3** | 4 周 | 渲染管线 + 深度/混合 + 性能优化 | ⬜ 待开始 |
| **Phase 4** | 8 周 | 渲染器 + 材质系统 + 光照 | ⬜ 待开始 |

## 核心模块

### RHI (Rendering Hardware Interface)

渲染硬件接口，提供跨图形 API 的抽象层。

**设计原则**:
- 单一后端优先（D3D12 → Vulkan → Metal）
- 显式资源状态管理
- 零开销抽象
- 现代 C++ 风格（RAII、智能指针）

**核心接口**:
```cpp
IDevice                 // 设备（工厂）
ICommandList            // 命令列表（录制）
IBuffer / ITexture      // 资源
IPipelineState          // 管线状态
IRootSignature          // 根签名
ISwapChain              // 交换链
IFrameResourceManager   // 帧资源管理（三缓冲）
IDescriptorAllocator    // 描述符分配
IUploadManager          // 上传管理器
```

### Core

引擎核心系统，提供基础功能。

**模块**:
- 内存管理（分配器、智能指针）
- 数学库（向量、矩阵、四元数）
- 容器（Array、Vector、Map）
- 日志系统
- 时间系统

### Window

窗口管理平台抽象。

**功能**:
- 窗口创建/销毁
- 输入处理（键盘、鼠标）
- DPI 感知
- 全屏/窗口模式切换

## 编码规范

### C++ 版本

- **标准**: C++20
- **特性**: Modules（可选）、Concepts、Coroutines

### 命名约定

```cpp
// 类和结构体：PascalCase
class IDevice;
struct FrameContext;

// 函数和方法：CamelCase
void BeginFrame();
uint32_t GetFrameIndex();

// 变量：camelCase
uint32_t frameIndex;
IDevice* device;

// 成员变量：m_前缀
uint32_t m_frameIndex;
IDevice* m_device;

// 常量：kCamelCase
const uint32_t kMaxFrames = 3;

// 枚举：PascalCase，值：ePascalCase
enum class ResourceState {
    eCommon,
    eVertexBuffer,
};

// 命名空间：小写
namespace engine::rhi {}
```

### 文件组织

```cpp
// 头文件：ClassName.h
// 源文件：ClassName.cpp

// 每个类单独文件，避免大文件
Source/Runtime/RHI/
├── Public/
│   ├── IDevice.h
│   ├── ICommandList.h
│   └── ITexture.h
└── Private/
    ├── D3D12Device.cpp
    ├── D3D12CommandList.cpp
    └── D3D12Texture.cpp
```

### 智能指针

```cpp
// 所有权：unique_ptr
std::unique_ptr<IDevice> device;

// 共享所有权：shared_ptr（谨慎使用）
std::shared_ptr<IResource> resource;

// 观察：raw pointer 或 weak_ptr
IResource* resource;  // 不拥有，仅观察
```

## 调试和性能分析

### PIX

- 启用调试层：`-DENGINE_DEBUG_LAYER=ON`
- 使用 PIX for Windows 捕获帧
- 分析 GPU/CPU 时间线

### RenderDoc

- 支持 RenderDoc 捕获
- 资源命名：`resource->SetDebugName("MyTexture")`

### 日志

```cpp
// 日志级别
RHI_LOG_ERROR("Error message");
RHI_LOG_WARNING("Warning message");
RHI_LOG_INFO("Info message");
RHI_LOG_DEBUG("Debug message");
RHI_LOG_TRACE("Trace message");
```

## 贡献指南

1. Fork 仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 提交 Pull Request

## 许可证

[待确定]

## 联系方式

- 项目主页：[待添加]
- 问题追踪：[待添加]
- 讨论区：[待添加]

---

*自研 3D 游戏引擎 - 龙景 2026-03-17* 🐉
