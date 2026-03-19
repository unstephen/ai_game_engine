// =============================================================================
// IResource.h - 资源基类接口
// =============================================================================

#pragma once

#include "RHICore.h"

namespace Engine::RHI
{

// =============================================================================
// IResource - 资源基类接口
// =============================================================================

class RHI_API IResource
{
  public:
    virtual ~IResource() = default;

    /// 获取资源状态
    virtual ResourceState GetState() const = 0;

    /// 设置资源状态
    virtual void SetState(ResourceState state) = 0;

    /// 获取调试名称
    virtual const char* GetDebugName() const = 0;

    /// 设置调试名称
    virtual void SetDebugName(const char* name) = 0;
};

} // namespace Engine::RHI