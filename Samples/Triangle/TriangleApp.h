// =============================================================================
// TriangleApp.h - 三角形渲染应用类
// =============================================================================

#pragma once

#include "RHI.h"
#include "D3D12Device.h"
#include "D3D12SwapChain.h"
#include "D3D12Buffer.h"
#include "D3D12FrameResourceManager.h"

#include <Windows.h>
#include <memory>

namespace Engine {
namespace Samples {

/**
 * @brief 三角形渲染应用
 * 
 * 演示完整的 D3D12 渲染管线初始化和渲染循环。
 */
class TriangleApp {
public:
    TriangleApp();
    ~TriangleApp();
    
    /// 初始化应用
    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    
    /// 运行主循环
    int Run();
    
    /// 关闭应用
    void Shutdown();
    
private:
    // ========== 初始化方法 ==========
    
    /// 创建窗口
    bool CreateWindow(HINSTANCE hInstance);
    
    /// 初始化 D3D12
    bool InitializeD3D12();
    
    /// 创建顶点缓冲区
    bool CreateVertexBuffer();
    
    /// 创建根签名
    bool CreateRootSignature();
    
    /// 创建管线状态
    bool CreatePipelineState();
    
    /// 编译着色器
    bool CompileShaders();
    
    // ========== 渲染方法 ==========
    
    /// 渲染一帧
    void RenderFrame();
    
    /// 录制命令列表
    void PopulateCommandList();
    
    // ========== 事件处理 ==========
    
    /// 窗口大小改变
    void OnResize(UINT width, UINT height);
    
    /// 窗口过程
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
private:
    // 窗口
    HWND m_hwnd = nullptr;
    UINT m_width = 1280;
    UINT m_height = 720;
    bool m_isRunning = false;
    bool m_isMinimized = false;
    
    // D3D12 核心对象
    std::unique_ptr<RHI::D3D12Device> m_device;
    std::unique_ptr<RHI::D3D12SwapChain> m_swapChain;
    std::unique_ptr<RHI::D3D12FrameResourceManager> m_frameManager;
    
    // 渲染资源
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
    
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    
    // 着色器
    ComPtr<ID3DBlob> m_vertexShader;
    ComPtr<ID3DBlob> m_pixelShader;
    
    // 帧追踪
    uint64_t m_frameCount = 0;
    double m_lastFpsUpdate = 0.0;
    uint32_t m_fps = 0;
    
    // 常量
    static constexpr LPCWSTR WINDOW_CLASS = L"TriangleApp";
    static constexpr LPCWSTR WINDOW_TITLE = L"自研引擎 - 三角形示例";
};

} // namespace Samples
} // namespace Engine