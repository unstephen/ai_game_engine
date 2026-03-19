// =============================================================================
// IFrameResourceManager.h - 帧资源管理器接口
// =============================================================================

#pragma once

#include <cstdint>

namespace Engine::RHI
{

// =============================================================================
// IFrameResourceManager - 帧资源管理器接口
// =============================================================================

class RHI_API IFrameResourceManager
{
  public:
    virtual ~IFrameResourceManager() = default;

    /// 开始新帧
    virtual void BeginFrame() = 0;

    /// 结束当前帧
    virtual void EndFrame() = 0;

    /// 获取当前帧索引
    virtual uint32_t GetCurrentFrameIndex() const = 0;

    /// 获取帧缓冲数量
    virtual uint32_t GetFrameCount() const = 0;

    /// 等待帧完成
    virtual void WaitForFrame(uint32_t frameIndex) = 0;

    /// 获取帧围栏值
    virtual uint64_t GetFrameFenceValue(uint32_t frameIndex) const = 0;
};

} // namespace Engine::RHI