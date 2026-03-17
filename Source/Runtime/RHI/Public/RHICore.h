// =============================================================================
// RHICore.h - RHI 核心类型和工具
// =============================================================================

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <vector>
#include <functional>

// =============================================================================
// 平台宏
// =============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define RHI_PLATFORM_WINDOWS 1
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define RHI_COMPILER_GCC 1
#endif

#ifdef _MSC_VER
    #define RHI_COMPILER_MSVC 1
#endif

// DLL 导出/导入
#if RHI_PLATFORM_WINDOWS
    #ifdef ENGINE_RHI_EXPORTS
        #define RHI_API __declspec(dllexport)
    #else
        #define RHI_API __declspec(dllimport)
    #endif
#else
    #define RHI_API
#endif

// 强制内联
#if RHI_COMPILER_MSVC
    #define RHI_FORCEINLINE __forceinline
#else
    #define RHI_FORCEINLINE __attribute__((always_inline)) inline
#endif

// 调试断言
#ifdef RHI_DEBUG
    #include <cassert>
    #define RHI_ASSERT(cond, msg) assert((cond) && (msg))
    #define RHI_DEBUG_BREAK() __debugbreak()
#else
    #define RHI_ASSERT(cond, msg) ((void)0)
    #define RHI_DEBUG_BREAK() ((void)0)
#endif

// 日志宏
#define RHI_LOG_ERROR(...)   Engine::RHI::LogMessage(Engine::RHI::LogLevel::Error, __VA_ARGS__)
#define RHI_LOG_WARNING(...) Engine::RHI::LogMessage(Engine::RHI::LogLevel::Warning, __VA_ARGS__)
#define RHI_LOG_INFO(...)    Engine::RHI::LogMessage(Engine::RHI::LogLevel::Info, __VA_ARGS__)
#define RHI_LOG_DEBUG(...)   Engine::RHI::LogMessage(Engine::RHI::LogLevel::Debug, __VA_ARGS__)

namespace Engine::RHI {

// =============================================================================
// 日志级别
// =============================================================================

enum class LogLevel {
    Error,
    Warning,
    Info,
    Debug,
    Trace,
};

// 日志函数
void LogMessage(LogLevel level, const char* format, ...);

// =============================================================================
// 错误码
// =============================================================================

enum class RHIResult : uint32_t {
    Success = 0,
    
    // 通用错误
    InvalidParameter = 1,
    OutOfMemory = 2,
    NotSupported = 3,
    InternalError = 4,
    
    // 设备相关
    DeviceLost = 10,
    DeviceRemoved = 11,
    DeviceReset = 12,
    DriverVersionMismatch = 13,
    
    // 资源相关
    ResourceCreationFailed = 20,
    InvalidResourceState = 21,
    ResourceAlreadyExists = 22,
    
    // 管线相关
    PipelineCreationFailed = 30,
    ShaderCompilationFailed = 31,
    InvalidShaderBinding = 32,
    
    // 交换链相关
    SwapChainError = 40,
    OutdatedSwapChain = 41,
    
    // 同步相关
    Timeout = 50,
    WaitAbandoned = 51,
};

inline bool IsSuccess(RHIResult result) { return result == RHIResult::Success; }
inline bool IsFailure(RHIResult result) { return result != RHIResult::Success; }

inline bool IsDeviceLost(RHIResult result) { 
    return result == RHIResult::DeviceLost || 
           result == RHIResult::DeviceRemoved ||
           result == RHIResult::DeviceReset;
}

const char* GetErrorName(RHIResult result);

// =============================================================================
// 后端类型
// =============================================================================

enum class Backend : uint8_t {
    D3D12,
    Vulkan,
    Metal,
    Null,  // 用于测试
};

// =============================================================================
// 资源状态（位标志）
// =============================================================================

enum class ResourceState : uint32_t {
    Undefined = 0,
    Common = 1 << 0,
    VertexBuffer = 1 << 1,
    IndexBuffer = 1 << 2,
    ConstantBuffer = 1 << 3,
    ShaderResource = 1 << 4,
    UnorderedAccess = 1 << 5,
    RenderTarget = 1 << 6,
    DepthWrite = 1 << 7,
    DepthRead = 1 << 8,
    Present = 1 << 9,
    CopySource = 1 << 10,
    CopyDest = 1 << 11,
    GenericRead = 1 << 12,
};

inline ResourceState operator|(ResourceState a, ResourceState b) {
    return static_cast<ResourceState>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ResourceState operator&(ResourceState a, ResourceState b) {
    return static_cast<ResourceState>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// =============================================================================
// 缓冲区用途
// =============================================================================

enum class BufferUsage : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Index = 1 << 1,
    Constant = 1 << 2,
    ShaderResource = 1 << 3,
    UnorderedAccess = 1 << 4,
    Indirect = 1 << 5,
    CopySource = 1 << 6,
    CopyDest = 1 << 7,
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// =============================================================================
// 纹理用途
// =============================================================================

enum class TextureUsage : uint32_t {
    None = 0,
    ShaderResource = 1 << 0,
    UnorderedAccess = 1 << 1,
    RenderTarget = 1 << 2,
    DepthStencil = 1 << 3,
    CopySource = 1 << 4,
    CopyDest = 1 << 5,
    ResolveSource = 1 << 6,
    ResolveDest = 1 << 7,
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// =============================================================================
// 像素格式
// =============================================================================

enum class Format : uint8_t {
    Unknown,
    
    // 32-bit
    R32G32B32A32_Float,
    R32G32B32_Float,
    R32G32_Float,
    R32_Float,
    
    // 16-bit
    R16G16B16A16_Float,
    R16G16_Float,
    R16_Float,
    
    // 8-bit
    R8G8B8A8_UNorm,
    R8G8B8A8_UNorm_SRGB,
    R8G8_UNorm,
    R8_UNorm,
    
    // 深度
    D32_Float,
    D24_UNorm_S8_UInt,
    D16_UNorm,
    
    // 压缩
    BC1_UNorm,
    BC3_UNorm,
    BC7_UNorm,
};

// =============================================================================
// 着色器阶段
// =============================================================================

enum class ShaderStage : uint8_t {
    Vertex,
    Pixel,
    Compute,
    Geometry,
    Hull,
    Domain,
    RayGeneration,
    ClosestHit,
    Miss,
};

// =============================================================================
// 拓扑类型
// =============================================================================

enum class PrimitiveTopology : uint8_t {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

// =============================================================================
// 填充模式
// =============================================================================

enum class FillMode : uint8_t {
    Solid,
    Wireframe,
};

// =============================================================================
// 剔除模式
// =============================================================================

enum class CullMode : uint8_t {
    None,
    Front,
    Back,
};

// =============================================================================
// 混合操作
// =============================================================================

enum class BlendOp : uint8_t {
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max,
};

// =============================================================================
// 混合因子
// =============================================================================

enum class BlendFactor : uint8_t {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor,
    SrcAlphaSat,
    BlendFactor,
    InvBlendFactor,
    Src1Color,
    InvSrc1Color,
    Src1Alpha,
    InvSrc1Alpha,
};

// =============================================================================
// 比较函数
// =============================================================================

enum class CompareFunc : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

// =============================================================================
// 模板操作
// =============================================================================

enum class StencilOp : uint8_t {
    Keep,
    Zero,
    Replace,
    IncrementSat,
    DecrementSat,
    Invert,
    Increment,
    Decrement,
};

// =============================================================================
// 配置结构
// =============================================================================

struct RHIConfig {
    Backend backend = Backend::D3D12;
    bool enableDebug = true;
    bool enableValidation = true;
    uint32_t numFrames = 3;  // 帧缓冲数量
};

} // namespace Engine::RHI
