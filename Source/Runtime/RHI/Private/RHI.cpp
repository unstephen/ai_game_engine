// =============================================================================
// RHI.cpp - 渲染硬件接口实现
// =============================================================================

#include "RHI.h"

// MSVC 下使用 C 标准库头文件
#ifdef _MSC_VER
#include <stdarg.h>
#include <stdio.h>
#else
#include <cstdarg>
#include <cstdio>
#endif

namespace Engine::RHI
{

// =============================================================================
// 日志实现
// =============================================================================

void LogMessage(LogLevel level, const char* format, ...)
{
    const char* levelStr = "";
    switch (level)
    {
        case LogLevel::Error:
            levelStr = "[ERROR]";
            break;
        case LogLevel::Warning:
            levelStr = "[WARN] ";
            break;
        case LogLevel::Info:
            levelStr = "[INFO] ";
            break;
        case LogLevel::Debug:
            levelStr = "[DEBUG]";
            break;
        case LogLevel::Trace:
            levelStr = "[TRACE]";
            break;
    }

    char    buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("%s RHI: %s\n", levelStr, buffer);
}

// =============================================================================
// 错误名称
// =============================================================================

const char* GetErrorName(RHIResult result)
{
    switch (result)
    {
        case RHIResult::Success:
            return "Success";
        case RHIResult::InvalidParameter:
            return "InvalidParameter";
        case RHIResult::OutOfMemory:
            return "OutOfMemory";
        case RHIResult::NotSupported:
            return "NotSupported";
        case RHIResult::InternalError:
            return "InternalError";
        case RHIResult::DeviceLost:
            return "DeviceLost";
        case RHIResult::DeviceRemoved:
            return "DeviceRemoved";
        case RHIResult::DeviceReset:
            return "DeviceReset";
        case RHIResult::DriverVersionMismatch:
            return "DriverVersionMismatch";
        case RHIResult::ResourceCreationFailed:
            return "ResourceCreationFailed";
        case RHIResult::InvalidResourceState:
            return "InvalidResourceState";
        case RHIResult::ResourceAlreadyExists:
            return "ResourceAlreadyExists";
        case RHIResult::PipelineCreationFailed:
            return "PipelineCreationFailed";
        case RHIResult::ShaderCompilationFailed:
            return "ShaderCompilationFailed";
        case RHIResult::InvalidShaderBinding:
            return "InvalidShaderBinding";
        case RHIResult::SwapChainError:
            return "SwapChainError";
        case RHIResult::OutdatedSwapChain:
            return "OutdatedSwapChain";
        case RHIResult::Timeout:
            return "Timeout";
        case RHIResult::WaitAbandoned:
            return "WaitAbandoned";
        default:
            return "Unknown";
    }
}

// =============================================================================
// 全局状态
// =============================================================================

static bool                     g_initialized = false;
static std::unique_ptr<IDevice> g_mainDevice;
static RHIConfig                g_config;

// =============================================================================
// 初始化/关闭
// =============================================================================

RHIResult Initialize(const RHIConfig& config)
{
    if (g_initialized)
    {
        LogMessage(LogLevel::Warning, "RHI 已经初始化");
        return RHIResult::Success;
    }

    g_config      = config;
    g_initialized = true;

    LogMessage(LogLevel::Info, "RHI 初始化完成 (版本 %s)", GetRHIVersionString());
    return RHIResult::Success;
}

void Shutdown()
{
    if (!g_initialized)
    {
        return;
    }

    g_mainDevice.reset();
    g_initialized = false;

    LogMessage(LogLevel::Info, "RHI 关闭完成");
}

bool IsInitialized()
{
    return g_initialized;
}

IDevice* GetMainDevice()
{
    return g_mainDevice.get();
}

IFrameResourceManager* GetFrameResourceManager()
{
    if (g_mainDevice)
    {
        return g_mainDevice->GetFrameResourceManager();
    }
    return nullptr;
}

IUploadManager* GetUploadManager()
{
    if (g_mainDevice)
    {
        return g_mainDevice->GetUploadManager();
    }
    return nullptr;
}

} // namespace Engine::RHI