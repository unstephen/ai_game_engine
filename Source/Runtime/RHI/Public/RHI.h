// =============================================================================
// RHI.h - 渲染硬件接口主头文件
// =============================================================================

#pragma once

// 先包含 RHICore.h 以获取 Windows API 宏的 #undef 保护
#include "RHICore.h"

#include "IBuffer.h"
#include "ICommandList.h"
#include "IDescriptorAllocator.h"
#include "IDevice.h"
#include "IFence.h"
#include "IFrameResourceManager.h"
#include "IPipelineState.h"
#include "IResource.h"
#include "IRootSignature.h"
#include "IShader.h"
#include "ISwapChain.h"
#include "ITexture.h"
#include "IUploadManager.h"

/**
 * @namespace Engine::RHI
 * @brief 渲染硬件接口核心命名空间
 *
 * 提供跨图形 API（D3D12/Vulkan/Metal）的统一抽象层
 */
namespace Engine::RHI
{

// =============================================================================
// 版本信息
// =============================================================================

constexpr uint32_t RHI_VERSION_MAJOR = 0;
constexpr uint32_t RHI_VERSION_MINOR = 2;
constexpr uint32_t RHI_VERSION_PATCH = 0;

inline const char* GetRHIVersionString()
{
    return "0.2.0";
}

// =============================================================================
// 初始化/关闭
// =============================================================================

/**
 * @brief 初始化 RHI 系统
 * @param config 配置选项
 * @return RHIResult 结果码
 */
RHIResult Initialize(const RHIConfig& config = {});

/**
 * @brief 关闭 RHI 系统
 */
void Shutdown();

/**
 * @brief 检查 RHI 是否已初始化
 * @return true 如果已初始化
 */
bool IsInitialized();

/**
 * @brief 获取主设备
 * @return IDevice* 主设备指针
 */
IDevice* GetMainDevice();

/**
 * @brief 获取帧资源管理器
 * @return IFrameResourceManager* 帧资源管理器指针
 */
IFrameResourceManager* GetFrameResourceManager();

/**
 * @brief 获取上传管理器
 * @return IUploadManager* 上传管理器指针
 */
IUploadManager* GetUploadManager();

} // namespace Engine::RHI

// =============================================================================
// 便捷宏
// =============================================================================

#ifdef RHI_DEBUG
#define RHI_VERIFY_CALL(call)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        auto _result = (call);                                                                                         \
        if (Engine::RHI::IsFailure(_result))                                                                           \
        {                                                                                                              \
            RHI_LOG_ERROR("RHI call failed: %s at %s:%d", Engine::RHI::GetErrorName(_result), __FILE__, __LINE__);     \
        }                                                                                                              \
    } while (0)
#else
#define RHI_VERIFY_CALL(call) (void)(call)
#endif
