# Phase 1 Task 003: D3D12Texture 实现

> 优先级：P0  
> 预计时间：4-6 小时  
> 状态：待开始

---

## 📋 任务描述

实现 `D3D12Texture` 类，完成 D3D12 纹理资源的创建、视图创建和数据上传。

---

## 🎯 目标

1. 实现 D3D12 纹理创建（2D/2D 数组/CubeMap）
2. 实现资源视图创建（SRV/RTV/DSV/UAV）
3. 实现 Mip 链支持
4. 实现纹理数据上传
5. 编写单元测试

---

## 📁 文件清单

### 需要创建的文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `D3D12Texture.h` | 头文件 | ⬜ 待创建 |
| `D3D12Texture.cpp` | 源文件 | ⬜ 待创建 |
| `D3D12TextureTest.cpp` | 测试 | ⬜ 待创建 |

### 依赖文件

- `ITexture.h` - 纹理接口
- `D3D12Device.h` - D3D12 设备
- `D3D12Buffer.h` - 缓冲区（用于上传）

---

## 🔧 实现要点

### 1. 纹理描述

```cpp
struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    Format format = Format::Unknown;
    TextureUsage usage = TextureUsage::None;
    ResourceState initialState = ResourceState::Common;
    bool cpuAccessible = false;
};
```

### 2. 创建流程

```cpp
RHIResult D3D12Texture::Initialize(D3D12Device* device, const TextureDesc& desc) {
    // 1. 转换格式
    DXGI_FORMAT dxgiFormat = ToDXGIFormat(desc.format);
    
    // 2. 创建资源
    CD3DX12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width = desc.width;
    resourceDesc.Height = desc.height;
    resourceDesc.DepthOrArraySize = desc.depth * desc.arraySize;
    resourceDesc.MipLevels = desc.mipLevels;
    resourceDesc.Format = dxgiFormat;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Flags = ToD3D12Flags(desc.usage);
    
    CD3DX12_HEAP_PROPERTIES heapProps(
        desc.cpuAccessible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT
    );
    
    device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );
    
    // 3. 保存信息
    m_desc = desc;
    m_dxgiFormat = dxgiFormat;
    
    return RHIResult::Success;
}
```

### 3. 视图创建

```cpp
// SRV - 着色器资源视图
RHIResult D3D12Texture::CreateSRV(D3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = m_dxgiFormat;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = m_desc.mipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    
    device->GetD3D12Device()->CreateShaderResourceView(
        m_resource.Get(), &srvDesc, handle
    );
    
    return RHIResult::Success;
}

// RTV - 渲染目标视图
RHIResult D3D12Texture::CreateRTV(D3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = m_dxgiFormat;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    
    device->GetD3D12Device()->CreateRenderTargetView(
        m_resource.Get(), &rtvDesc, handle
    );
    
    return RHIResult::Success;
}

// DSV - 深度模板视图
RHIResult D3D12Texture::CreateDSV(D3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = m_dxgiFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = 0;
    
    device->GetD3D12Device()->CreateDepthStencilView(
        m_resource.Get(), &dsvDesc, handle
    );
    
    return RHIResult::Success;
}
```

### 4. 纹理数据上传

```cpp
RHIResult D3D12Texture::UpdateData(
    D3D12Device* device,
    ICommandList* cmdList,
    const void* data,
    size_t dataSize,
    uint32_t rowPitch,
    uint32_t arraySlice = 0,
    uint32_t mipLevel = 0
) {
    if (!m_cpuAccessible) {
        // 默认堆需要通过上传堆复制
        return UploadViaStagingBuffer(device, cmdList, data, dataSize, rowPitch);
    }
    
    // 上传堆直接写入
    D3D12_MAPPED_SUBRESOURCE mapped;
    m_resource->Map(0, nullptr, &mapped.pData);
    
    // 复制数据（考虑行间距）
    CopyTextureData(
        mapped.pData, mapped.RowPitch,
        data, rowPitch,
        m_desc.width, m_desc.height,
        GetFormatBytesPerPixel(m_desc.format)
    );
    
    m_resource->Unmap(0, nullptr);
    return RHIResult::Success;
}
```

---

## ✅ 验收标准

### 功能测试

- [ ] 2D 纹理创建成功
- [ ] 纹理数组创建成功
- [ ] SRV 创建成功
- [ ] RTV 创建成功
- [ ] DSV 创建成功
- [ ] 纹理数据上传正确
- [ ] Mip 链支持正确

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

### Test 1: 2D 纹理创建

```cpp
TEST(D3D12Texture, Create2DTexture) {
    D3D12Device device;
    device.Initialize({});
    
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    D3D12Texture texture;
    RHIResult result = texture.Initialize(&device, desc);
    
    ASSERT_EQ(result, RHIResult::Success);
    EXPECT_EQ(texture.GetWidth(), 1024);
    EXPECT_EQ(texture.GetHeight(), 1024);
    EXPECT_NE(texture.GetGPUVirtualAddress(), 0);
}
```

### Test 2: 渲染目标纹理

```cpp
TEST(D3D12Texture, RenderTarget) {
    D3D12Device device;
    device.Initialize({});
    
    TextureDesc desc = {};
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::RenderTarget;
    
    D3D12Texture texture;
    texture.Initialize(&device, desc);
    
    // 创建 RTV 描述符堆
    auto rtvHeap = device.CreateDescriptorHeap({DescriptorHeapType::RTV, 1});
    
    // 创建 RTV
    RHIResult result = texture.CreateRTV(&device, rtvHeap->GetCPUDescriptorHandle(0));
    ASSERT_EQ(result, RHIResult::Success);
}
```

### Test 3: 深度纹理

```cpp
TEST(D3D12Texture, DepthStencil) {
    D3D12Device device;
    device.Initialize({});
    
    TextureDesc desc = {};
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::D32_Float;
    desc.usage = TextureUsage::DepthStencil;
    
    D3D12Texture texture;
    texture.Initialize(&device, desc);
    
    // 创建 DSV
    auto dsvHeap = device.CreateDescriptorHeap({DescriptorHeapType::DSV, 1});
    RHIResult result = texture.CreateDSV(&device, dsvHeap->GetCPUDescriptorHandle(0));
    ASSERT_EQ(result, RHIResult::Success);
}
```

### Test 4: 纹理数据上传

```cpp
TEST(D3D12Texture, UpdateData) {
    D3D12Device device;
    device.Initialize({});
    
    TextureDesc desc = {};
    desc.width = 256;
    desc.height = 256;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.cpuAccessible = true;
    
    D3D12Texture texture;
    texture.Initialize(&device, desc);
    
    // 创建测试数据（红色渐变）
    std::vector<uint8_t> data(256 * 256 * 4);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            data[(y * 256 + x) * 4 + 0] = 255;  // R
            data[(y * 256 + x) * 4 + 1] = 0;    // G
            data[(y * 256 + x) * 4 + 2] = 0;    // B
            data[(y * 256 + x) * 4 + 3] = 255;  // A
        }
    }
    
    // 上传数据
    RHIResult result = texture.UpdateData(&device, nullptr, data.data(), data.size(), 256 * 4);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 验证数据（Map 读取）
    void* mapped = texture.Map(0, 0);
    EXPECT_EQ(memcmp(mapped, data.data(), data.size()), 0);
}
```

### Test 5: Mip 链纹理

```cpp
TEST(D3D12Texture, MipChain) {
    D3D12Device device;
    device.Initialize({});
    
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.mipLevels = 11;  // 1024 的完整 Mip 链
    desc.format = Format::R8G8B8A8_UNorm;
    
    D3D12Texture texture;
    texture.Initialize(&device, desc);
    
    EXPECT_EQ(texture.GetDesc().mipLevels, 11);
    EXPECT_EQ(texture.GetWidth(0), 1024);
    EXPECT_EQ(texture.GetWidth(1), 512);
    EXPECT_EQ(texture.GetWidth(10), 1);
}
```

---

## 📚 参考资料

- [DX12 纹理创建](https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-textures)
- [资源视图创建](https://docs.microsoft.com/en-us/windows/win32/direct3d12/resource-views)
- [Mip 链计算](https://docs.microsoft.com/en-us/windows/win32/direct3d9/mipmap-level-calculation)
- UE5 D3D12 纹理实现

---

*Task 003 - 龙景 2026-03-17*
