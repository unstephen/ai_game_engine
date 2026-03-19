// =============================================================================
// IFence.h - 围栏接口
// ============================================================================

#pragma once

#include <cstdint>

namespace Engine::RHI
{

// =============================================================================
// IFence - 围栏接口
// =============================================================================

class RHI_API IFence
{
  public:
    virtual ~IFence() = default;

    /// 获取当前值
    virtual uint64_t GetCompletedValue() const = 0;

    /// 获取围栏句柄
    virtual void* GetNativeHandle() const = 0;

    /// 等待围栏达到指定值
    virtual void Wait(uint64_t value) = 0;

    /// 信号围栏达到指定值
    virtual void Signal(uint64_t value) = 0;
};

} // namespace Engine::RHI