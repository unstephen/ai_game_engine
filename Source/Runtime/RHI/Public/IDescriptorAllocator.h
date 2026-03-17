// =============================================================================
// IDescriptorAllocator.h - 描述符分配器接口
// =============================================================================

#pragma once

#include "RHICore.h"

namespace Engine::RHI
{

// =============================================================================
// 描述符堆类型
// =============================================================================

enum class DescriptorHeapType : uint8_t
{
    CBV_SRV_UAV, // 常量缓冲区、着色器资源、无序访问
    Sampler,     // 采样器
    RTV,         // 渲染目标视图
    DSV,         // 深度模板视图
};

// =============================================================================
// 描述符堆描述
// =============================================================================

struct DescriptorHeapDesc
{
    DescriptorHeapType type           = DescriptorHeapType::CBV_SRV_UAV;
    uint32_t           numDescriptors = 1;
    bool               shaderVisible  = false; // 是否对着色器可见
    const char*        debugName      = nullptr;
};

// =============================================================================
// 描述符句柄
// =============================================================================

struct DescriptorHandle
{
    uint64_t cpuHandle = 0;
    uint64_t gpuHandle = 0;
    bool     isValid   = false;
};

// =============================================================================
// IDescriptorHeap - 描述符堆接口
// =============================================================================

class RHI_API IDescriptorHeap
{
  public:
    virtual ~IDescriptorHeap() = default;

    /// 分配描述符
    virtual DescriptorHandle Allocate() = 0;

    /// 释放描述符
    virtual void Free(DescriptorHandle handle) = 0;

    /// 获取堆起始 CPU 句柄
    virtual DescriptorHandle GetHeapStart() const = 0;

    /// 获取描述符数量
    virtual uint32_t GetDescriptorCount() const = 0;

    /// 获取已使用描述符数量
    virtual uint32_t GetUsedDescriptorCount() const = 0;
};

} // namespace Engine::RHI