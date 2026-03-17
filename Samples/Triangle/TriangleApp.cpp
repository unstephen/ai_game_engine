// =============================================================================
// TriangleApp.cpp - 三角形渲染应用实现
// =============================================================================

#include "TriangleApp.h"
#include "Log.h"

#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>
#include <sstream>

#pragma comment(lib, "d3dcompiler.lib")

namespace Engine {
namespace Samples {

// =============================================================================
// 顶点数据
// =============================================================================

struct Vertex {
    float position[3];
    float color[3];
};

// 彩色三角形顶点
static const Vertex s_triangleVertices[] = {
    {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 顶部（红色）
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // 右下（绿色）
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}   // 左下（蓝色）
};

// =============================================================================
// 构造函数和析构函数
// =============================================================================

TriangleApp::TriangleApp() {
}

TriangleApp::~TriangleApp() {
    Shutdown();
}

// =============================================================================
// 初始化
// =============================================================================

bool TriangleApp::Initialize(HINSTANCE hInstance, int nCmdShow) {
    // 1. 创建窗口
    if (!CreateWindow(hInstance)) {
        RHI_LOG_ERROR("Failed to create window");
        return false;
    }
    
    // 2. 初始化 D3D12
    if (!InitializeD3D12()) {
        RHI_LOG_ERROR("Failed to initialize D3D12");
        return false;
    }
    
    // 3. 创建顶点缓冲区
    if (!CreateVertexBuffer()) {
        RHI_LOG_ERROR("Failed to create vertex buffer");
        return false;
    }
    
    // 4. 创建根签名
    if (!CreateRootSignature()) {
        RHI_LOG_ERROR("Failed to create root signature");
        return false;
    }
    
    // 5. 编译着色器
    if (!CompileShaders()) {
        RHI_LOG_ERROR("Failed to compile shaders");
        return false;
    }
    
    // 6. 创建管线状态
    if (!CreatePipelineState()) {
        RHI_LOG_ERROR("Failed to create pipeline state");
        return false;
    }
    
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
    
    m_isRunning = true;
    
    RHI_LOG_INFO("TriangleApp initialized successfully");
    return true;
}

bool TriangleApp::CreateWindow(HINSTANCE hInstance) {
    // 注册窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS;
    
    if (!RegisterClassExW(&wc)) {
        RHI_LOG_ERROR("Failed to register window class");
        return false;
    }
    
    // 计算窗口大小
    RECT rect = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    // 创建窗口
    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        RHI_LOG_ERROR("Failed to create window");
        return false;
    }
    
    return true;
}

bool TriangleApp::InitializeD3D12() {
    // 1. 创建设备
    m_device = std::make_unique<RHI::D3D12Device>();
    
    RHI::DeviceDesc deviceDesc = {};
    deviceDesc.backend = RHI::Backend::D3D12;
    deviceDesc.enableDebug = true;
    deviceDesc.enableValidation = true;
    
    RHI::RHIResult result = m_device->Initialize(deviceDesc);
    if (RHI::IsFailure(result)) {
        RHI_LOG_ERROR("Failed to create D3D12 device: %s", RHI::GetErrorName(result));
        return false;
    }
    
    // 2. 创建交换链
    m_swapChain = std::make_unique<RHI::D3D12SwapChain>();
    
    RHI::SwapChainDesc swapChainDesc = {};
    swapChainDesc.windowHandle = m_hwnd;
    swapChainDesc.width = m_width;
    swapChainDesc.height = m_height;
    swapChainDesc.format = RHI::Format::R8G8B8A8_UNorm;
    swapChainDesc.bufferCount = 3;
    swapChainDesc.enableVSync = true;
    
    result = m_swapChain->Initialize(m_device.get(), swapChainDesc);
    if (RHI::IsFailure(result)) {
        RHI_LOG_ERROR("Failed to create swap chain: %s", RHI::GetErrorName(result));
        return false;
    }
    
    // 3. 创建帧资源管理器
    m_frameManager = std::make_unique<RHI::D3D12FrameResourceManager>(m_device.get(), 3);
    
    result = m_frameManager->Initialize();
    if (RHI::IsFailure(result)) {
        RHI_LOG_ERROR("Failed to create frame resource manager: %s", RHI::GetErrorName(result));
        return false;
    }
    
    RHI_LOG_INFO("D3D12 initialized successfully");
    RHI_LOG_INFO("Device: %s", m_device->GetDeviceName().data());
    
    auto info = m_device->GetDeviceInfo();
    RHI_LOG_INFO("Dedicated VRAM: %llu MB", info.dedicatedVideoMemory / (1024 * 1024));
    
    return true;
}

bool TriangleApp::CreateVertexBuffer() {
    const UINT vertexBufferSize = sizeof(s_triangleVertices);
    
    ID3D12Device* d3dDevice = m_device->GetD3D12Device();
    
    // 1. 创建默认堆顶点缓冲
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    
    HRESULT hr = d3dDevice->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to create vertex buffer: 0x%08X", hr);
        return false;
    }
    
    m_vertexBuffer->SetName(L"TriangleVertexBuffer");
    
    // 2. 创建上传堆
    ComPtr<ID3D12Resource> uploadBuffer;
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    
    hr = d3dDevice->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to create upload buffer: 0x%08X", hr);
        return false;
    }
    
    // 3. 映射并复制数据
    void* mappedData = nullptr;
    hr = uploadBuffer->Map(0, nullptr, &mappedData);
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to map upload buffer: 0x%08X", hr);
        return false;
    }
    
    memcpy(mappedData, s_triangleVertices, vertexBufferSize);
    uploadBuffer->Unmap(0, nullptr);
    
    // 4. 创建命令列表并复制
    ComPtr<ID3D12CommandAllocator> cmdAllocator;
    d3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&cmdAllocator)
    );
    
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    d3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        cmdAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&cmdList)
    );
    
    // 资源屏障：Common -> CopyDest
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        m_vertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST
    ));
    
    // 复制数据
    cmdList->CopyBufferRegion(
        m_vertexBuffer.Get(), 0,
        uploadBuffer.Get(), 0,
        vertexBufferSize
    );
    
    // 资源屏障：CopyDest -> VertexBuffer
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        m_vertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    ));
    
    cmdList->Close();
    
    // 5. 执行命令
    ID3D12CommandList* cmdLists[] = {cmdList.Get()};
    m_device->GetCommandQueue()->ExecuteCommandLists(1, cmdLists);
    
    // 6. 等待 GPU 完成
    ComPtr<ID3D12Fence> fence;
    d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_device->GetCommandQueue()->Signal(fence.Get(), 1);
    fence->SetEventOnCompletion(1, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
    CloseHandle(fenceEvent);
    
    // 7. 创建顶点缓冲视图
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;
    
    RHI_LOG_INFO("Vertex buffer created: %u bytes", vertexBufferSize);
    return true;
}

bool TriangleApp::CreateRootSignature() {
    // 空根签名（三角形示例不需要根参数）
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(
        0, nullptr,                    // 无根参数
        0, nullptr,                    // 无静态采样器
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error
    );
    
    if (FAILED(hr)) {
        if (error) {
            RHI_LOG_ERROR("Root signature serialization failed: %s", 
                         static_cast<const char*>(error->GetBufferPointer()));
        }
        return false;
    }
    
    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to create root signature: 0x%08X", hr);
        return false;
    }
    
    m_rootSignature->SetName(L"TriangleRootSignature");
    
    RHI_LOG_INFO("Root signature created");
    return true;
}

bool TriangleApp::CompileShaders() {
    // 读取着色器文件
    std::wstring shaderPath = L"shaders.hlsl";
    
#if _DEBUG
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    
    // 编译顶点着色器
    ComPtr<ID3DBlob> vsError;
    HRESULT hr = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr, nullptr,
        "VSMain",
        "vs_5_0",
        compileFlags, 0,
        &m_vertexShader,
        &vsError
    );
    
    if (FAILED(hr)) {
        if (vsError) {
            RHI_LOG_ERROR("Vertex shader compilation failed: %s",
                         static_cast<const char*>(vsError->GetBufferPointer()));
        } else {
            RHI_LOG_ERROR("Failed to load shader file: %ws", shaderPath.c_str());
        }
        return false;
    }
    
    // 编译像素着色器
    ComPtr<ID3DBlob> psError;
    hr = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr, nullptr,
        "PSMain",
        "ps_5_0",
        compileFlags, 0,
        &m_pixelShader,
        &psError
    );
    
    if (FAILED(hr)) {
        if (psError) {
            RHI_LOG_ERROR("Pixel shader compilation failed: %s",
                         static_cast<const char*>(psError->GetBufferPointer()));
        }
        return false;
    }
    
    RHI_LOG_INFO("Shaders compiled successfully");
    return true;
}

bool TriangleApp::CreatePipelineState() {
    // 输入布局
    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    
    // 管线状态描述
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputElements, 2};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = {
        m_vertexShader->GetBufferPointer(),
        m_vertexShader->GetBufferSize()
    };
    psoDesc.PS = {
        m_pixelShader->GetBufferPointer(),
        m_pixelShader->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    
    HRESULT hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_pipelineState)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to create pipeline state: 0x%08X", hr);
        return false;
    }
    
    m_pipelineState->SetName(L"TrianglePipelineState");
    
    RHI_LOG_INFO("Pipeline state created");
    return true;
}

// =============================================================================
// 渲染
// =============================================================================

void TriangleApp::RenderFrame() {
    // 1. 开始帧
    m_frameManager->BeginFrame();
    
    // 2. 获取命令分配器
    ID3D12CommandAllocator* cmdAllocator = m_frameManager->GetCurrentCommandAllocator();
    
    // 3. 创建/重置命令列表
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    m_device->GetD3D12Device()->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        cmdAllocator,
        nullptr,
        IID_PPV_ARGS(&cmdList)
    );
    
    // 4. PIX 标记（如果有 PIX）
#ifdef _PIX_H_
    PIXSetMarker(cmdList.Get(), 0, L"RenderFrame");
#endif
    
    // 5. 设置视口和裁剪
    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);
    
    D3D12_RECT scissorRect = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
    cmdList->RSSetScissorRects(1, &scissorRect);
    
    // 6. 获取当前后备缓冲
    ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBufferResource();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetCurrentRTVHandle();
    
    // 7. 资源屏障：Present -> RenderTarget
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));
    
    // 8. 清除渲染目标
    const float clearColor[] = {0.0f, 0.1f, 0.2f, 1.0f};  // 深蓝色
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    
    // 9. 设置渲染目标
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    
    // 10. 设置管线状态
    cmdList->SetPipelineState(m_pipelineState.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    
    // 11. 设置图元拓扑
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // 12. 绑定顶点缓冲
    cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    
    // 13. 绘制三角形
    cmdList->DrawInstanced(3, 1, 0, 0);
    
    // 14. 资源屏障：RenderTarget -> Present
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));
    
    // 15. 关闭命令列表
    cmdList->Close();
    
    // 16. 提交命令
    ID3D12CommandList* cmdLists[] = {cmdList.Get()};
    m_device->GetCommandQueue()->ExecuteCommandLists(1, cmdLists);
    
    // 17. 结束帧（信号围栏）
    m_frameManager->EndFrame();
    
    // 18. 呈现
    m_swapChain->Present(1, 0);  // 1 = VSync
    
    // 更新帧计数
    m_frameCount++;
}

int TriangleApp::Run() {
    MSG msg = {};
    
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
    
    while (m_isRunning) {
        // 处理消息
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_isRunning = false;
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (m_isRunning && !m_isMinimized) {
            // 渲染帧
            RenderFrame();
        }
    }
    
    return static_cast<int>(msg.wParam);
}

// =============================================================================
// 事件处理
// =============================================================================

void TriangleApp::OnResize(UINT width, UINT height) {
    if (width == 0 || height == 0) {
        m_isMinimized = true;
        return;
    }
    
    m_isMinimized = false;
    m_width = width;
    m_height = height;
    
    if (m_swapChain) {
        // 等待 GPU 空闲
        // ...
        
        // 调整交换链大小
        m_swapChain->ResizeBuffers(width, height, RHI::Format::R8G8B8A8_UNorm);
    }
}

LRESULT CALLBACK TriangleApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    TriangleApp* app = reinterpret_cast<TriangleApp*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
    );
    
    switch (message) {
    case WM_CREATE:
        {
            LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 
                            reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        }
        return 0;
        
    case WM_PAINT:
        if (app && app->m_isRunning) {
            app->RenderFrame();
        }
        ValidateRect(hWnd, nullptr);
        return 0;
        
    case WM_SIZE:
        if (app) {
            app->OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
        
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            if (app) app->m_isRunning = false;
        }
        return 0;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// =============================================================================
// 清理
// =============================================================================

void TriangleApp::Shutdown() {
    m_isRunning = false;
    
    // 等待 GPU 完成所有工作
    if (m_device && m_frameManager) {
        m_frameManager->WaitForAllFrames();
    }
    
    // 按依赖顺序释放资源
    m_pipelineState.Reset();
    m_rootSignature.Reset();
    m_pixelShader.Reset();
    m_vertexShader.Reset();
    m_vertexBuffer.Reset();
    
    m_frameManager.reset();
    m_swapChain.reset();
    m_device.reset();
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    RHI_LOG_INFO("TriangleApp shutdown complete");
}

} // namespace Samples
} // namespace Engine