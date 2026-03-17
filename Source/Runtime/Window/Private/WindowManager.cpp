// =============================================================================
// WindowManager.cpp - 窗口管理器实现
// =============================================================================

#include "WindowManager.h"

namespace Engine {

WindowManager& WindowManager::GetInstance() {
    static WindowManager instance;
    return instance;
}

IWindow* WindowManager::CreateWindow(const WindowDesc& desc) {
    // TODO: 实现窗口创建
    return nullptr;
}

void WindowManager::DestroyWindow(IWindow* window) {
    // TODO: 实现窗口销毁
}

void WindowManager::PollEvents() {
    // TODO: 实现事件轮询
}

void WindowManager::WaitEvents() {
    // TODO: 实现事件等待
}

IWindow* WindowManager::GetWindow(size_t index) const {
    if (index < m_windows.size()) {
        return m_windows[index].get();
    }
    return nullptr;
}

} // namespace Engine