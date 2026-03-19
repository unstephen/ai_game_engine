// =============================================================================
// IBuffer.h - 缓冲区接口
// =============================================================================

#pragma once

#include "RHICore.h"

namespace Engine::RHI
{

// =============================================================================
// 缓冲区描述
// =============================================================================

struct BufferDesc
{
    uint64_t      size         = 0;
    uint32_t      stride       = 0; // 结构化缓冲区的元素大小
    BufferUsage   usage        = BufferUsage::None;
    ResourceState initialState = ResourceState::Common;
    const char*   debugName    = nullptr;
    void*         initialData  = nullptr;
};

// =============================================================================
// IBuffer - 缓冲区接口
// =============================================================================

class RHI_API IBuffer
{
  public:
    virtual ~IBuffer() = default;

    /// 获取缓冲区大小
    virtual uint64_t GetSize() const = 0;

    /// 获取缓冲区步长
    virtual uint32_t GetStride() const = 0;

    /// 获取缓冲区用途
    virtual BufferUsage GetUsage() const = 0;

    /// 获取 GPU 虚拟地址
    virtual uint64_t GetGPUVirtualAddress() const = 0;

    /// 映射缓冲区到 CPU 可访问内存
    virtual void* Map() = 0;

    /// 取消映射
    virtual void Unmap() = 0;

    /// 更新缓冲区数据
    virtual void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) = 0;
};

} // namespace Engine::RHI