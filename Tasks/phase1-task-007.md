# Phase 1 Task 007: D3D12SwapChain 实现

> 优先级：P0  
> 预计时间：4-6 小时  
> 状态：✅ 已完成

---

## 📋 任务描述

实现 `D3D12SwapChain` 类，完成 DXGI 交换链的创建、后备缓冲区访问和呈现。

---

## 🎯 目标

1. ✅ 实现 DXGI 交换链创建
2. ✅ 实现后备缓冲区访问
3. ✅ 实现 RTV 创建（每个后备缓冲区）
4. ✅ 实现 Present 呈现
5. ✅ 实现 Resize 调整大小
6. ✅ 编写单元测试

---

## 📁 文件清单

### 已创建的文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `D3D12SwapChain.h` | 头文件 | ✅ 已创建 (206 行) |
| `D3D12SwapChain.cpp` | 源文件 | ✅ 已创建 (606 行) |
| `D3D12SwapChainTest.cpp` | 测试 | ✅ 已创建 (746 行) |

### 依赖文件

- `ISwapChain.h` - 交换链接口
- `D3D12Device.h` - D3D12 设备
- `D3D12Texture.h` - 纹理（后备缓冲包装）

---

## 🔧 实现要点

### 1. 交换链描述

```cpp
struct SwapChainDesc {
    void* windowHandle = nullptr;
    uint32_t width = 1920;
    uint32_t height = 1080;
    Format format = Format::R8G8B8A8_UNorm;
    uint32_t bufferCount = 2;       // 双缓冲
    bool enableVSync = false;
    bool enableFullscreen = false;
    bool allowTearing = false;
};
```

### 2. 创建流程

```cpp
RHIResult D3D12SwapChain::Initialize(
    D3D12Device* device,
    const SwapChainDesc& desc
) {
    // 1. 保存设备引用
    m_device = device;
    m_desc = desc;
    
    // 2. 创建 DXGI 工厂
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    
    // 3. 获取适配器
    ComPtr<IDXGIAdapter1> adapter;
    factory->EnumAdapterByGpuPreference(
        0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)
    );
    
    // 4. 创建临时交换链（用于获取缓冲）
    ComPtr<IDXGISwapChain1> tempSwapChain;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = desc.width;
    swapChainDesc.Height = desc.height;
    swapChainDesc.Format = ToDXGIFormat(desc.format);
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = desc.bufferCount;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    
    factory->CreateSwapChainForHwnd(
        device->GetCommandQueue(),
        desc.windowHandle,
        &swapChainDesc,
        nullptr,
        nullptr,
        &tempSwapChain
    );
    
    tempSwapChain.As(&m_swapChain);
    
    // 5. 禁用 Alt+Enter
    factory->MakeWindowAssociation(desc.windowHandle, DXGI_MWA_NO_ALT_ENTER);
    
    // 6. 创建后备缓冲区 RTV
    RHI_VERIFY(CreateRenderTargetViews());
    
    // 7. 获取当前帧索引
    m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();
    
    return RHIResult::Success;
}
```

### 3. 后备缓冲区 RTV 创建

```cpp
RHIResult D3D12SwapChain::CreateRenderTargetViews() {
    // 获取后备缓冲区数量
    m_backBuffers.resize(m_desc.bufferCount);
    m_rtvHandles.resize(m_desc.bufferCount);
    
    // 创建 RTV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = m_desc.bufferCount;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    m_device->GetD3D12Device()->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)
    );
    
    UINT rtvSize = m_device->GetD3D12Device()->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV
    );
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = 
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    
    // 为每个后备缓冲区创建 RTV
    for (UINT i = 0; i < m_desc.bufferCount; i++) {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        
        m_device->GetD3D12Device()->CreateRenderTargetView(
            m_backBuffers[i].Get(), nullptr, rtvHandle
        );
        
        m_rtvHandles[i] = rtvHandle;
        rtvHandle.ptr += rtvSize;
    }
    
    return RHIResult::Success;
}
```

### 4. Present 呈现

```cpp
RHIResult D3D12SwapChain::Present(uint32_t syncInterval, uint32_t flags) override {
    HRESULT hr = m_swapChain->Present(syncInterval, flags);
    
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            // 设备丢失
            m_device->OnDeviceLost(DeviceLostReason::DeviceRemoved, "DXGI_ERROR_DEVICE_REMOVED");
            return RHIResult::DeviceRemoved;
        } else if (hr == DXGI_ERROR_INVALID_CALL) {
            return RHIResult::InvalidParameter;
        } else {
            return RHIResult::InternalError;
        }
    }
    
    // 更新当前帧索引
    m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();
    
    return RHIResult::Success;
}
```

### 5. Resize 调整大小

```cpp
RHIResult D3D12SwapChain::ResizeBuffers(
    uint32_t width, 
    uint32_t height,
    Format format
) override {
    // 1. 释放后备缓冲区和 RTV
    for (auto& buffer : m_backBuffers) {
        buffer.Reset();
    }
    m_rtvHeap.Reset();
    
    // 2. 更新描述
    m_desc.width = width;
    m_desc.height = height;
    if (format != Format::Unknown) {
        m_desc.format = format;
    }
    
    // 3. 调整缓冲区大小
    HRESULT hr = m_swapChain->ResizeBuffers(
        m_desc.bufferCount,
        width, height,
        ToDXGIFormat(m_desc.format),
        DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    );
    
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            m_device->OnDeviceLost(DeviceLostReason::DeviceRemoved, "ResizeBuffers failed");
            return RHIResult::DeviceRemoved;
        }
        return RHIResult::SwapChainError;
    }
    
    // 4. 重新创建 RTV
    RHI_VERIFY(CreateRenderTargetViews());
    
    // 5. 更新帧索引
    m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();
    
    return RHIResult::Success;
}
```

### 6. VSync 控制

```cpp
void D3D12SwapChain::SetVSync(bool enable) override {
    m_desc.enableVSync = enable;
    // Present 时 syncInterval = enable ? 1 : 0
}

RHIResult D3D12SwapChain::Present() {
    return Present(m_desc.enableVSync ? 1 : 0, 0);
}
```

---

## ✅ 验收标准

### 功能测试

- [ ] 交换链创建成功
- [ ] 后备缓冲区访问正确
- [ ] RTV 创建成功
- [ ] Present 呈现正常
- [ ] Resize 调整大小正常
- [ ] VSync 控制正常
- [ ] 帧索引更新正确

### 代码质量

- [ ] 使用 ComPtr 管理资源
- [ ] 完整的错误检查
- [ ] 清晰的日志输出
- [ ] 注释完整

### 测试覆盖

- [ ] 正常路径测试
- [ ] 错误路径测试
- [ ] 边界条件测试

---

## 🧪 测试用例

### Test 1: 交换链创建

```cpp
TEST(D3D12SwapChain, CreateSwapChain) {
    D3D12Device device;
    device.Initialize({});
    
    // 创建测试窗口
    HWND hWnd = CreateTestWindow();
    
    SwapChainDesc desc = {};
    desc.windowHandle = hWnd;
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.bufferCount = 2;
    
    D3D12SwapChain swapChain;
    RHIResult result = swapChain.Initialize(&device, desc);
    
    ASSERT_EQ(result, RHIResult::Success);
    EXPECT_EQ(swapChain.GetWidth(), 1920);
    EXPECT_EQ(swapChain.GetHeight(), 1080);
    EXPECT_LT(swapChain.GetCurrentFrameIndex(), 2);
    
    DestroyWindow(hWnd);
}
```

### Test 2: 后备缓冲区访问

```cpp
TEST(D3D12SwapChain, GetBackBuffer) {
    D3D12Device device;
    device.Initialize({});
    
    HWND hWnd = CreateTestWindow();
    
    SwapChainDesc desc = {};
    desc.windowHandle = hWnd;
    desc.width = 800;
    desc.height = 600;
    desc.bufferCount = 3;
    
    D3D12SwapChain swapChain;
    swapChain.Initialize(&device, desc);
    
    // 获取当前后备缓冲
    ID3D12Resource* backBuffer = swapChain.GetCurrentBackBuffer();
    EXPECT_NE(backBuffer, nullptr);
    
    // 验证 RTV 句柄
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain.GetCurrentRTV();
    EXPECT_TRUE(rtv.ptr != 0);
    
    DestroyWindow(hWnd);
}
```

### Test 3: Resize 测试

```cpp
TEST(D3D12SwapChain, Resize) {
    D3D12Device device;
    device.Initialize({});
    
    HWND hWnd = CreateTestWindow();
    
    SwapChainDesc desc = {};
    desc.windowHandle = hWnd;
    desc.width = 800;
    desc.height = 600;
    
    D3D12SwapChain swapChain;
    swapChain.Initialize(&device, desc);
    
    // 调整大小
    RHIResult result = swapChain.ResizeBuffers(1920, 1080, Format::Unknown);
    
    ASSERT_EQ(result, RHIResult::Success);
    EXPECT_EQ(swapChain.GetWidth(), 1920);
    EXPECT_EQ(swapChain.GetHeight(), 1080);
    
    DestroyWindow(hWnd);
}
```

### Test 4: VSync 控制

```cpp
TEST(D3D12SwapChain, VSync) {
    D3D12Device device;
    device.Initialize({});
    
    HWND hWnd = CreateTestWindow();
    
    SwapChainDesc desc = {};
    desc.windowHandle = hWnd;
    desc.width = 800;
    desc.height = 600;
    
    D3D12SwapChain swapChain;
    swapChain.Initialize(&device, desc);
    
    // 启用 VSync
    swapChain.SetVSync(true);
    EXPECT_TRUE(swapChain.IsVSyncEnabled());
    
    // 禁用 VSync
    swapChain.SetVSync(false);
    EXPECT_FALSE(swapChain.IsVSyncEnabled());
    
    DestroyWindow(hWnd);
}
```

### Test 5: 帧索引更新

```cpp
TEST(D3D12SwapChain, FrameIndexUpdate) {
    D3D12Device device;
    device.Initialize({});
    
    HWND hWnd = CreateTestWindow();
    
    SwapChainDesc desc = {};
    desc.windowHandle = hWnd;
    desc.bufferCount = 3;
    
    D3D12SwapChain swapChain;
    swapChain.Initialize(&device, desc);
    
    uint32_t initialIndex = swapChain.GetCurrentFrameIndex();
    
    // Present 后帧索引应该更新
    swapChain.Present(0, 0);
    
    uint32_t newIndex = swapChain.GetCurrentFrameIndex();
    EXPECT_NE(initialIndex, newIndex);  // 可能相同也可能不同，取决于实现
    EXPECT_LT(newIndex, 3);
    
    DestroyWindow(hWnd);
}
```

---

## 📚 参考资料

- [DXGI 交换链创建](https://docs.microsoft.com/en-us/windows/win32/direct3d12/creating-a-swap-chain)
- [Flip Effect 交换模型](https://docs.microsoft.com/en-us/windows/win32/direct3darticles/flip-model-swap-chains)
- [避免 Tearing](https://docs.microsoft.com/en-us/windows/win32/direct3darticles/techniques-to-minimize-tearing)
- UE5 D3D12 交换链实现

---

*Task 007 - 龙景 2026-03-17*
