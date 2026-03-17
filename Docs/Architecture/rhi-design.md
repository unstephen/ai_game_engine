# RHI 架构设计文档

> 版本：v0.2  
> 日期：2026-03-17  
> 状态：设计中

---

## 1. 概述

RHI（Rendering Hardware Interface）是引擎的渲染硬件抽象层，提供跨图形 API 的统一接口。

### 1.1 设计目标

1. **跨平台抽象** - 统一接口支持 D3D12/Vulkan/Metal
2. **高性能** - 最小化抽象层开销，接近原生 API 性能
3. **显式控制** - 资源状态、屏障、同步由调用者显式管理
4. **现代 C++** - RAII、智能指针、类型安全

### 1.2 非目标

- 多线程命令录制（Phase 2）
- Render Graph 自动管理（Phase 3）
- 光线追踪支持（Phase 4）

---

## 2. 架构层次

```
┌─────────────────────────────────────────────────────────────┐
│                     Renderer Layer                          │
│                  (Forward+/Deferred)                        │
├─────────────────────────────────────────────────────────────┤
│                      RHI Layer                              │
│  ┌─────────────┬─────────────┬─────────────────────────┐   │
│  │  Public API │  Internals  │      D3D12 Backend      │   │
│  │  (接口定义)  │  (资源管理)  │  (ID3D12Device 等封装)   │   │
│  └─────────────┴─────────────┴─────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Graphics API                             │
│         D3D12        │       Vulkan       │     Metal       │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. 核心模块

### 3.1 模块依赖图

```
                    RHI.h (主头文件)
                        │
        ┌───────────────┼───────────────┐
        │               │               │
    IDevice.h    ICommandList.h    IResource.h
        │               │               │
        ├───────────────┤               │
        │               │               │
  IBuffer.h       IPipelineState.h  ISwapChain.h
  ITexture.h      IRootSignature.h  IFence.h
  IShader.h
```

### 3.2 类关系图

```
┌────────────────────────────────────────────────────────────┐
│                         IDevice                            │
│  - CreateBuffer()                                          │
│  - CreateTexture()                                         │
│  - CreateCommandList()                                     │
│  - SubmitCommandLists()                                    │
└────────────────────────────────────────────────────────────┘
                            │
            ┌───────────────┼───────────────┐
            │               │               │
            ▼               ▼               ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │  IBuffer     │ │  ITexture    │ │ ICommandList │
    └──────────────┘ └──────────────┘ └──────────────┘
                            │
                            ▼
                    ┌──────────────┐
                    │IPipelineState│
                    └──────────────┘
```

---

## 4. 资源管理

### 4.1 资源生命周期

```
创建 → 初始化 → 使用 → 状态转换 → 销毁
  │        │        │        │         │
  │        │        │        │         └─→ unique_ptr 自动释放
  │        │        │        │
  │        │        │        └─→ TransitionBarrier
  │        │        │
  │        │        └─→ 绑定到命令列表
  │        │
  │        └─→ 上传数据（UploadManager）
  │
  └─→ CreateBuffer/CreateTexture
```

### 4.2 帧资源管理（三缓冲）

```cpp
FrameContext frames[3];
uint32_t currentFrame = 0;

void BeginFrame() {
    FrameContext& ctx = frames[currentFrame];
    
    // 1. 等待 GPU 完成该帧
    WaitForFence(ctx.fence, ctx.fenceValue);
    
    // 2. 重置命令分配器
    ctx.commandAllocator->Reset();
    
    // 3. 重置描述符分配器
    ctx.descriptorAllocator->Reset();
    
    // 4. 清理临时资源
    ctx.uploadBuffers.clear();
    ctx.pendingDeletions.clear();
}

void EndFrame() {
    FrameContext& ctx = frames[currentFrame];
    
    // 1. 信号围栏
    SignalFence(ctx.fence, ++ctx.fenceValue);
    
    // 2. 切换到下一帧
    currentFrame = (currentFrame + 1) % 3;
}
```

### 4.3 上传管理器

```cpp
// 上传流程
UploadAllocation alloc = uploadManager->Allocate(dataSize);
memcpy(alloc.mappedData, vertexData, dataSize);

cmdList->CopyBuffer(vertexBuffer, 0, 
                    alloc.buffer, alloc.offset, 
                    dataSize);

// GPU 完成后重用
uploadManager->CleanupCompletedUploads(completedFenceValue);
```

---

## 5. 描述符管理

### 5.1 描述符堆类型

| 类型 | Shader Visible | 分配策略 | 重置频率 |
|------|---------------|----------|----------|
| CBV/SRV/UAV | ✅ | 环形分配 | 每帧 |
| Sampler | ✅ | 环形分配 | 每帧 |
| RTV | ❌ | 线性分配 | 永不 |
| DSV | ❌ | 线性分配 | 永不 |

### 5.2 分配器设计

```cpp
class DescriptorAllocator {
    // Shader Visible 堆（每帧重置）
    RingDescriptorAllocator cbvSrvUavHeap;
    RingDescriptorAllocator samplerHeap;
    
    // CPU-only 堆（线性分配）
    LinearDescriptorAllocator rtvHeap;
    LinearDescriptorAllocator dsvHeap;
    
    DescriptorHandle Allocate(DescriptorType type) {
        switch (type) {
        case CBV:
        case SRV:
        case UAV:
            return cbvSrvUavHeap.Allocate();
        case Sampler:
            return samplerHeap.Allocate();
        case RTV:
            return rtvHeap.Allocate();
        case DSV:
            return dsvHeap.Allocate();
        }
    }
    
    void Reset() {
        cbvSrvUavHeap.Reset();  // 每帧重置
        samplerHeap.Reset();
        // RTV/DSV 不重置
    }
};
```

---

## 6. 错误处理

### 6.1 错误码策略

```cpp
RHIResult result = device->CreateBuffer(desc, buffer);

if (IsFailure(result)) {
    if (IsDeviceLost(result)) {
        // 设备丢失，需要恢复或重启
        HandleDeviceLost();
    } else {
        // 其他错误，记录日志
        RHI_LOG_ERROR("CreateBuffer failed: %s", GetErrorName(result));
    }
}
```

### 6.2 设备丢失处理

```cpp
void SetDeviceLostCallback(DeviceLostCallback callback) {
    deviceLostCallback = callback;
}

void OnDeviceLost(DeviceLostReason reason) {
    if (deviceLostCallback) {
        deviceLostCallback(reason, "Device lost");
    }
    
    // 进入错误状态
    isDeviceLost = true;
    deviceLostReason = reason;
}
```

---

## 7. 调试支持

### 7.1 资源命名

```cpp
buffer->SetDebugName("VertexBuffers_Character");
texture->SetDebugName("Textures_Diffuse_Albedo");
```

### 7.2 状态跟踪（调试模式）

```cpp
#ifdef RHI_DEBUG
class ResourceStateTracker {
    std::unordered_map<IResource*, ResourceState> states;
    
    void ValidateTransition(IResource* resource, 
                           ResourceState before, 
                           ResourceState after) {
        if (states[resource] != before) {
            RHI_LOG_ERROR("Invalid state transition");
        }
        states[resource] = after;
    }
};
#endif
```

### 7.3 PIX/RenderDoc 集成

```cpp
// PIX 事件标记
cmdList->BeginEvent("RenderScene");
RenderScene(cmdList);
cmdList->EndEvent();

// 资源命名
texture->SetDebugName("GBuffer_Albedo");
```

---

## 8. 性能考虑

### 8.1 批处理

```cpp
// 批量屏障
std::vector<ResourceBarrier> barriers;
barriers.reserve(8);

for (auto& transition : transitions) {
    barriers.push_back(CreateBarrier(transition));
}

if (!barriers.empty()) {
    cmdList->Barriers(barriers);  // 一次提交
}
```

### 8.2 描述符复用

```cpp
// 持久描述符（不每帧创建）
DescriptorHandle rtvHandle = descriptorAllocator->AllocateRTV(texture);
// 整个生命周期只分配一次

// 临时描述符（每帧重置）
DescriptorHandle srvHandle = descriptorAllocator->AllocateSRV(texture);
// 每帧重新分配
```

### 8.3 上传批处理

```cpp
// 合并多个小上传
UploadAllocation alloc1 = uploadManager->Allocate(1024);
UploadAllocation alloc2 = uploadManager->Allocate(2048);
UploadAllocation alloc3 = uploadManager->Allocate(512);

// 一次性提交
uploadManager->SubmitUpload(cmdList);
```

---

## 9. 实施检查清单

### Phase 1 (2 周)

- [ ] CMake 项目搭建
- [ ] IDevice 接口定义
- [ ] D3D12Device 实现
- [ ] ISwapChain 实现
- [ ] ICommandList 实现
- [ ] 帧资源管理器
- [ ] 三角形渲染测试

### Phase 2 (4 周)

- [ ] IDescriptorAllocator 实现
- [ ] IUploadManager 实现
- [ ] IBuffer 完整实现
- [ ] ITexture 完整实现
- [ ] 描述符创建（CBV/SRV/UAV/RTV/DSV）

### Phase 3 (4 周)

- [ ] 深度缓冲支持
- [ ] 混合状态支持
- [ ] 多渲染目标
- [ ] 采样器支持
- [ ] 资源状态跟踪器
- [ ] PIX 性能分析

---

## 10. 参考文档

- [DX12 官方文档](https://docs.microsoft.com/en-us/windows/win32/direct3d12/)
- [Vulkan 规范](https://www.khronos.org/registry/vulkan/specs/)
- [UE5 RHI 源码](https://github.com/EpicGames/UnrealEngine)
- [Godot 4 RD 源码](https://github.com/godotengine/godot)
- [Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine)

---

*RHI 架构设计文档 v0.2 - 龙景 2026-03-17* 🐉
