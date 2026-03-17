# Phase 1 Task 006: D3D12CommandList 实现

> 优先级：P0  
> 预计时间：6-8 小时  
> 状态：待开始

---

## 📋 任务描述

实现 `D3D12CommandList` 类，完成 D3D12 命令列表的录制、资源绑定、绘制命令和状态管理。

---

## 🎯 目标

1. 实现命令列表生命周期管理（Begin/End/Reset）
2. 实现管线状态绑定
3. 实现资源绑定（顶点/索引/常量缓冲）
4. 实现绘制命令（Draw/DrawIndexed）
5. 实现资源屏障和状态转换
6. 实现渲染过程（RenderPass）
7. 实现调试标记
8. 编写单元测试

---

## 📁 文件清单

### 需要创建的文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `D3D12CommandList.h` | 头文件 | ⬜ 待创建 |
| `D3D12CommandList.cpp` | 源文件 | ⬜ 待创建 |
| `D3D12CommandListTest.cpp` | 测试 | ⬜ 待创建 |

### 依赖文件

- `ICommandList.h` - 命令列表接口
- `D3D12Device.h` - D3D12 设备
- `D3D12Buffer.h` - 缓冲区
- `D3D12Texture.h` - 纹理
- `D3D12PipelineState.h` - 管线状态（Task 009）

---

## 🔧 实现要点

### 1. 命令列表类结构

```cpp
class D3D12CommandList : public ICommandList {
private:
    D3D12Device* m_device;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ID3D12CommandAllocator* m_commandAllocator;
    
    bool m_isRecording = false;
    bool m_isClosed = false;
    
    // 当前状态追踪
    D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D12PipelineState* m_currentPipelineState = nullptr;
    ID3D12DescriptorHeap* m_currentDescriptorHeap = nullptr;
    
    // 当前绑定的资源
    struct {
        ID3D12Buffer* vertexBuffers[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        UINT vertexBufferStrides[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        UINT vertexBufferOffsets[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        ID3D12Buffer* indexBuffer;
        DXGI_FORMAT indexFormat;
        UINT64 indexBufferOffset;
    } m_boundResources;
    
    // 当前渲染目标
    struct {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        D3D12_CPU_DESCRIPTOR_HANDLE dsv;
        UINT numRenderTargets;
        bool hasDepthStencil;
    } m_currentRenderTargets;
    
public:
    D3D12CommandList(D3D12Device* device);
    virtual ~D3D12CommandList() override;
    
    // 初始化
    RHIResult Initialize(ID3D12CommandAllocator* allocator);
    
    // ICommandList 接口
    virtual void Begin() override;
    virtual void End() override;
    virtual void Reset() override;
    
    // 状态设置
    virtual void SetViewport(const Viewport& viewport) override;
    virtual void SetScissorRect(const Rect2D& rect) override;
    virtual void SetPipelineState(IPipelineState* pipeline) override;
    virtual void SetRootSignature(IRootSignature* rootSig) override;
    
    // 资源绑定
    virtual void BindVertexBuffer(uint32_t slot, IBuffer* buffer, 
                                  uint64_t offset, uint32_t stride) override;
    virtual void BindIndexBuffer(IBuffer* buffer, uint64_t offset, 
                                 Format format) override;
    virtual void BindConstantBuffer(uint32_t slot, IBuffer* buffer,
                                    uint64_t offset, uint64_t size) override;
    
    // 绘制
    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount,
                     uint32_t startVertex, uint32_t startInstance) override;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                            uint32_t startIndex, int32_t vertexOffset,
                            uint32_t startInstance) override;
    virtual void DispatchCompute(uint32_t groupCountX, uint32_t groupCountY,
                                 uint32_t groupCountZ) override;
    
    // 资源管理
    virtual void TransitionBarrier(IResource* resource,
                                   ResourceState before,
                                   ResourceState after) override;
    virtual void Barriers(std::span<const ResourceBarrier> barriers) override;
    
    // 渲染过程
    virtual void BeginRenderPass(
        std::span<const RenderTargetAttachment> attachments,
        const DepthStencilAttachment* depthStencil) override;
    virtual void EndRenderPass() override;
    
    // 复制
    virtual void CopyBuffer(IBuffer* dest, uint64_t destOffset,
                           IBuffer* src, uint64_t srcOffset,
                           uint64_t size) override;
    
    // 调试
    virtual void SetMarker(const char* name) override;
    virtual void BeginEvent(const char* name) override;
    virtual void EndEvent() override;
    
    // D3D12 特定
    ID3D12GraphicsCommandList* GetD3D12CommandList() const { return m_commandList.Get(); }
    bool IsRecording() const { return m_isRecording; }
    bool IsClosed() const { return m_isClosed; }
};
```

### 2. 初始化

```cpp
D3D12CommandList::D3D12CommandList(D3D12Device* device)
    : m_device(device)
    , m_commandAllocator(nullptr)
    , m_isRecording(false)
    , m_isClosed(false)
{
    // 初始化绑定资源状态
    ZeroMemory(&m_bound_resources, sizeof(m_bound_resources));
    ZeroMemory(&m_currentRenderTargets, sizeof(m_currentRenderTargets));
}

RHIResult D3D12CommandList::Initialize(ID3D12CommandAllocator* allocator) {
    if (!allocator) {
        RHI_LOG_ERROR("Command allocator is null");
        return RHIResult::InvalidParameter;
    }
    
    m_commandAllocator = allocator;
    
    // 创建命令列表
    HRESULT hr = m_device->GetD3D12Device()->CreateCommandList(
        0,                              // nodeMask
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator,
        nullptr,                        // initialPipelineState
        IID_PPV_ARGS(&m_commandList)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to create command list: 0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }
    
    // 设置调试名称
    m_commandList->SetName(L"D3D12CommandList");
    
    // 关闭命令列表（等待 Begin() 时重置）
    m_commandList->Close();
    m_isClosed = true;
    
    return RHIResult::Success;
}
```

### 3. Begin/End/Reset

```cpp
void D3D12CommandList::Begin() {
    if (m_isRecording) {
        RHI_LOG_WARNING("Command list is already recording");
        return;
    }
    
    // 重置命令列表
    HRESULT hr = m_commandList->Reset(m_commandAllocator, nullptr);
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to reset command list: 0x%08X", hr);
        return;
    }
    
    m_isRecording = true;
    m_isClosed = false;
    
    // 重置状态追踪
    m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_currentPipelineState = nullptr;
    ZeroMemory(&m_bound_resources, sizeof(m_bound_resources));
    ZeroMemory(&m_currentRenderTargets, sizeof(m_currentRenderTargets));
}

void D3D12CommandList::End() {
    if (!m_isRecording) {
        RHI_LOG_WARNING("Command list is not recording");
        return;
    }
    
    // 关闭命令列表
    HRESULT hr = m_commandList->Close();
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to close command list: 0x%08X", hr);
        return;
    }
    
    m_isRecording = false;
    m_isClosed = true;
}

void D3D12CommandList::Reset() {
    // 重置到初始状态
    if (m_isRecording) {
        End();
    }
    
    // 关闭命令列表（如果还没关闭）
    if (!m_isClosed) {
        m_commandList->Close();
        m_isClosed = true;
    }
}
```

### 4. 状态设置

```cpp
void D3D12CommandList::SetViewport(const Viewport& viewport) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    D3D12_VIEWPORT vp;
    vp.TopLeftX = viewport.topLeftX;
    vp.TopLeftY = viewport.topLeftY;
    vp.Width = viewport.width;
    vp.Height = viewport.height;
    vp.MinDepth = viewport.minDepth;
    vp.MaxDepth = viewport.maxDepth;
    
    m_commandList->RSSetViewports(1, &vp);
}

void D3D12CommandList::SetScissorRect(const Rect2D& rect) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    D3D12_RECT scissorRect;
    scissorRect.left = rect.left;
    scissorRect.top = rect.top;
    scissorRect.right = rect.right;
    scissorRect.bottom = rect.bottom;
    
    m_commandList->RSSetScissorRects(1, &scissorRect);
}

void D3D12CommandList::SetPipelineState(IPipelineState* pipeline) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    if (!pipeline) {
        RHI_LOG_ERROR("Pipeline state is null");
        return;
    }
    
    D3D12PipelineState* d3dPipeline = static_cast<D3D12PipelineState*>(pipeline);
    ID3D12PipelineState* pso = d3dPipeline->GetPipelineState();
    
    m_commandList->SetPipelineState(pso);
    m_currentPipelineState = pso;
}

void D3D12CommandList::SetRootSignature(IRootSignature* rootSig) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    if (!rootSig) {
        RHI_LOG_ERROR("Root signature is null");
        return;
    }
    
    D3D12RootSignature* d3dRootSig = static_cast<D3D12RootSignature*>(rootSig);
    ID3D12RootSignature* rootSignature = d3dRootSig->GetRootSignature();
    
    m_commandList->SetGraphicsRootSignature(rootSignature);
}
```

### 5. 资源绑定

```cpp
void D3D12CommandList::BindVertexBuffer(
    uint32_t slot,
    IBuffer* buffer,
    uint64_t offset,
    uint32_t stride
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    if (slot >= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) {
        RHI_LOG_ERROR("Vertex buffer slot %u out of range", slot);
        return;
    }
    
    D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);
    ID3D12Resource* resource = d3dBuffer ? d3dBuffer->GetResource() : nullptr;
    
    m_commandList->IASetVertexBuffers(slot, 1, &resource, &stride, &offset);
    
    // 追踪绑定状态
    m_bound_resources.vertexBuffers[slot] = resource;
    m_bound_resources.vertexBufferStrides[slot] = stride;
    m_bound_resources.vertexBufferOffsets[slot] = static_cast<UINT>(offset);
}

void D3D12CommandList::BindIndexBuffer(
    IBuffer* buffer,
    uint64_t offset,
    Format format
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);
    ID3D12Resource* resource = d3dBuffer ? d3dBuffer->GetResource() : nullptr;
    
    DXGI_FORMAT dxgiFormat = (format == Format::R16_UInt) 
        ? DXGI_FORMAT_R16_UINT 
        : DXGI_FORMAT_R32_UINT;
    
    m_commandList->IASetIndexBuffer(resource, static_cast<UINT>(offset), dxgiFormat);
    
    // 追踪绑定状态
    m_bound_resources.indexBuffer = resource;
    m_bound_resources.indexFormat = dxgiFormat;
    m_bound_resources.indexBufferOffset = offset;
}

void D3D12CommandList::BindConstantBuffer(
    uint32_t slot,
    IBuffer* buffer,
    uint64_t offset,
    uint64_t size
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);
    ID3D12Resource* resource = d3dBuffer ? d3dBuffer->GetResource() : nullptr;
    
    // 创建常量缓冲视图（需要描述符分配器）
    // 这里简化处理，实际需要通过描述符堆绑定
    // m_commandList->SetGraphicsRootConstantBufferView(slot, resource->GetGPUVirtualAddress() + offset);
}
```

### 6. 绘制命令

```cpp
void D3D12CommandList::Draw(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t startVertex,
    uint32_t startInstance
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    // 验证拓扑已设置
    if (m_primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED) {
        RHI_LOG_ERROR("Primitive topology not set");
        return;
    }
    
    m_commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void D3D12CommandList::DrawIndexed(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t startIndex,
    int32_t vertexOffset,
    uint32_t startInstance
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    // 验证索引缓冲已绑定
    if (!m_bound_resources.indexBuffer) {
        RHI_LOG_ERROR("Index buffer not bound");
        return;
    }
    
    m_commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, vertexOffset, startInstance);
}

void D3D12CommandList::DispatchCompute(
    uint32_t groupCountX,
    uint32_t groupCountY,
    uint32_t groupCountZ
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}
```

### 7. 资源屏障

```cpp
void D3D12CommandList::TransitionBarrier(
    IResource* resource,
    ResourceState before,
    ResourceState after
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    if (!resource) {
        RHI_LOG_ERROR("Resource is null");
        return;
    }
    
    D3D12Resource* d3dResource = static_cast<D3D12Resource*>(resource);
    ID3D12Resource* d3dRes = d3dResource->GetResource();
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = d3dRes;
    barrier.Transition.StateBefore = ToD3D12ResourceState(before);
    barrier.Transition.StateAfter = ToD3D12ResourceState(after);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12CommandList::Barriers(std::span<const ResourceBarrier> barriers) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    std::vector<D3D12_RESOURCE_BARRIER> d3dBarriers;
    d3dBarriers.reserve(barriers.size());
    
    for (const auto& barrier : barriers) {
        D3D12_RESOURCE_BARRIER d3dBarrier = {};
        d3dBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        d3dBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        d3dBarrier.Transition.pResource = static_cast<D3D12Resource*>(barrier.resource)->GetResource();
        d3dBarrier.Transition.StateBefore = ToD3D12ResourceState(barrier.before);
        d3dBarrier.Transition.StateAfter = ToD3D12ResourceState(barrier.after);
        d3dBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        
        d3dBarriers.push_back(d3dBarrier);
    }
    
    if (!d3dBarriers.empty()) {
        m_commandList->ResourceBarrier(static_cast<UINT>(d3dBarriers.size()), d3dBarriers.data());
    }
}
```

### 8. 渲染过程

```cpp
void D3D12CommandList::BeginRenderPass(
    std::span<const RenderTargetAttachment> attachments,
    const DepthStencilAttachment* depthStencil
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    // 设置渲染目标
    UINT numRTs = static_cast<UINT>(std::min(attachments.size(), 
                                              static_cast<size_t>(D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)));
    
    for (UINT i = 0; i < numRTs; i++) {
        const auto& attachment = attachments[i];
        D3D12Texture* d3dTexture = static_cast<D3D12Texture*>(attachment.texture);
        
        // 创建 RTV（如果还没创建）
        if (!attachment.rtvHandle.ptr) {
            RHI_LOG_ERROR("RTV handle not provided");
            continue;
        }
        
        m_currentRenderTargets.rtvs[i] = attachment.rtvHandle;
        
        // 状态转换
        if (attachment.initialState != ResourceState::RenderTarget) {
            TransitionBarrier(attachment.texture, attachment.initialState, ResourceState::RenderTarget);
        }
    }
    
    m_currentRenderTargets.numRenderTargets = numRTs;
    
    // 设置深度模板
    if (depthStencil && depthStencil->texture) {
        D3D12Texture* d3dTexture = static_cast<D3D12Texture*>(depthStencil->texture);
        
        if (!depthStencil->dsvHandle.ptr) {
            RHI_LOG_ERROR("DSV handle not provided");
            return;
        }
        
        m_currentRenderTargets.dsv = depthStencil->dsvHandle;
        m_currentRenderTargets.hasDepthStencil = true;
        
        // 状态转换
        if (depthStencil->initialState != ResourceState::DepthWrite) {
            TransitionBarrier(depthStencil->texture, depthStencil->initialState, ResourceState::DepthWrite);
        }
    } else {
        m_currentRenderTargets.hasDepthStencil = false;
    }
    
    // 绑定渲染目标
    m_commandList->OMSetRenderTargets(numRTs, m_currentRenderTargets.rtvs, 
                                       FALSE, 
                                       m_currentRenderTargets.hasDepthStencil ? &m_currentRenderTargets.dsv : nullptr);
}

void D3D12CommandList::EndRenderPass() {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    // 重置渲染目标状态
    ZeroMemory(&m_currentRenderTargets, sizeof(m_currentRenderTargets));
}
```

### 9. 复制命令

```cpp
void D3D12CommandList::CopyBuffer(
    IBuffer* dest,
    uint64_t destOffset,
    IBuffer* src,
    uint64_t srcOffset,
    uint64_t size
) {
    if (!m_isRecording) {
        RHI_LOG_ERROR("Command list is not recording");
        return;
    }
    
    if (!dest || !src) {
        RHI_LOG_ERROR("Source or destination buffer is null");
        return;
    }
    
    D3D12Buffer* d3dDest = static_cast<D3D12Buffer*>(dest);
    D3D12Buffer* d3dSrc = static_cast<D3D12Buffer*>(src);
    
    m_commandList->CopyBufferRegion(
        d3dDest->GetResource(),
        destOffset,
        d3dSrc->GetResource(),
        srcOffset,
        size
    );
}
```

### 10. 调试标记

```cpp
void D3D12CommandList::SetMarker(const char* name) {
    if (!m_isRecording) {
        return;
    }
    
    // 转换为宽字符串
    wchar_t wideName[256];
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wideName, 256);
    
    m_commandList->SetMarker(0, wideName, static_cast<UINT>(wcslen(wideName) * 2 + 2));
}

void D3D12CommandList::BeginEvent(const char* name) {
    if (!m_isRecording) {
        return;
    }
    
    wchar_t wideName[256];
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wideName, 256);
    
    m_commandList->BeginEvent(0, wideName, static_cast<UINT>(wcslen(wideName) * 2 + 2));
}

void D3D12CommandList::EndEvent() {
    if (!m_isRecording) {
        return;
    }
    
    m_commandList->EndEvent();
}
```

---

## ✅ 验收标准

### 功能测试

- [ ] Begin/End/Reset 正常
- [ ] 视口和裁剪矩形设置
- [ ] 管线状态绑定
- [ ] 顶点/索引缓冲绑定
- [ ] Draw/DrawIndexed 绘制
- [ ] 资源屏障转换
- [ ] RenderPass 开始/结束
- [ ] 复制命令
- [ ] 调试标记

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

### Test 1: 命令列表生命周期

```cpp
TEST(D3D12CommandList, Lifecycle) {
    D3D12Device device;
    device.Initialize({});
    
    // 创建命令分配器
    auto allocator = device.CreateCommandAllocator();
    
    D3D12CommandList cmdList(&device);
    cmdList.Initialize(allocator.Get());
    
    // Begin/End 循环
    for (int i = 0; i < 3; i++) {
        cmdList.Begin();
        EXPECT_TRUE(cmdList.IsRecording());
        
        // 模拟命令录制...
        
        cmdList.End();
        EXPECT_TRUE(cmdList.IsClosed());
    }
}
```

### Test 2: 视口设置

```cpp
TEST(D3D12CommandList, SetViewport) {
    D3D12Device device;
    device.Initialize({});
    
    auto allocator = device.CreateCommandAllocator();
    D3D12CommandList cmdList(&device);
    cmdList.Initialize(allocator.Get());
    
    cmdList.Begin();
    
    Viewport viewport = {0, 0, 1920, 1080, 0.0f, 1.0f};
    cmdList.SetViewport(viewport);
    
    // 验证（需要访问内部状态）
    
    cmdList.End();
}
```

### Test 3: 资源绑定

```cpp
TEST(D3D12CommandList, ResourceBinding) {
    D3D12Device device;
    device.Initialize({});
    
    auto allocator = device.CreateCommandAllocator();
    D3D12CommandList cmdList(&device);
    cmdList.Initialize(allocator.Get());
    
    // 创建测试缓冲区
    auto vertexBuffer = device.CreateBuffer({
        .size = 1024,
        .usage = BufferUsage::Vertex,
        .cpuAccessible = true
    });
    
    cmdList.Begin();
    
    // 绑定顶点缓冲
    cmdList.BindVertexBuffer(0, vertexBuffer.get(), 0, sizeof(float) * 4);
    
    // 验证绑定状态
    
    cmdList.End();
}
```

### Test 4: 资源屏障

```cpp
TEST(D3D12CommandList, ResourceBarrier) {
    D3D12Device device;
    device.Initialize({});
    
    auto allocator = device.CreateCommandAllocator();
    D3D12CommandList cmdList(&device);
    cmdList.Initialize(allocator.Get());
    
    // 创建测试纹理
    auto texture = device.CreateTexture({
        .width = 1024,
        .height = 1024,
        .format = Format::R8G8B8A8_UNorm,
        .usage = TextureUsage::ShaderResource
    });
    
    cmdList.Begin();
    
    // 状态转换
    cmdList.TransitionBarrier(texture.get(), 
                              ResourceState::Common, 
                              ResourceState::ShaderResource);
    
    cmdList.End();
}
```

### Test 5: 调试标记

```cpp
TEST(D3D12CommandList, DebugMarkers) {
    D3D12Device device;
    device.Initialize({});
    
    auto allocator = device.CreateCommandAllocator();
    D3D12CommandList cmdList(&device);
    cmdList.Initialize(allocator.Get());
    
    cmdList.Begin();
    
    // 添加调试标记
    cmdList.BeginEvent("RenderPass");
    cmdList.SetMarker("DrawGeometry");
    cmdList.EndEvent();
    
    cmdList.End();
}
```

---

## 📚 参考资料

- [DX12 命令列表](https://docs.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)
- [资源屏障](https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-barriers)
- [渲染过程](https://docs.microsoft.com/en-us/windows/win32/direct3d12/render-passes)
- UE5 D3D12 命令列表实现

---

*Task 006 - 龙景 2026-03-17*
