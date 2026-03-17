# Phase 1 Task 010: Triangle Sample 实现

> 优先级：P0  
> 预计时间：4-6 小时  
> 状态：待开始

---

## 📋 任务描述

创建三角形渲染示例，集成所有 Phase 1 核心模块，验证完整的 D3D12 渲染管线。

---

## 🎯 目标

1. 创建 Windows 窗口应用框架
2. 初始化 D3D12 设备和 SwapChain
3. 创建顶点缓冲区和索引缓冲区
4. 创建着色器（顶点/像素）
5. 创建管线状态对象（PSO）
6. 创建根签名
7. 实现渲染循环
8. 集成调试标记
9. 编写编译和运行说明

---

## 📁 文件清单

### 需要创建的文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `Samples/Triangle/main.cpp` | 源文件 | ⬜ 待创建 |
| `Samples/Triangle/TriangleApp.h` | 头文件 | ⬜ 待创建 |
| `Samples/Triangle/TriangleApp.cpp` | 源文件 | ⬜ 待创建 |
| `Samples/Triangle/shaders.hlsl` | 着色器 | ⬜ 待创建 |
| `Samples/Triangle/README.md` | 文档 | ⬜ 待创建 |

### 依赖模块

- D3D12Device - 设备初始化
- D3D12SwapChain - 交换链
- D3D12Buffer - 顶点/索引缓冲
- D3D12CommandList - 命令录制
- D3D12FrameResourceManager - 帧资源管理
- D3D12PipelineState - 管线状态
- D3D12RootSignature - 根签名

---

## 🔧 实现要点

### 1. 顶点结构

```cpp
struct Vertex {
    float3 position : POSITION;
    float3 color : COLOR;
};

// 彩色三角形顶点
const Vertex triangleVertices[] = {
    {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 顶部（红色）
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // 右下（绿色）
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}   // 左下（蓝色）
};
```

### 2. HLSL 着色器

```hlsl
// 顶点着色器
struct VSInput {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}

// 像素着色器
struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
```

### 3. 应用类结构

```cpp
class TriangleApp {
private:
    HWND m_hwnd;
    UINT m_width;
    UINT m_height;
    
    // RHI 模块
    std::unique_ptr<D3D12Device> m_device;
    std::unique_ptr<D3D12SwapChain> m_swapChain;
    std::unique_ptr<D3D12FrameResourceManager> m_frameManager;
    std::unique_ptr<D3D12UploadManager> m_uploadManager;
    
    // 渲染资源
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vbv;
    
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    
    // 状态
    bool m_running;
    
public:
    TriangleApp();
    ~TriangleApp();
    
    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    void Run();
    void Shutdown();
    
private:
    bool CreateWindow(HINSTANCE hInstance, int nCmdShow);
    bool InitializeD3D12();
    bool CreateVertexBuffer();
    bool CreateRootSignature();
    bool CreatePipelineState();
    void RenderFrame();
    void LoadPipeline();
    void PopulateCommandList();
    
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};
```

### 4. 窗口创建

```cpp
bool TriangleApp::CreateWindow(HINSTANCE hInstance, int nCmdShow) {
    m_width = 1280;
    m_height = 720;
    
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TriangleApp";
    RegisterClassEx(&wc);
    
    RECT rect = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    m_hwnd = CreateWindowEx(
        0,
        L"TriangleApp",
        L"自研引擎 - 三角形示例",
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
    
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(m_hwnd, nCmdShow);
    
    return true;
}
```

### 5. D3D12 初始化

```cpp
bool TriangleApp::InitializeD3D12() {
    // 1. 创建设备
    m_device = std::make_unique<D3D12Device>();
    RHIResult result = m_device->Initialize({
        .enableDebug = true,
        .enableValidation = true
    });
    
    if (IsFailure(result)) {
        RHI_LOG_ERROR("Failed to create device: %d", result);
        return false;
    }
    
    // 2. 创建交换链
    m_swapChain = std::make_unique<D3D12SwapChain>();
    result = m_swapChain->Initialize(m_device.get(), m_hwnd, m_width, m_height);
    
    if (IsFailure(result)) {
        RHI_LOG_ERROR("Failed to create swap chain: %d", result);
        return false;
    }
    
    // 3. 创建帧资源管理器
    m_frameManager = std::make_unique<D3D12FrameResourceManager>(m_device.get(), 3);
    result = m_frameManager->Initialize();
    
    if (IsFailure(result)) {
        RHI_LOG_ERROR("Failed to create frame manager: %d", result);
        return false;
    }
    
    // 4. 创建上传管理器
    m_uploadManager = std::make_unique<D3D12UploadManager>(m_device.get());
    result = m_uploadManager->Initialize({});
    
    if (IsFailure(result)) {
        RHI_LOG_ERROR("Failed to create upload manager: %d", result);
        return false;
    }
    
    return true;
}
```

### 6. 顶点缓冲区创建

```cpp
bool TriangleApp::CreateVertexBuffer() {
    const UINT vertexBufferSize = sizeof(triangleVertices);
    
    // 1. 创建默认堆顶点缓冲
    m_vertexBuffer = m_device->CreateCommittedResource(
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr
    );
    
    // 2. 创建上传堆
    ComPtr<ID3D12Resource> uploadBuffer;
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    
    m_device->GetD3D12Device()->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );
    
    // 3. 映射并复制数据
    UINT8* mappedData;
    uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    memcpy(mappedData, triangleVertices, vertexBufferSize);
    uploadBuffer->Unmap(0, nullptr);
    
    // 4. 录制复制命令
    auto cmdList = m_device->CreateCommandList();
    cmdList->Begin();
    
    cmdList->TransitionBarrier(
        m_vertexBuffer.Get(),
        ResourceState::Common,
        ResourceState::CopyDest
    );
    
    cmdList->CopyBufferRegion(
        m_vertexBuffer.Get(), 0,
        uploadBuffer.Get(), 0,
        vertexBufferSize
    );
    
    cmdList->TransitionBarrier(
        m_vertexBuffer.Get(),
        ResourceState::CopyDest,
        ResourceState::VertexBuffer
    );
    
    cmdList->End();
    m_device->SubmitCommandLists({cmdList.get()});
    
    // 等待 GPU 完成
    // ...
    
    // 5. 创建顶点缓冲视图
    m_vbv.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vbv.StrideInBytes = sizeof(Vertex);
    m_vbv.SizeInBytes = vertexBufferSize;
    
    return true;
}
```

### 7. 根签名创建

```cpp
bool TriangleApp::CreateRootSignature() {
    // 空根签名（三角形示例不需要根参数）
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(
        0, nullptr,
        0, nullptr,
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
        RHI_LOG_ERROR("Failed to serialize root signature: 0x%08X", hr);
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
    
    return true;
}
```

### 8. 管线状态创建

```cpp
bool TriangleApp::CreatePipelineState() {
    // 编译着色器
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    
    #ifdef _DEBUG
    DWORD compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
    DWORD compileFlags = 0;
    #endif
    
    // 顶点着色器
    HRESULT hr = D3DCompileFromFile(
        L"shaders.hlsl",
        nullptr, nullptr,
        "VSMain", "vs_5_0",
        compileFlags, 0,
        &vsBlob, nullptr
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to compile vertex shader: 0x%08X", hr);
        return false;
    }
    
    // 像素着色器
    hr = D3DCompileFromFile(
        L"shaders.hlsl",
        nullptr, nullptr,
        "PSMain", "ps_5_0",
        compileFlags, 0,
        &psBlob, nullptr
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to compile pixel shader: 0x%08X", hr);
        return false;
    }
    
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
    psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
    psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    
    hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
        &psoDesc, IID_PPV_ARGS(&m_pipelineState)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to create pipeline state: 0x%08X", hr);
        return false;
    }
    
    return true;
}
```

### 9. 渲染循环

```cpp
void TriangleApp::RenderFrame() {
    // 1. BeginFrame - 等待 GPU 并重置
    m_frameManager->BeginFrame();
    
    // 2. 获取当前帧的命令分配器
    ID3D12CommandAllocator* allocator = m_frameManager->GetCurrentCommandAllocator();
    
    // 3. 创建/重置命令列表
    auto cmdList = m_device->CreateCommandList();
    cmdList->Reset(allocator);
    
    // 4. 设置视口和裁剪
    D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
    cmdList->RSSetViewports(1, &viewport);
    
    D3D12_RECT scissorRect = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
    cmdList->RSSetScissorRects(1, &scissorRect);
    
    // 5. 资源屏障 - 后备缓冲到渲染目标
    auto backBuffer = m_swapChain->GetCurrentBackBuffer();
    cmdList->TransitionBarrier(
        backBuffer,
        ResourceState::Present,
        ResourceState::RenderTarget
    );
    
    // 6. 设置渲染目标
    auto rtv = m_swapChain->GetCurrentRTV();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    
    // 7. 清除渲染目标（深蓝色）
    const float clearColor[] = {0.0f, 0.1f, 0.2f, 1.0f};
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    
    // 8. 设置管线和资源
    cmdList->SetPipelineState(m_pipelineState.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->IASetVertexBuffers(0, 1, &m_vbv);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // 9. 绘制三角形
    cmdList->DrawInstanced(3, 1, 0, 0);
    
    // 10. 资源屏障 - 渲染目标到呈现
    cmdList->TransitionBarrier(
        backBuffer,
        ResourceState::RenderTarget,
        ResourceState::Present
    );
    
    // 11. 结束命令列表
    cmdList->Close();
    
    // 12. 提交命令
    m_device->SubmitCommandLists({cmdList.get()});
    
    // 13. 呈现
    m_swapChain->Present(1, 0);  // 1 = VSync
    
    // 14. EndFrame - 信号围栏并切换帧
    m_frameManager->EndFrame();
}

void TriangleApp::Run() {
    m_running = true;
    
    MSG msg = {};
    while (m_running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (m_running) {
            RenderFrame();
        }
    }
}
```

### 10. 窗口过程

```cpp
LRESULT CALLBACK TriangleApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    TriangleApp* app = reinterpret_cast<TriangleApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    
    switch (message) {
    case WM_PAINT:
        if (app && app->m_running) {
            app->RenderFrame();
        } else {
            ValidateRect(hWnd, nullptr);
        }
        return 0;
        
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            if (app) app->m_running = false;
        }
        return 0;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        
    case WM_SIZE:
        if (app && wParam != SIZE_MINIMIZED) {
            // 处理窗口大小调整
            // app->OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}
```

---

## ✅ 验收标准

### 功能测试

- [ ] 窗口创建成功
- [ ] D3D12 初始化成功
- [ ] 顶点缓冲区创建成功
- [ ] 着色器编译成功
- [ ] 管线状态创建成功
- [ ] 渲染循环正常运行
- [ ] 显示彩色三角形
- [ ] 按 ESC 退出

### 代码质量

- [ ] 使用 ComPtr 管理资源
- [ ] 完整的错误检查
- [ ] 清晰的日志输出
- [ ] 注释完整

### 性能

- [ ] 60 FPS 稳定运行
- [ ] 无内存泄漏
- [ ] CPU/GPU 同步正常

---

## 📚 编译和运行

### 前置要求

- Windows 10/11
- Visual Studio 2022
- Windows SDK 10.0.19041.0+

### 编译步骤

```bash
cd /root/.openclaw/workspace/engine

# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. -G "Visual Studio 17 2022" -A x64 \
    -DENGINE_RHI_D3D12=ON \
    -DENGINE_BUILD_SAMPLES=ON

# 编译
cmake --build . --config Release

# 运行示例
./bin/Release/Sample_Triangle.exe
```

---

## 📝 预期效果

```
┌─────────────────────────────────────────┐
│   自研引擎 - 三角形示例                  │
├─────────────────────────────────────────┤
│                                         │
│              ● (红色)                   │
│             / \                         │
│            /   \                        │
│           /     \                       │
│          /       \                      │
│         /         \                     │
│        /           \                    │
│       ●─────────────●                   │
│    (蓝色)   (绿色)                      │
│                                         │
│   深蓝色背景，彩色三角形                 │
│                                         │
│   FPS: 60 | GPU: NVIDIA RTX 4090        │
└─────────────────────────────────────────┘
```

---

*Task 010 - 龙景 2026-03-17*
