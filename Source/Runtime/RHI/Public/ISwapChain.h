// =============================================================================
// ISwapChain.h - 交换链接口
// =============================================================================

#pragma once

#include "RHICore.h"

namespace Engine::RHI
{

// =============================================================================
// 交换链描述
// =============================================================================

struct SwapChainDesc
{
    void*       windowHandle = nullptr;
    uint32_t    width        = 800;
    uint32_t    height       = 600;
    Format      format       = Format::R8G8B8A8_UNorm;
    uint32_t    bufferCount  = 2;
    bool        vsync        = true;
    const char* debugName    = nullptr;
};

// =============================================================================
// ISwapChain - 交换链接口
// =============================================================================

class RHI_API ISwapChain
{
  public:
    virtual ~ISwapChain() = default;

    /// 获取当前后缓冲区索引
    virtual uint32_t GetCurrentBackBufferIndex() const = 0;

    /// 获取缓冲区数量
    virtual uint32_t GetBufferCount() const = 0;

    /// 获取宽度
    virtual uint32_t GetWidth() const = 0;

    /// 获取高度
    virtual uint32_t GetHeight() const = 0;

    /// 获取格式
    virtual Format GetFormat() const = 0;

    /// 调整大小
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    /// 呈现
    virtual void Present() = 0;

    /// 获取后缓冲区纹理
    virtual ITexture* GetBackBuffer(uint32_t index) = 0;
};

} // namespace Engine::RHI