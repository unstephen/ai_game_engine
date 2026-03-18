// =============================================================================
// WindowManager.h - 窗口管理器
// =============================================================================

#pragma once

#include "Window.h"

#include <memory>
#include <vector>

// 防止 Windows API 宏污染
#ifdef CreateWindowA
#undef CreateWindowA
#endif
#ifdef CreateWindowW
#undef CreateWindowW
#endif
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef DestroyWindow
#undef DestroyWindow
#endif

namespace Engine
{

class ENGINE_CORE_API WindowManager
{
  public:
    static WindowManager& GetInstance();

    IWindow* CreateWindow(const WindowDesc& desc);
    void     DestroyWindow(IWindow* window);

    void PollEvents();
    void WaitEvents();

    size_t   GetWindowCount() const { return m_windows.size(); }
    IWindow* GetWindow(size_t index) const;

  private:
    WindowManager() = default;
    std::vector<std::unique_ptr<IWindow>> m_windows;
};

} // namespace Engine