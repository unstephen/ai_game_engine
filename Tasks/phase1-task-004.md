# Phase 1 Task 004: D3D12FrameResourceManager 实现

> 优先级：P0  
> 预计时间：4-6 小时  
> 状态：待开始

---

## 📋 任务描述

实现 `D3D12FrameResourceManager` 类，完成三缓冲帧资源管理、围栏同步和临时资源清理。

---

## 🎯 目标

1. 实现三缓冲帧上下文管理
2. 实现围栏同步机制
3. 实现临时资源生命周期管理
4. 实现命令分配器每帧重置
5. 实现描述符分配器每帧重置
6. 编写单元测试

---

## 📁 文件清单

### 需要创建的文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `D3D12FrameResourceManager.h` | 头文件 | ⬜ 待创建 |
| `D3D12FrameResourceManager.cpp` | 源文件 | ⬜ 待创建 |
| `D3D12FrameResourceManagerTest.cpp` | 测试 | ⬜ 待创建 |

### 依赖文件

- `IFrameResourceManager.h` - 帧资源管理器接口
- `D3D12Device.h` - D3D12 设备
- `D3D12Buffer.h` - 缓冲区
- `D3D12Texture.h` - 纹理

---

## 🔧 实现要点

### 1. 帧上下文结构

```cpp
struct D3D12FrameContext {
    uint32_t frameIndex;
    uint64_t fenceValue;
    
    // 围栏
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent;
    
    // 命令分配器
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    
    // 描述符分配器（每帧重置）
    std::unique_ptr<D3D12DescriptorAllocator> descriptorAllocator;
    
    // 上传缓冲区（GPU 完成后释放）
    std::vector<ComPtr<ID3D12Resource>> uploadBuffers;
    
    // 临时资源（GPU 完成后释放）
    std::vector<ComPtr<ID3D12Resource>> temporaryResources;
    
    // 延迟删除资源
    std::vector<ID3D12Resource*> pendingDeletions;
    
    // 状态
    bool isGpuComplete = false;
};
```

### 2. 三缓冲管理

```cpp
class D3D12FrameResourceManager : public IFrameResourceManager {
private:
    D3D12Device* m_device;
    uint32_t m_numFrames;
    uint32_t m_currentFrameIndex;
    uint64_t m_nextFenceValue;
    
    std::vector<D3D12FrameContext> m_frameContexts;
    
public:
    D3D12FrameResourceManager(D3D12Device* device, uint32_t numFrames = 3)
        : m_device(device)
        , m_numFrames(numFrames)
        , m_currentFrameIndex(0)
        , m_nextFenceValue(1)
    {
        m_frameContexts.resize(numFrames);
    }
    
    RHIResult Initialize() override {
        for (uint32_t i = 0; i < m_numFrames; i++) {
            D3D12FrameContext& ctx = m_frameContexts[i];
            ctx.frameIndex = i;
            ctx.fenceValue = 0;
            
            // 创建围栏
            ID3D12Device* d3dDevice = m_device->GetD3D12Device();
            d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx.fence));
            
            // 创建围栏事件
            ctx.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!ctx.fenceEvent) {
                return RHIResult::OutOfMemory;
            }
            
            // 创建命令分配器
            d3dDevice->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&ctx.commandAllocator)
            );
            
            // 创建描述符分配器
            ctx.descriptorAllocator = std::make_unique<D3D12DescriptorAllocator>(
                m_device,
                DescriptorAllocatorConfig{}
            );
        }
        
        return RHIResult::Success;
    }
};
```

### 3. BeginFrame - 等待 GPU 并重置

```cpp
RHIResult D3D12FrameResourceManager::BeginFrame() override {
    D3D12FrameContext& ctx = m_frameContexts[m_currentFrameIndex];
    
    // 1. 等待 GPU 完成该帧
    if (ctx.fenceValue > 0) {
        HRESULT hr = WaitForFence(ctx.fence.Get(), ctx.fenceValue, ctx.fenceEvent);
        if (FAILED(hr)) {
            return RHIResult::InternalError;
        }
    }
    
    // 2. 重置命令分配器
    HRESULT hr = ctx.commandAllocator->Reset();
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to reset command allocator");
        return RHIResult::InternalError;
    }
    
    // 3. 重置描述符分配器（Shader Visible 部分）
    ctx.descriptorAllocator->Reset();
    
    // 4. 清理上传缓冲区
    ctx.uploadBuffers.clear();
    
    // 5. 清理临时资源
    ctx.temporaryResources.clear();
    
    // 6. 处理延迟删除
    for (ID3D12Resource* resource : ctx.pendingDeletions) {
        if (resource) {
            resource->Release();
        }
    }
    ctx.pendingDeletions.clear();
    
    return RHIResult::Success;
}
```

### 4. EndFrame - 提交并信号围栏

```cpp
RHIResult D3D12FrameResourceManager::EndFrame() override {
    D3D12FrameContext& ctx = m_frameContexts[m_currentFrameIndex];
    
    // 1. 更新围栏值
    ctx.fenceValue = m_nextFenceValue++;
    
    // 2. 信号围栏（由调用者在提交命令列表后执行）
    // m_device->GetCommandQueue()->Signal(ctx.fence.Get(), ctx.fenceValue);
    
    // 3. 切换到下一帧
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_numFrames;
    
    return RHIResult::Success;
}
```

### 5. 资源注册

```cpp
void D3D12FrameResourceManager::RegisterUploadBuffer(ID3D12Resource* buffer) override {
    if (buffer) {
        m_frameContexts[m_currentFrameIndex].uploadBuffers.push_back(buffer);
    }
}

void D3D12FrameResourceManager::RegisterTemporaryResource(ID3D12Resource* resource) override {
    if (resource) {
        m_frameContexts[m_currentFrameIndex].temporaryResources.push_back(resource);
    }
}

void D3D12FrameResourceManager::DelayedDeleteResource(ID3D12Resource* resource) override {
    if (resource) {
        m_frameContexts[m_currentFrameIndex].pendingDeletions.push_back(resource);
    }
}
```

### 6. 围栏等待辅助函数

```cpp
HRESULT D3D12FrameResourceManager::WaitForFence(
    ID3D12Fence* fence,
    uint64_t fenceValue,
    HANDLE fenceEvent
) {
    // 检查是否已完成
    if (fence->GetCompletedValue() >= fenceValue) {
        return S_OK;
    }
    
    // 设置事件通知
    HRESULT hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to set fence event");
        return hr;
    }
    
    // 等待事件
    WaitForSingleObject(fenceEvent, INFINITE);
    
    return S_OK;
}
```

### 7. 获取当前帧资源

```cpp
ID3D12CommandAllocator* D3D12FrameResourceManager::GetCurrentCommandAllocator() override {
    return m_frameContexts[m_currentFrameIndex].commandAllocator.Get();
}

D3D12DescriptorAllocator* D3D12FrameResourceManager::GetCurrentDescriptorAllocator() override {
    return m_frameContexts[m_currentFrameIndex].descriptorAllocator.get();
}

uint32_t D3D12FrameResourceManager::GetCurrentFrameIndex() const override {
    return m_currentFrameIndex;
}

uint64_t D3D12FrameResourceManager::GetCurrentFenceValue() const override {
    return m_frameContexts[m_currentFrameIndex].fenceValue;
}
```

---

## ✅ 验收标准

### 功能测试

- [ ] 三缓冲初始化成功
- [ ] BeginFrame 等待 GPU 完成
- [ ] EndFrame 信号围栏
- [ ] 命令分配器每帧重置
- [ ] 描述符分配器每帧重置
- [ ] 临时资源正确清理

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

### Test 1: 初始化

```cpp
TEST(D3D12FrameResourceManager, Initialize) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12FrameResourceManager manager(&device, 3);
    RHIResult result = manager.Initialize();
    
    ASSERT_EQ(result, RHIResult::Success);
    EXPECT_EQ(manager.GetCurrentFrameIndex(), 0);
    EXPECT_NE(manager.GetCurrentCommandAllocator(), nullptr);
}
```

### Test 2: BeginFrame/EndFrame 循环

```cpp
TEST(D3D12FrameResourceManager, BeginEndFrame) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12FrameResourceManager manager(&device, 3);
    manager.Initialize();
    
    // 模拟 3 帧
    for (int i = 0; i < 3; i++) {
        RHIResult beginResult = manager.BeginFrame();
        ASSERT_EQ(beginResult, RHIResult::Success);
        
        // 模拟命令列表录制...
        
        RHIResult endResult = manager.EndFrame();
        ASSERT_EQ(endResult, RHIResult::Success);
    }
    
    // 帧索引应该循环
    EXPECT_EQ(manager.GetCurrentFrameIndex(), 0);
}
```

### Test 3: 资源注册和清理

```cpp
TEST(D3D12FrameResourceManager, ResourceRegistration) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12FrameResourceManager manager(&device, 3);
    manager.Initialize();
    
    // 创建测试资源
    ComPtr<ID3D12Resource> testBuffer;
    device.GetD3D12Device()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(1024),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&testBuffer)
    );
    
    // 注册上传缓冲区
    manager.RegisterUploadBuffer(testBuffer.Get());
    
    // 验证已注册
    // （实际验证需要访问内部状态）
    
    // 模拟帧循环，资源应该被清理
    manager.BeginFrame();
    manager.EndFrame();
}
```

### Test 4: 围栏同步

```cpp
TEST(D3D12FrameResourceManager, FenceSync) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12FrameResourceManager manager(&device, 3);
    manager.Initialize();
    
    // 多帧循环，验证围栏值递增
    uint64_t initialFenceValue = manager.GetCurrentFenceValue();
    
    for (int i = 0; i < 3; i++) {
        manager.BeginFrame();
        manager.EndFrame();
    }
    
    // 围栏值应该增加
    EXPECT_GT(manager.GetCurrentFenceValue(), initialFenceValue);
}
```

### Test 5: 描述符分配器重置

```cpp
TEST(D3D12FrameResourceManager, DescriptorAllocatorReset) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12FrameResourceManager manager(&device, 3);
    manager.Initialize();
    
    // 获取描述符分配器
    auto* allocator = manager.GetCurrentDescriptorAllocator();
    
    // 分配描述符
    auto handle1 = allocator->AllocateSRV(/* ... */);
    auto handle2 = allocator->AllocateUAV(/* ... */);
    
    // 重置后应该可以重新分配
    manager.BeginFrame();  // 这会重置描述符分配器
    
    // 描述符分配器已重置，可以重新分配
    // （具体验证取决于实现）
}
```

---

## 📚 参考资料

- [DX12 帧资源管理](https://docs.microsoft.com/en-us/windows/win32/direct3d12/frame-latency)
- [三缓冲技术](https://en.wikipedia.org/wiki/Multiple_buffering)
- [围栏同步](https://docs.microsoft.com/en-us/windows/win32/direct3d12/synchronization)
- UE5 D3D12 帧资源实现

---

*Task 004 - 龙景 2026-03-17*
