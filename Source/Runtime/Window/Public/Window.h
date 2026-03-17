// =============================================================================
// Window.h - 窗口接口
// =============================================================================

#pragma once

#include "Core.h"

#include <functional>
#include <string>

namespace Engine
{

struct WindowDesc
{
    std::string title        = "Engine";
    uint32_t    width        = 1280;
    uint32_t    height       = 720;
    bool        fullscreen   = false;
    bool        resizable    = true;
    void*       nativeHandle = nullptr;
};

class ENGINE_CORE_API IWindow
{
  public:
    virtual ~IWindow() = default;

    virtual bool Create(const WindowDesc& desc) = 0;
    virtual void Destroy()                      = 0;

    virtual void*    GetNativeHandle() const = 0;
    virtual uint32_t GetWidth() const        = 0;
    virtual uint32_t GetHeight() const       = 0;

    virtual bool ShouldClose() const = 0;
    virtual void PollEvents()        = 0;
    virtual void SwapBuffers()       = 0;
};

// 窗口事件回调
using WindowResizeCallback = std::function<void(uint32_t, uint32_t)>;
using WindowCloseCallback  = std::function<void()>;
using KeyCallback          = std::function<void(int, int, int, int)>;
using MouseCallback        = std::function<void(double, double)>;

} // namespace Engine