# Phase 1 Task 002: D3D12Buffer 实现

> 优先级：P0  
> 预计时间：3-4 小时  
> 状态：待开始

---

## 📋 任务描述

实现 `D3D12Buffer` 类，完成 D3D12 缓冲区的创建、映射和数据上传。

---

## 🎯 目标

1. 实现 D3D12 缓冲区创建（顶点/索引/常量/存储缓冲）
2. 实现 CPU 映射（Map/Unmap）
3. 实现上传堆缓冲（CPU 可访问）
4. 实现默认堆缓冲（GPU 优化）
5. 编写单元测试

---

## 📁 文件清单

### 需要创建的文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `D3D12Buffer.h` | 头文件 | ⬜ 待创建 |
| `D3D12Buffer.cpp` | 源文件 | ⬜ 待创建 |
| `D3D12BufferTest.cpp` | 测试 | ⬜ 待创建 |

### 依赖文件

- `IBuffer.h` - 缓冲区接口
- `D3D12Device.h` - D3D12 设备

---

## 🔧 实现要点

### 1. 缓冲区描述

```cpp
struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    ResourceState initialState = ResourceState::Common;
    bool cpuAccessible = false;  // true=上传堆，false=默认堆
};
```

### 2. 创建流程

```cpp
RHIResult D3D12Buffer::Initialize(D3D12Device* device, const BufferDesc& desc) {
    // 1. 确定堆类型
    D3D12_HEAP_TYPE heapType = desc.cpuAccessible 
        ? D3D12_HEAP_TYPE_UPLOAD 
        : D3D12_HEAP_TYPE_DEFAULT;
    
    // 2. 创建资源
    CD3DX12_HEAP_PROPERTIES heapProps(heapType);
    CD3DX12_RESOURCE_DESC resourceDesc(...);
    
    device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );
    
    // 3. 如果是上传堆，映射内存
    if (desc.cpuAccessible) {
        m_resource->Map(0, nullptr, &m_mappedData);
    }
    
    return RHIResult::Success;
}
```

### 3. Map/Unmap

```cpp
void* D3D12Buffer::Map(uint64_t offset, uint64_t size) override {
    if (!m_cpuAccessible) {
        RHI_LOG_ERROR("Cannot map GPU-only buffer");
        return nullptr;
    }
    
    // 上传堆已经映射，直接返回偏移
    return static_cast<byte*>(m_mappedData) + offset;
}

void D3D12Buffer::Unmap() override {
    if (!m_cpuAccessible) {
        return;
    }
    
    // 上传堆通常不 Unmap，保持映射状态
    // m_resource->Unmap(0, nullptr);
}
```

### 4. GPU 虚拟地址

```cpp
uint64_t D3D12Buffer::GetGPUVirtualAddress() const override {
    return m_resource ? m_resource->GetGPUVirtualAddress() : 0;
}
```

---

## ✅ 验收标准

### 功能测试

- [ ] 上传堆缓冲创建成功
- [ ] 默认堆缓冲创建成功
- [ ] Map/Unmap 正常工作
- [ ] GPU 虚拟地址正确
- [ ] 不同用途缓冲创建正确

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

### Test 1: 上传堆缓冲区

```cpp
TEST(D3D12Buffer, UploadHeap) {
    D3D12Device device;
    device.Initialize({});
    
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Vertex;
    desc.cpuAccessible = true;  // 上传堆
    
    D3D12Buffer buffer;
    RHIResult result = buffer.Initialize(&device, desc);
    
    ASSERT_EQ(result, RHIResult::Success);
    EXPECT_NE(buffer.GetGPUVirtualAddress(), 0);
    
    // 测试 Map
    void* data = buffer.Map(0, 1024);
    EXPECT_NE(data, nullptr);
    
    // 写入数据
    float testData[] = {1.0f, 2.0f, 3.0f, 4.0f};
    memcpy(data, testData, sizeof(testData));
    
    // 验证数据
    EXPECT_EQ(memcmp(data, testData, sizeof(testData)), 0);
}
```

### Test 2: 默认堆缓冲区

```cpp
TEST(D3D12Buffer, DefaultHeap) {
    D3D12Device device;
    device.Initialize({});
    
    BufferDesc desc = {};
    desc.size = 4096;
    desc.usage = BufferUsage::Vertex | BufferUsage::Index;
    desc.cpuAccessible = false;  // 默认堆
    
    D3D12Buffer buffer;
    RHIResult result = buffer.Initialize(&device, desc);
    
    ASSERT_EQ(result, RHIResult::Success);
    EXPECT_NE(buffer.GetGPUVirtualAddress(), 0);
    
    // 默认堆不能 Map
    void* data = buffer.Map(0, 4096);
    EXPECT_EQ(data, nullptr);
}
```

### Test 3: 常量缓冲区

```cpp
TEST(D3D12Buffer, ConstantBuffer) {
    D3D12Device device;
    device.Initialize({});
    
    BufferDesc desc = {};
    desc.size = 256;  // 常量缓冲需要 256 字节对齐
    desc.usage = BufferUsage::Constant;
    desc.cpuAccessible = true;
    
    D3D12Buffer buffer;
    RHIResult result = buffer.Initialize(&device, desc);
    
    ASSERT_EQ(result, RHIResult::Success);
    
    // 验证大小对齐
    EXPECT_GE(buffer.GetSize(), 256);
}
```

### Test 4: 错误处理

```cpp
TEST(D3D12Buffer, InvalidSize) {
    D3D12Device device;
    device.Initialize({});
    
    BufferDesc desc = {};
    desc.size = 0;  // 无效大小
    
    D3D12Buffer buffer;
    RHIResult result = buffer.Initialize(&device, desc);
    
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}
```

---

## 📚 参考资料

- [DX12 缓冲区创建](https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-buffers)
- [上传堆 vs 默认堆](https://docs.microsoft.com/en-us/windows/win32/direct3d12/uploading-data-to-buffers)
- UE5 D3D12 缓冲区实现

---

*Task 002 - 龙景 2026-03-17*
