// =============================================================================
// IRootSignature.h - 根签名接口
// =============================================================================

#pragma once

#include "RHICore.h"
#include <vector>

namespace Engine::RHI {

// =============================================================================
// 根参数类型
// =============================================================================

enum class RootParameterType : uint8_t {
    Constant32Bit,
    DescriptorTable,
    RootDescriptor,
};

// =============================================================================
// 根签名描述
// =============================================================================

struct RootSignatureDesc {
    // TODO: 更详细的根签名参数
    uint32_t numParameters = 0;
    uint32_t numStaticSamplers = 0;
    const char* debugName = nullptr;
};

// =============================================================================
// IRootSignature - 根签名接口
// =============================================================================

class RHI_API IRootSignature {
public:
    virtual ~IRootSignature() = default;
    
    /// 获取根签名句柄
    virtual void* GetNativeHandle() const = 0;
};

} // namespace Engine::RHI