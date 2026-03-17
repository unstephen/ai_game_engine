// =============================================================================
// ITexture.h - 纹理接口
// =============================================================================

#pragma once

#include "RHICore.h"

namespace Engine::RHI {

// =============================================================================
// 纹理描述
// =============================================================================

struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    Format format = Format::Unknown;
    TextureUsage usage = TextureUsage::None;
    ResourceState initialState = ResourceState::Common;
    const char* debugName = nullptr;
    void* initialData = nullptr;
};

// =============================================================================
// ITexture - 纹理接口
// =============================================================================

class RHI_API ITexture {
public:
    virtual ~ITexture() = default;
    
    /// 获取纹理宽度
    virtual uint32_t GetWidth() const = 0;
    
    /// 获取纹理高度
    virtual uint32_t GetHeight() const = 0;
    
    /// 获取纹理深度
    virtual uint32_t GetDepth() const = 0;
    
    /// 获取 Mip 级别数
    virtual uint32_t GetMipLevels() const = 0;
    
    /// 获取数组大小
    virtual uint32_t GetArraySize() const = 0;
    
    /// 获取像素格式
    virtual Format GetFormat() const = 0;
    
    /// 获取纹理用途
    virtual TextureUsage GetUsage() const = 0;
};

} // namespace Engine::RHI