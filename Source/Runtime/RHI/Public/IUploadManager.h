// =============================================================================
// IUploadManager.h - 上传管理器接口
// =============================================================================

#pragma once

#include <cstddef>
#include <cstdint>

namespace Engine::RHI
{

// =============================================================================
// IUploadManager - 上传管理器接口
// =============================================================================

class RHI_API IUploadManager
{
  public:
    virtual ~IUploadManager() = default;

    /// 分配上传内存
    virtual void* Allocate(uint64_t size, uint64_t alignment = 256) = 0;

    /// 提交上传
    virtual void Submit() = 0;

    /// 等待上传完成
    virtual void WaitForCompletion() = 0;

    /// 获取总上传内存大小
    virtual uint64_t GetTotalMemory() const = 0;

    /// 获取已使用上传内存大小
    virtual uint64_t GetUsedMemory() const = 0;

    /// 重置上传缓冲区
    virtual void Reset() = 0;
};

} // namespace Engine::RHI