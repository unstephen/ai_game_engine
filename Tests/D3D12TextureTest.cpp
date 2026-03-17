// =============================================================================
// D3D12TextureTest.cpp - D3D12Texture 单元测试
// =============================================================================

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "D3D12Texture.h"
#include "D3D12Device.h"

#include <Windows.h>
#include <d3d12.h>

using namespace Engine::RHI;
using namespace Engine::RHI::D3D12;

// =============================================================================
// 测试固件
// =============================================================================

class D3D12TextureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建设备
        DeviceDesc deviceDesc = {};
        deviceDesc.enableDebug = true;
        deviceDesc.enableValidation = true;
        
        m_device = std::make_unique<D3D12Device>();
        RHIResult result = m_device->Initialize(deviceDesc);
        
        // 如果设备创建失败，跳过测试
        if (IsFailure(result)) {
            GTEST_SKIP() << "D3D12 设备初始化失败，跳过测试";
        }
    }
    
    void TearDown() override {
        m_texture.reset();
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }
    
    // 创建纹理并验证结果
    RHIResult CreateTexture(const TextureDesc& desc) {
        m_texture = std::make_unique<D3D12Texture>();
        return m_texture->Initialize(m_device.get(), desc);
    }
    
    std::unique_ptr<D3D12Device> m_device;
    std::unique_ptr<D3D12Texture> m_texture;
};

// =============================================================================
// 测试用例 1：2D 纹理创建
// =============================================================================

TEST_F(D3D12TextureTest, Create2DTexture_Success) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    desc.debugName = "Test2DTexture";
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_texture, nullptr);
    
    // 验证基本属性
    EXPECT_EQ(m_texture->GetWidth(), 1024u);
    EXPECT_EQ(m_texture->GetHeight(), 1024u);
    EXPECT_EQ(m_texture->GetDepth(), 1u);
    EXPECT_EQ(m_texture->GetMipLevels(), 1u);
    EXPECT_EQ(m_texture->GetArraySize(), 1u);
    EXPECT_EQ(m_texture->GetFormat(), Format::R8G8B8A8_UNorm);
    EXPECT_EQ(m_texture->GetUsage(), TextureUsage::ShaderResource);
    
    // 验证 D3D12 资源
    EXPECT_NE(m_texture->GetD3D12Resource(), nullptr);
    
    // 验证 DXGI 格式
    EXPECT_EQ(m_texture->GetDXGIFormat(), DXGI_FORMAT_R8G8B8A8_UNORM);
}

TEST_F(D3D12TextureTest, Create2DTexture_DifferentFormats) {
    // 测试不同格式
    struct FormatTest {
        Format format;
        DXGI_FORMAT expectedDXGI;
    };
    
    FormatTest formats[] = {
        {Format::R8G8B8A8_UNorm, DXGI_FORMAT_R8G8B8A8_UNORM},
        {Format::R8G8B8A8_UNorm_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB},
        {Format::R32G32B32A32_Float, DXGI_FORMAT_R32G32B32A32_FLOAT},
        {Format::R16G16B16A16_Float, DXGI_FORMAT_R16G16B16A16_FLOAT},
        {Format::R32_Float, DXGI_FORMAT_R32_FLOAT},
    };
    
    for (const auto& test : formats) {
        TextureDesc desc = {};
        desc.width = 256;
        desc.height = 256;
        desc.format = test.format;
        desc.usage = TextureUsage::ShaderResource;
        
        RHIResult result = CreateTexture(desc);
        EXPECT_EQ(result, RHIResult::Success) << "格式测试失败";
        
        if (IsSuccess(result)) {
            EXPECT_EQ(m_texture->GetDXGIFormat(), test.expectedDXGI);
        }
    }
}

// =============================================================================
// 测试用例 2：渲染目标纹理
// =============================================================================

TEST_F(D3D12TextureTest, RenderTarget_CreateSuccess) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::RenderTarget | TextureUsage::ShaderResource;
    desc.debugName = "TestRenderTarget";
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_NE(m_texture->GetD3D12Resource(), nullptr);
}

TEST_F(D3D12TextureTest, RenderTarget_CreateRTV) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::RenderTarget;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建 RTV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    HRESULT hr = m_device->GetD3D12Device()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&rtvHeap));
    ASSERT_EQ(hr, S_OK);
    
    // Act - 创建 RTV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    result = m_texture->CreateRTV(m_device.get(), handle);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
}

TEST_F(D3D12TextureTest, RenderTarget_RTVWithMipSlice) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::RenderTarget;
    desc.mipLevels = 4;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建 RTV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = 4;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    m_device->GetD3D12Device()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&rtvHeap));
    
    // Act - 为每个 mip 级别创建 RTV
    uint32_t incrementSize = m_device->GetD3D12Device()->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    
    for (uint32_t mip = 0; mip < 4; ++mip) {
        result = m_texture->CreateRTV(m_device.get(), handle, mip);
        EXPECT_EQ(result, RHIResult::Success);
        
        handle.ptr += incrementSize;
    }
}

// =============================================================================
// 测试用例 3：深度纹理
// =============================================================================

TEST_F(D3D12TextureTest, DepthStencil_CreateSuccess) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::D32_Float;
    desc.usage = TextureUsage::DepthStencil;
    desc.debugName = "TestDepthTexture";
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_NE(m_texture->GetD3D12Resource(), nullptr);
}

TEST_F(D3D12TextureTest, DepthStencil_CreateDSV) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::D32_Float;
    desc.usage = TextureUsage::DepthStencil;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建 DSV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    HRESULT hr = m_device->GetD3D12Device()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&dsvHeap));
    ASSERT_EQ(hr, S_OK);
    
    // Act - 创建 DSV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    result = m_texture->CreateDSV(m_device.get(), handle);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
}

TEST_F(D3D12TextureTest, DepthStencil_DifferentFormats) {
    // 测试不同的深度格式
    struct DepthFormatTest {
        Format format;
        DXGI_FORMAT expectedDXGI;
    };
    
    DepthFormatTest formats[] = {
        {Format::D32_Float, DXGI_FORMAT_D32_FLOAT},
        {Format::D24_UNorm_S8_UInt, DXGI_FORMAT_D24_UNORM_S8_UINT},
        {Format::D16_UNorm, DXGI_FORMAT_D16_UNORM},
    };
    
    for (const auto& test : formats) {
        TextureDesc desc = {};
        desc.width = 1024;
        desc.height = 1024;
        desc.format = test.format;
        desc.usage = TextureUsage::DepthStencil;
        
        RHIResult result = CreateTexture(desc);
        EXPECT_EQ(result, RHIResult::Success) << "深度格式测试失败";
        
        if (IsSuccess(result)) {
            EXPECT_EQ(m_texture->GetDXGIFormat(), test.expectedDXGI);
        }
    }
}

// =============================================================================
// 测试用例 4：SRV 创建
// =============================================================================

TEST_F(D3D12TextureTest, ShaderResource_CreateSRV) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建 SRV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    HRESULT hr = m_device->GetD3D12Device()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&srvHeap));
    ASSERT_EQ(hr, S_OK);
    
    // Act - 创建 SRV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    result = m_texture->CreateSRV(m_device.get(), handle);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
}

TEST_F(D3D12TextureTest, ShaderResource_SRVWithMipRange) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    desc.mipLevels = 5;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建 SRV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    m_device->GetD3D12Device()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&srvHeap));
    
    // Act - 创建指定 mip 范围的 SRV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    result = m_texture->CreateSRV(m_device.get(), handle, 1, 3);  // 从 mip 1 开始，共 3 个
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
}

// =============================================================================
// 测试用例 5：Mip 链支持
// =============================================================================

TEST_F(D3D12TextureTest, MipChain_CreateSuccess) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.mipLevels = 11;  // 1024 的完整 Mip 链
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_texture->GetMipLevels(), 11u);
}

TEST_F(D3D12TextureTest, MipChain_GetMipDimensions) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 512;
    desc.mipLevels = 10;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 验证每个 mip 级别的尺寸
    EXPECT_EQ(m_texture->GetWidth(0), 1024u);
    EXPECT_EQ(m_texture->GetHeight(0), 512u);
    
    EXPECT_EQ(m_texture->GetWidth(1), 512u);
    EXPECT_EQ(m_texture->GetHeight(1), 256u);
    
    EXPECT_EQ(m_texture->GetWidth(2), 256u);
    EXPECT_EQ(m_texture->GetHeight(2), 128u);
    
    // 最后一个 mip 级别
    EXPECT_EQ(m_texture->GetWidth(9), 2u);
    EXPECT_EQ(m_texture->GetHeight(9), 1u);
}

TEST_F(D3D12TextureTest, MipChain_CalcSubresource) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 512;
    desc.height = 512;
    desc.mipLevels = 5;
    desc.arraySize = 1;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 验证子资源计算
    EXPECT_EQ(m_texture->CalcSubresource(0, 0), 0u);
    EXPECT_EQ(m_texture->CalcSubresource(1, 0), 1u);
    EXPECT_EQ(m_texture->CalcSubresource(4, 0), 4u);
    
    // 验证子资源总数
    EXPECT_EQ(m_texture->GetSubresourceCount(), 5u);
}

// =============================================================================
// 测试用例 6：纹理数组
// =============================================================================

TEST_F(D3D12TextureTest, TextureArray_CreateSuccess) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 512;
    desc.height = 512;
    desc.arraySize = 6;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_texture->GetArraySize(), 6u);
    EXPECT_EQ(m_texture->GetSubresourceCount(), 6u);  // 1 mip * 6 slices
}

TEST_F(D3D12TextureTest, TextureArray_CreateSRV) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 512;
    desc.height = 512;
    desc.arraySize = 4;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建 SRV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    m_device->GetD3D12Device()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&srvHeap));
    
    // Act - 创建纹理数组 SRV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    result = m_texture->CreateSRV(m_device.get(), handle);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
}

TEST_F(D3D12TextureTest, CubeMap_CreateSuccess) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 512;
    desc.height = 512;
    desc.arraySize = 6;  // CubeMap: 6 个面
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_texture->IsCubeMap());
}

TEST_F(D3D12TextureTest, CubeMap_CreateSRV) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 512;
    desc.height = 512;
    desc.arraySize = 6;
    desc.mipLevels = 5;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建 SRV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    m_device->GetD3D12Device()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&srvHeap));
    
    // Act - 创建 CubeMap SRV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    result = m_texture->CreateSRV(m_device.get(), handle);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
}

// =============================================================================
// 测试用例 7：错误处理
// =============================================================================

TEST_F(D3D12TextureTest, ErrorHandling_ZeroWidth) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 0;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12TextureTest, ErrorHandling_ZeroHeight) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 0;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12TextureTest, ErrorHandling_UnknownFormat) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::Unknown;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12TextureTest, ErrorHandling_NoUsage) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::None;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12TextureTest, ErrorHandling_NullDevice) {
    // Arrange
    D3D12Texture texture;
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act - 使用空设备
    RHIResult result = texture.Initialize(nullptr, desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

// =============================================================================
// 测试用例 8：移动语义
// =============================================================================

TEST_F(D3D12TextureTest, MoveConstructor_TransferOwnership) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    ID3D12Resource* originalResource = m_texture->GetD3D12Resource();
    
    // Act - 移动构造
    D3D12Texture movedTexture(std::move(*m_texture));
    
    // Assert
    EXPECT_EQ(movedTexture.GetWidth(), 1024u);
    EXPECT_EQ(movedTexture.GetD3D12Resource(), originalResource);
    EXPECT_EQ(m_texture->GetD3D12Resource(), nullptr);  // 原对象应该为空
}

TEST_F(D3D12TextureTest, MoveAssignment_TransferOwnership) {
    // Arrange
    TextureDesc desc1 = {};
    desc1.width = 1024;
    desc1.height = 1024;
    desc1.format = Format::R8G8B8A8_UNorm;
    desc1.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc1);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建另一个纹理
    D3D12Texture anotherTexture;
    TextureDesc desc2 = {};
    desc2.width = 512;
    desc2.height = 512;
    desc2.format = Format::R8G8B8A8_UNorm;
    desc2.usage = TextureUsage::ShaderResource;
    anotherTexture.Initialize(m_device.get(), desc2);
    
    // Act - 移动赋值
    anotherTexture = std::move(*m_texture);
    
    // Assert
    EXPECT_EQ(anotherTexture.GetWidth(), 1024u);
}

// =============================================================================
// 测试用例 9：资源状态
// =============================================================================

TEST_F(D3D12TextureTest, ResourceState_InitialState) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::RenderTarget;
    desc.initialState = ResourceState::RenderTarget;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_texture->GetResourceState(), D3D12_RESOURCE_STATE_RENDER_TARGET);
}

TEST_F(D3D12TextureTest, ResourceState_DepthStencil) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::D32_Float;
    desc.usage = TextureUsage::DepthStencil;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    // 深度纹理应该初始化为 DEPTH_WRITE 状态
    EXPECT_EQ(m_texture->GetResourceState(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

TEST_F(D3D12TextureTest, ResourceState_SetState) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 1024;
    desc.height = 1024;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    RHIResult result = CreateTexture(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    m_texture->SetResourceState(D3D12_RESOURCE_STATE_COPY_DEST);
    
    // Assert
    EXPECT_EQ(m_texture->GetResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
}

// =============================================================================
// 测试用例 10：大型纹理
// =============================================================================

TEST_F(D3D12TextureTest, LargeTexture_CreateSuccess) {
    // Arrange
    TextureDesc desc = {};
    desc.width = 4096;
    desc.height = 4096;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    // Act
    RHIResult result = CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_texture->GetWidth(), 4096u);
    EXPECT_EQ(m_texture->GetHeight(), 4096u);
}

// =============================================================================
// 主函数
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}