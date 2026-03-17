# Phase 1 Task 005: D3D12UploadManager 实现

> 优先级：P0  
> 预计时间：4-6 小时  
> 状态：待开始

---

## 📋 任务描述

实现 `D3D12UploadManager` 类，完成上传堆内存的块状管理、分配和 GPU 完成后重用。

---

## 🎯 目标

1. 实现上传堆块状管理（64MB 默认块）
2. 实现内存分配和偏移追踪
3. 实现 GPU 完成后块重用
4. 实现数据上传辅助函数
5. 编写单元测试

---

## 📁 文件清单

### 需要创建的文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `D3D12UploadManager.h` | 头文件 | ⬜ 待创建 |
| `D3D12UploadManager.cpp` | 源文件 | ⬜ 待创建 |
| `D3D12UploadManagerTest.cpp` | 测试 | ⬜ 待创建 |

### 依赖文件

- `IUploadManager.h` - 上传管理器接口
- `D3D12Device.h` - D3D12 设备
- `D3D12FrameResourceManager.h` - 帧资源管理

---

## 🔧 实现要点

### 1. 上传块结构

```cpp
struct UploadBlock {
    ComPtr<ID3D12Resource> buffer;      // 上传缓冲资源
    void* mappedPtr = nullptr;          // 映射的 CPU 可见内存
    size_t capacity = 0;                // 块容量（字节）
    size_t used = 0;                    // 已使用量
    uint64_t fenceValue = 0;            // 可重用的围栏值
    bool isFull = false;                // 是否已满
    
    // 重置块
    void Reset() {
        used = 0;
        isFull = false;
        fenceValue = 0;
    }
};
```

### 2. 上传分配结构

```cpp
struct UploadAllocation {
    ID3D12Resource* buffer;     // 上传缓冲区
    void* mappedData;           // 映射的 CPU 内存
    uint64_t gpuAddress;        // GPU 虚拟地址
    size_t offset;              // 在块中的偏移
    size_t size;                // 分配大小
    uint32_t blockIndex;        // 块索引
    
    bool IsValid() const { return buffer != nullptr; }
};
```

### 3. 管理器类结构

```cpp
class D3D12UploadManager : public IUploadManager {
private:
    D3D12Device* m_device;
    UploadManagerConfig m_config;
    
    std::vector<UploadBlock> m_blocks;
    UploadBlock* m_currentBlock;
    
    // 待清理的块（等待 GPU 完成）
    struct PendingCleanup {
        UploadBlock* block;
        uint64_t fenceValue;
    };
    std::vector<PendingCleanup> m_pendingCleanups;
    
    // 创建新块
    UploadBlock* CreateNewBlock(size_t minSize);
    
public:
    D3D12UploadManager(D3D12Device* device);
    virtual ~D3D12UploadManager() override;
    
    // IUploadManager 接口
    virtual RHIResult Initialize(const UploadManagerConfig& config) override;
    virtual UploadAllocation Allocate(size_t size, size_t alignment = 256) override;
    virtual void SubmitUpload(ICommandList* cmdList) override;
    virtual void CleanupCompletedUploads(uint64_t completedFenceValue) override;
    virtual void Reset() override;
};
```

### 4. 初始化

```cpp
RHIResult D3D12UploadManager::Initialize(const UploadManagerConfig& config) {
    m_config = config;
    
    // 预分配 1-2 个块
    for (int i = 0; i < 2; i++) {
        UploadBlock* block = CreateNewBlock(m_config.defaultBlockSize);
        if (!block) {
            return RHIResult::OutOfMemory;
        }
    }
    
    m_currentBlock = &m_blocks[0];
    
    return RHIResult::Success;
}

UploadBlock* D3D12UploadManager::CreateNewBlock(size_t minSize) {
    // 计算块大小
    size_t blockSize = std::max(m_config.defaultBlockSize, minSize);
    blockSize = std::min(blockSize, m_config.maxBlockSize);
    
    // 创建上传堆资源
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc(
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,                          // alignment
        blockSize,                  // width
        1, 1, 1,                    // height, depth, mipLevels
        DXGI_FORMAT_UNKNOWN,
        1, 0,                       // sampleCount, sampleQuality
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    );
    
    ComPtr<ID3D12Resource> resource;
    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to create upload buffer: 0x%08X", hr);
        return nullptr;
    }
    
    // 映射内存
    void* mappedPtr = nullptr;
    hr = resource->Map(0, nullptr, &mappedPtr);
    if (FAILED(hr)) {
        RHI_LOG_ERROR("Failed to map upload buffer: 0x%08X", hr);
        return nullptr;
    }
    
    // 创建块
    UploadBlock block;
    block.buffer = resource;
    block.mappedPtr = mappedPtr;
    block.capacity = blockSize;
    block.used = 0;
    block.fenceValue = 0;
    block.isFull = false;
    
    m_blocks.push_back(block);
    return &m_blocks.back();
}
```

### 5. 内存分配

```cpp
UploadAllocation D3D12UploadManager::Allocate(size_t size, size_t alignment) {
    // 对齐大小
    size = (size + alignment - 1) & ~(alignment - 1);
    
    // 查找或创建合适的块
    if (!m_currentBlock || m_currentBlock->used + size > m_currentBlock->capacity) {
        m_currentBlock = CreateNewBlock(size);
        if (!m_currentBlock) {
            // 分配失败
            return UploadAllocation{};
        }
    }
    
    // 在当前块中分配
    UploadAllocation alloc;
    alloc.buffer = m_currentBlock->buffer.Get();
    alloc.mappedData = static_cast<byte*>(m_currentBlock->mappedPtr) + m_currentBlock->used;
    alloc.gpuAddress = m_currentBlock->buffer->GetGPUVirtualAddress() + m_currentBlock->used;
    alloc.offset = m_currentBlock->used;
    alloc.size = size;
    alloc.blockIndex = static_cast<uint32_t>(m_blocks.size() - 1);
    
    // 更新块使用量
    m_currentBlock->used += size;
    
    // 检查块是否已满（95% 阈值）
    if (m_currentBlock->used >= m_currentBlock->capacity * 0.95f) {
        m_currentBlock->isFull = true;
        m_currentBlock = nullptr;  // 下次分配创建新块
    }
    
    return alloc;
}
```

### 6. 围栏值注册

```cpp
void D3D12UploadManager::RegisterFenceValue(UploadAllocation& alloc, uint64_t fenceValue) {
    // 找到对应的块并设置围栏值
    if (alloc.blockIndex < m_blocks.size()) {
        UploadBlock& block = m_blocks[alloc.blockIndex];
        block.fenceValue = fenceValue;
        
        // 添加到待清理列表
        m_pendingCleanups.push_back({&block, fenceValue});
    }
}
```

### 7. 清理已完成的上传

```cpp
void D3D12UploadManager::CleanupCompletedUploads(uint64_t completedFenceValue) {
    // 清理已完成的块
    for (auto it = m_pendingCleanups.begin(); it != m_pendingCleanups.end(); ) {
        if (it->fenceValue <= completedFenceValue) {
            // GPU 已完成，重置块以供重用
            it->block->Reset();
            it = m_pendingCleanups.erase(it);
        } else {
            ++it;
        }
    }
    
    // 更新当前块
    if (m_currentBlock && m_currentBlock->fenceValue <= completedFenceValue) {
        m_currentBlock->Reset();
    }
}
```

### 8. 数据上传辅助函数

```cpp
RHIResult D3D12UploadManager::UploadBufferData(
    ICommandList* cmdList,
    ID3D12Resource* destBuffer,      // 目标（默认堆）
    uint64_t destOffset,
    const void* data,
    size_t dataSize
) {
    // 1. 分配上传内存
    UploadAllocation alloc = Allocate(dataSize);
    if (!alloc.IsValid()) {
        return RHIResult::OutOfMemory;
    }
    
    // 2. 复制数据到上传内存
    memcpy(alloc.mappedData, data, dataSize);
    
    // 3. 录制复制命令
    cmdList->CopyBufferRegion(destBuffer, destOffset, alloc.buffer, alloc.offset, dataSize);
    
    // 4. 添加资源屏障
    cmdList->TransitionBarrier(
        destBuffer,
        ResourceState::CopyDest,
        ResourceState::VertexBuffer  // 或其他目标状态
    );
    
    return RHIResult::Success;
}

RHIResult D3D12UploadManager::UploadTextureData(
    ICommandList* cmdList,
    ID3D12Resource* destTexture,     // 目标纹理（默认堆）
    const void* data,
    size_t dataSize,
    uint32_t width,
    uint32_t height,
    uint32_t rowPitch,
    uint32_t arraySlice = 0,
    uint32_t mipLevel = 0
) {
    // 1. 分配上传内存
    UploadAllocation alloc = Allocate(dataSize);
    if (!alloc.IsValid()) {
        return RHIResult::OutOfMemory;
    }
    
    // 2. 复制数据（考虑行间距）
    CopyTextureData(
        alloc.mappedData, rowPitch,
        data, rowPitch,
        width, height,
        GetFormatBytesPerPixel(m_format)
    );
    
    // 3. 录制复制命令
    D3D12_TEXTURE_COPY_LOCATION destLoc = {};
    destLoc.pResource = destTexture;
    destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destLoc.SubresourceIndex = D3D12CalcSubresource(mipLevel, arraySlice, 0, 1, false);
    
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = alloc.buffer;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint.Offset = alloc.offset;
    srcLoc.PlacedFootprint.Footprint.Format = m_dxgiFormat;
    srcLoc.PlacedFootprint.Footprint.Width = width;
    srcLoc.PlacedFootprint.Footprint.Height = height;
    srcLoc.PlacedFootprint.Footprint.Depth = 1;
    srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;
    
    cmdList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
    
    return RHIResult::Success;
}
```

### 9. Reset

```cpp
void D3D12UploadManager::Reset() {
    // 标记所有块为待清理（实际清理在 CleanupCompletedUploads）
    // 不需要额外操作
}
```

---

## ✅ 验收标准

### 功能测试

- [ ] 上传块创建成功
- [ ] 内存分配正确（对齐）
- [ ] 块满后创建新块
- [ ] GPU 完成后块重用
- [ ] 数据上传正确

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
TEST(D3D12UploadManager, Initialize) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12UploadManager manager(&device);
    
    UploadManagerConfig config = {};
    config.defaultBlockSize = 64 * 1024 * 1024;  // 64MB
    config.maxBlockSize = 256 * 1024 * 1024;     // 256MB
    
    RHIResult result = manager.Initialize(config);
    
    ASSERT_EQ(result, RHIResult::Success);
}
```

### Test 2: 内存分配

```cpp
TEST(D3D12UploadManager, Allocate) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12UploadManager manager(&device);
    manager.Initialize({});
    
    // 分配小内存
    UploadAllocation alloc1 = manager.Allocate(1024);
    EXPECT_TRUE(alloc1.IsValid());
    EXPECT_NE(alloc1.mappedData, nullptr);
    EXPECT_GT(alloc1.gpuAddress, 0);
    
    // 分配大内存
    UploadAllocation alloc2 = manager.Allocate(1024 * 1024);  // 1MB
    EXPECT_TRUE(alloc2.IsValid());
    
    // 验证偏移
    EXPECT_GT(alloc2.offset, alloc1.offset);
}
```

### Test 3: 对齐

```cpp
TEST(D3D12UploadManager, Alignment) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12UploadManager manager(&device);
    manager.Initialize({});
    
    // 分配非对齐大小
    UploadAllocation alloc = manager.Allocate(100, 256);
    
    // 验证对齐
    EXPECT_EQ(alloc.offset % 256, 0);
    EXPECT_EQ(alloc.size % 256, 0);  // 应该向上对齐到 256
}
```

### Test 4: 块重用

```cpp
TEST(D3D12UploadManager, BlockReuse) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12UploadManager manager(&device);
    manager.Initialize({});
    
    // 分配并注册围栏值
    UploadAllocation alloc = manager.Allocate(1024);
    manager.RegisterFenceValue(alloc, 1);
    
    // 模拟 GPU 完成
    manager.CleanupCompletedUploads(1);
    
    // 块应该被重置，可以重用
    // （具体验证需要访问内部状态）
}
```

### Test 5: 数据上传

```cpp
TEST(D3D12UploadManager, UploadBufferData) {
    D3D12Device device;
    device.Initialize({});
    
    D3D12UploadManager manager(&device);
    manager.Initialize({});
    
    // 创建目标缓冲区
    auto destBuffer = device.CreateBuffer({
        .size = 1024,
        .usage = BufferUsage::Vertex,
        .cpuAccessible = false
    });
    
    // 测试数据
    float testData[] = {1.0f, 2.0f, 3.0f, 4.0f};
    
    // 创建命令列表
    auto cmdList = device.CreateCommandList();
    cmdList->Begin();
    
    // 上传数据
    RHIResult result = manager.UploadBufferData(
        cmdList.get(),
        destBuffer->GetNativeResource(),
        0,
        testData,
        sizeof(testData)
    );
    
    ASSERT_EQ(result, RHIResult::Success);
    
    cmdList->End();
    device.SubmitCommandLists({cmdList.get()});
}
```

---

## 📚 参考资料

- [DX12 上传堆最佳实践](https://docs.microsoft.com/en-us/windows/win32/direct3d12/uploading-data-to-buffers)
- [块状内存管理](https://en.wikipedia.org/wiki/Memory_pool)
- UE5 D3D12 上传管理器实现

---

*Task 005 - 龙景 2026-03-17*
