// =============================================================================
// main.cpp - 三角形示例入口点
// =============================================================================

#include "TriangleApp.h"

using namespace Engine::Samples;

/// Windows 入口点
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 创建应用
    TriangleApp app;
    
    // 初始化
    if (!app.Initialize(hInstance, nCmdShow)) {
        MessageBoxA(nullptr, "初始化失败", "错误", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // 运行主循环
    int result = app.Run();
    
    // 清理
    app.Shutdown();
    
    return result;
}