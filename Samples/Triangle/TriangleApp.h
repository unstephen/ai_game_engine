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
#include <wrl/client.h>
#include <memory>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

// D3D12 辅助结构（避免依赖 d3dx12.h）
struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES {
    CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE type) {
        Type = type;
        CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        CreationNodeMask = 1;
        VisibleNodeMask = 1;
    }
};

struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC {
    CD3DX12_RESOURCE_DESC() = default;
    static CD3DX12_RESOURCE_DESC Buffer(UINT64 size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
        CD3DX12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = flags;
        return desc;
    }
};

struct CD3DX12_ROOT_SIGNATURE_DESC : public D3D12_ROOT_SIGNATURE_DESC {
    CD3DX12_ROOT_SIGNATURE_DESC() = default;
    void Init(
        UINT numParameters,
        const D3D12_ROOT_PARAMETER* parameters,
        UINT numStaticSamplers,
        const D3D12_STATIC_SAMPLER_DESC* staticSamplers,
        D3D12_ROOT_SIGNATURE_FLAGS flags) {
        D3D12_ROOT_SIGNATURE_DESC::NumParameters = numParameters;
        D3D12_ROOT_SIGNATURE_DESC::pParameters = parameters;
        D3D12_ROOT_SIGNATURE_DESC::NumStaticSamplers = numStaticSamplers;
        D3D12_ROOT_SIGNATURE_DESC::pStaticSamplers = staticSamplers;
        D3D12_ROOT_SIGNATURE_DESC::Flags = flags;
    }
};

struct CD3DX12_RESOURCE_BARRIER {
    static D3D12_RESOURCE_BARRIER Transition(
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = flags;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        return barrier;
    }
};

// 简化的 D3D12 默认值宏
#define D3D12_DEFAULT 0

struct CD3DX12_RASTERIZER_DESC {
    CD3DX12_RASTERIZER_DESC(int) {
        FillMode = D3D12_FILL_MODE_SOLID;
        CullMode = D3D12_CULL_MODE_BACK;
        FrontCounterClockwise = FALSE;
        DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        DepthClipEnable = TRUE;
        MultisampleEnable = FALSE;
        AntialiasedLineEnable = FALSE;
        ForcedSampleCount = 0;
        ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }
};

struct CD3DX12_BLEND_DESC {
    CD3DX12_BLEND_DESC(int) {
        AlphaToCoverageEnable = FALSE;
        IndependentBlendEnable = FALSE;
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            RenderTarget[i].BlendEnable = FALSE;
            RenderTarget[i].LogicOpEnable = FALSE;
            RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
            RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
            RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
            RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
            RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
            RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
            RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }
    }
};

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