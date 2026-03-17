// =============================================================================
// D3D12CommandListTest.cpp - D3D12CommandList 单元测试
// =============================================================================

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"

#include <Windows.h>
#include <d3d12.h>

using namespace Engine::RHI;
using namespace Engine::RHI::D3D12;

// =============================================================================
// 测试固件
// =============================================================================

class D3D12CommandListTest : public ::testing::Test {
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
        
        // 创建命令分配器
        HRESULT hr = m_device->GetD3D12Device()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_commandAllocator)
        );
        
        if (FAILED(hr)) {
            GTEST_SKIP() << "命令分配器创建失败，跳过测试";
        }
    }
    
    void TearDown() override {
        m_commandList.reset();
        m_commandAllocator.Reset();
        
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }
    
    // 创建命令列表并验证结果
    RHIResult CreateCommandList(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) {
        m_commandList = std::make_unique<D3D12CommandList>();
        return m_commandList->Initialize(m_device.get(), m_commandAllocator.Get(), type);
    }
    
    std::unique_ptr<D3D12Device> m_device;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    std::unique_ptr<D3D12CommandList> m_commandList;
};

// =============================================================================
// 测试用例 1：命令列表生命周期
// =============================================================================

TEST_F(D3D12CommandListTest, Lifecycle_InitializeSuccess) {
    // Arrange & Act
    RHIResult result = CreateCommandList();
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_commandList, nullptr);
    
    // 验证初始状态
    EXPECT_FALSE(m_commandList->IsRecording());
    EXPECT_TRUE(m_commandList->IsClosed());
    EXPECT_NE(m_commandList->GetD3D12CommandList(), nullptr);
    EXPECT_NE(m_commandList->GetNativeCommandList(), nullptr);
}

TEST_F(D3D12CommandListTest, Lifecycle_BeginEndCycle) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 第一次 Begin/End 循环
    {
        // Act - Begin
        m_commandList->Begin();
        
        // Assert - 录制状态
        EXPECT_TRUE(m_commandList->IsRecording());
        EXPECT_FALSE(m_commandList->IsClosed());
        
        // Act - End
        m_commandList->End();
        
        // Assert - 关闭状态
        EXPECT_FALSE(m_commandList->IsRecording());
        EXPECT_TRUE(m_commandList->IsClosed());
    }
    
    // 第二次 Begin/End 循环（验证可重用）
    {
        // Act - Begin
        m_commandList->Begin();
        
        // Assert
        EXPECT_TRUE(m_commandList->IsRecording());
        
        // Act - End
        m_commandList->End();
        
        // Assert
        EXPECT_TRUE(m_commandList->IsClosed());
    }
}

TEST_F(D3D12CommandListTest, Lifecycle_MultipleBeginCalls) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 多次调用 Begin
    m_commandList->Begin();
    m_commandList->Begin(); // 重复调用应该被忽略或警告
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, Lifecycle_EndWithoutBegin) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 在未调用 Begin 的情况下调用 End
    m_commandList->End();
    
    // Assert - 状态不应该改变
    EXPECT_FALSE(m_commandList->IsRecording());
    EXPECT_TRUE(m_commandList->IsClosed());
}

// =============================================================================
// 测试用例 2：视口设置
// =============================================================================

TEST_F(D3D12CommandListTest, Viewport_SetSuccess) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetViewport(0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 1.0f);
    
    // Assert - 命令列表仍然在录制状态
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, Viewport_MultipleViewports) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 设置多个视口（模拟场景切换）
    m_commandList->SetViewport(0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f);
    m_commandList->SetViewport(0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 1.0f);
    m_commandList->SetViewport(100.0f, 100.0f, 512.0f, 512.0f, 0.0f, 1.0f);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 3：裁剪矩形设置
// =============================================================================

TEST_F(D3D12CommandListTest, ScissorRect_SetSuccess) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetScissorRect(0, 0, 1920, 1080);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, ScissorRect_PartialScreen) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 设置部分屏幕裁剪
    m_commandList->SetScissorRect(100, 100, 500, 500);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 4：图元拓扑设置
// =============================================================================

TEST_F(D3D12CommandListTest, PrimitiveTopology_TriangleList) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, PrimitiveTopology_MultipleTypes) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 设置不同类型
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::PointList);
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::LineList);
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleStrip);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 5：顶点缓冲区绑定
// =============================================================================

TEST_F(D3D12CommandListTest, VertexBuffer_BindSuccess) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建顶点缓冲区
    BufferDesc vbDesc = {};
    vbDesc.size = 1024;
    vbDesc.stride = 32; // 每顶点 32 字节
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.debugName = "TestVertexBuffer";
    
    auto vertexBuffer = std::make_unique<D3D12Buffer>();
    RHIResult vbResult = vertexBuffer->Initialize(m_device.get(), vbDesc);
    ASSERT_EQ(vbResult, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetVertexBuffer(0, vertexBuffer.get(), 0);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, VertexBuffer_MultipleSlots) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建多个顶点缓冲区
    BufferDesc vbDesc = {};
    vbDesc.size = 512;
    vbDesc.stride = 16;
    vbDesc.usage = BufferUsage::Vertex;
    
    auto vb0 = std::make_unique<D3D12Buffer>();
    auto vb1 = std::make_unique<D3D12Buffer>();
    auto vb2 = std::make_unique<D3D12Buffer>();
    
    ASSERT_EQ(vb0->Initialize(m_device.get(), vbDesc), RHIResult::Success);
    ASSERT_EQ(vb1->Initialize(m_device.get(), vbDesc), RHIResult::Success);
    ASSERT_EQ(vb2->Initialize(m_device.get(), vbDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 绑定到多个槽位
    m_commandList->SetVertexBuffer(0, vb0.get(), 0);
    m_commandList->SetVertexBuffer(1, vb1.get(), 0);
    m_commandList->SetVertexBuffer(2, vb2.get(), 0);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, VertexBuffer_WithOffset) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    BufferDesc vbDesc = {};
    vbDesc.size = 1024;
    vbDesc.stride = 32;
    vbDesc.usage = BufferUsage::Vertex;
    
    auto vertexBuffer = std::make_unique<D3D12Buffer>();
    ASSERT_EQ(vertexBuffer->Initialize(m_device.get(), vbDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 带偏移绑定
    m_commandList->SetVertexBuffer(0, vertexBuffer.get(), 256); // 256 字节偏移
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 6：索引缓冲区绑定
// =============================================================================

TEST_F(D3D12CommandListTest, IndexBuffer_BindSuccess) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建索引缓冲区
    BufferDesc ibDesc = {};
    ibDesc.size = 1024;
    ibDesc.stride = 4; // 32 位索引
    ibDesc.usage = BufferUsage::Index;
    ibDesc.debugName = "TestIndexBuffer";
    
    auto indexBuffer = std::make_unique<D3D12Buffer>();
    RHIResult ibResult = indexBuffer->Initialize(m_device.get(), ibDesc);
    ASSERT_EQ(ibResult, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetIndexBuffer(indexBuffer.get(), 0);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, IndexBuffer_DifferentFormats) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建索引缓冲区
    BufferDesc ibDesc = {};
    ibDesc.size = 1024;
    ibDesc.usage = BufferUsage::Index;
    
    auto indexBuffer = std::make_unique<D3D12Buffer>();
    ASSERT_EQ(indexBuffer->Initialize(m_device.get(), ibDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 使用不同的格式
    m_commandList->SetIndexBuffer(indexBuffer.get(), 0, DXGI_FORMAT_R16_UINT); // 16 位
    m_commandList->SetIndexBuffer(indexBuffer.get(), 0, DXGI_FORMAT_R32_UINT); // 32 位
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 7：绘制命令
// =============================================================================

TEST_F(D3D12CommandListTest, Draw_SimpleDraw) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    
    // Act
    m_commandList->Draw(3, 0); // 绘制一个三角形
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, Draw_MultipleDraws) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    
    // Act - 多次绘制
    m_commandList->Draw(3, 0);    // 第一个三角形
    m_commandList->Draw(3, 3);    // 第二个三角形
    m_commandList->Draw(6, 6);    // 两个三角形
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, DrawInstanced_Success) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    
    // Act - 实例化绘制
    m_commandList->DrawInstanced(3, 10, 0, 0); // 10 个实例，每个 3 顶点
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, DrawIndexed_Success) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建索引缓冲区
    BufferDesc ibDesc = {};
    ibDesc.size = 128;
    ibDesc.usage = BufferUsage::Index;
    
    auto indexBuffer = std::make_unique<D3D12Buffer>();
    ASSERT_EQ(indexBuffer->Initialize(m_device.get(), ibDesc), RHIResult::Success);
    
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    m_commandList->SetIndexBuffer(indexBuffer.get(), 0);
    
    // Act
    m_commandList->DrawIndexed(3, 0, 0); // 绘制一个索引三角形
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, DrawIndexedInstanced_Success) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    BufferDesc ibDesc = {};
    ibDesc.size = 128;
    ibDesc.usage = BufferUsage::Index;
    
    auto indexBuffer = std::make_unique<D3D12Buffer>();
    ASSERT_EQ(indexBuffer->Initialize(m_device.get(), ibDesc), RHIResult::Success);
    
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    m_commandList->SetIndexBuffer(indexBuffer.get(), 0);
    
    // Act - 索引实例化绘制
    m_commandList->DrawIndexedInstanced(36, 5, 0, 0, 0); // 5 个实例，每个 36 索引
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 8：资源屏障
// =============================================================================

TEST_F(D3D12CommandListTest, ResourceBarrier_TextureTransition) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建纹理
    TextureDesc texDesc = {};
    texDesc.width = 512;
    texDesc.height = 512;
    texDesc.format = Format::R8G8B8A8_UNorm;
    texDesc.usage = TextureUsage::RenderTarget;
    texDesc.initialState = ResourceState::Common;
    
    auto texture = std::make_unique<D3D12Texture>();
    ASSERT_EQ(texture->Initialize(m_device.get(), texDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 资源状态转换
    m_commandList->ResourceBarrier(texture.get(), ResourceState::RenderTarget);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, ResourceBarrier_BufferTransition) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建缓冲区
    BufferDesc bufDesc = {};
    bufDesc.size = 1024;
    bufDesc.usage = BufferUsage::Constant;
    
    auto buffer = std::make_unique<D3D12Buffer>();
    ASSERT_EQ(buffer->Initialize(m_device.get(), bufDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 资源状态转换
    m_commandList->ResourceBarrier(buffer.get(), ResourceState::ConstantBuffer);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, ResourceBarrier_MultipleTransitions) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建多个资源
    TextureDesc texDesc = {};
    texDesc.width = 256;
    texDesc.height = 256;
    texDesc.format = Format::R8G8B8A8_UNorm;
    texDesc.usage = TextureUsage::ShaderResource | TextureUsage::RenderTarget;
    
    auto tex1 = std::make_unique<D3D12Texture>();
    auto tex2 = std::make_unique<D3D12Texture>();
    ASSERT_EQ(tex1->Initialize(m_device.get(), texDesc), RHIResult::Success);
    ASSERT_EQ(tex2->Initialize(m_device.get(), texDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 多次状态转换
    m_commandList->ResourceBarrier(tex1.get(), ResourceState::RenderTarget);
    m_commandList->ResourceBarrier(tex2.get(), ResourceState::RenderTarget);
    m_commandList->ResourceBarrier(tex1.get(), ResourceState::ShaderResource);
    m_commandList->ResourceBarrier(tex2.get(), ResourceState::ShaderResource);
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, ResourceBarrier_D3D12Native) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建纹理
    TextureDesc texDesc = {};
    texDesc.width = 512;
    texDesc.height = 512;
    texDesc.format = Format::R8G8B8A8_UNorm;
    texDesc.usage = TextureUsage::RenderTarget;
    
    auto texture = std::make_unique<D3D12Texture>();
    ASSERT_EQ(texture->Initialize(m_device.get(), texDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 使用原生 D3D12 API
    m_commandList->TransitionBarrier(
        texture->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 9：调试标记
// =============================================================================

TEST_F(D3D12CommandListTest, DebugMarkers_SetMarker) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetMarker("TestMarker");
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, DebugMarkers_BeginEndEvent) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->BeginEvent("RenderPass");
    {
        m_commandList->SetMarker("DrawGeometry");
        m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
        m_commandList->Draw(3, 0);
    }
    m_commandList->EndEvent();
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, DebugMarkers_NestedEvents) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 嵌套事件
    m_commandList->BeginEvent("Frame");
    {
        m_commandList->BeginEvent("ShadowPass");
        {
            m_commandList->SetMarker("DrawShadowCasters");
        }
        m_commandList->EndEvent();
        
        m_commandList->BeginEvent("MainPass");
        {
            m_commandList->SetMarker("DrawGeometry");
            m_commandList->SetMarker("DrawTransparency");
        }
        m_commandList->EndEvent();
    }
    m_commandList->EndEvent();
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 10：计算调度
// =============================================================================

TEST_F(D3D12CommandListTest, Dispatch_Simple) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->Dispatch(16, 16, 1); // 16x16 工作组
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, Dispatch_3DGroups) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 3D 工作组
    m_commandList->Dispatch(8, 8, 4); // 8x8x4 工作组
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 11：复制命令
// =============================================================================

TEST_F(D3D12CommandListTest, CopyBufferRegion_Success) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建源和目标缓冲区
    BufferDesc bufDesc = {};
    bufDesc.size = 1024;
    bufDesc.usage = BufferUsage::CopySource;
    
    auto srcBuffer = std::make_unique<D3D12Buffer>();
    auto destBuffer = std::make_unique<D3D12Buffer>();
    
    bufDesc.usage = BufferUsage::CopySource;
    ASSERT_EQ(srcBuffer->Initialize(m_device.get(), bufDesc), RHIResult::Success);
    
    bufDesc.usage = BufferUsage::CopyDest;
    ASSERT_EQ(destBuffer->Initialize(m_device.get(), bufDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->CopyBufferRegion(
        destBuffer->GetD3D12Resource(), 0,
        srcBuffer->GetD3D12Resource(), 0,
        512
    );
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 12：描述符堆绑定
// =============================================================================

TEST_F(D3D12CommandListTest, DescriptorHeap_SetSingleHeap) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 16;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
    ComPtr<ID3D12DescriptorHeap> heap;
    HRESULT hr = m_device->GetD3D12Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
    ASSERT_EQ(hr, S_OK);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetDescriptorHeaps(heap.Get());
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 13：根签名常量缓冲区视图
// =============================================================================

TEST_F(D3D12CommandListTest, RootConstantBuffer_SetSuccess) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建常量缓冲区
    BufferDesc cbDesc = {};
    cbDesc.size = 256; // 256 字节对齐
    cbDesc.usage = BufferUsage::Constant;
    
    auto constantBuffer = std::make_unique<D3D12Buffer>();
    ASSERT_EQ(constantBuffer->Initialize(m_device.get(), cbDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act
    m_commandList->SetGraphicsRootConstantBufferView(
        0,
        constantBuffer->GetGPUVirtualAddress()
    );
    
    // Assert
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 14：错误处理
// =============================================================================

TEST_F(D3D12CommandListTest, ErrorHandling_NullBuffer) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 传入空缓冲区
    m_commandList->SetVertexBuffer(0, nullptr, 0);
    m_commandList->SetIndexBuffer(nullptr, 0);
    
    // Assert - 命令列表仍然有效
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

TEST_F(D3D12CommandListTest, ErrorHandling_InvalidSlot) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    BufferDesc vbDesc = {};
    vbDesc.size = 256;
    vbDesc.stride = 16;
    vbDesc.usage = BufferUsage::Vertex;
    
    auto vertexBuffer = std::make_unique<D3D12Buffer>();
    ASSERT_EQ(vertexBuffer->Initialize(m_device.get(), vbDesc), RHIResult::Success);
    
    m_commandList->Begin();
    
    // Act - 使用无效槽位（超过最大槽位数）
    // 注意：这应该在日志中产生警告，但不应该崩溃
    m_commandList->SetVertexBuffer(100, vertexBuffer.get(), 0);
    
    // Assert - 命令列表仍然有效
    EXPECT_TRUE(m_commandList->IsRecording());
    
    m_commandList->End();
}

// =============================================================================
// 测试用例 15：重置命令列表
// =============================================================================

TEST_F(D3D12CommandListTest, Reset_Success) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 第一次录制
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    m_commandList->Draw(3, 0);
    m_commandList->End();
    
    // Act - 重置并重新录制
    m_commandList->Reset();
    
    // Assert - 应该在录制状态
    EXPECT_TRUE(m_commandList->IsRecording());
    EXPECT_FALSE(m_commandList->IsClosed());
    
    // 新的录制
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::PointList);
    m_commandList->Draw(100, 0);
    m_commandList->End();
    
    EXPECT_TRUE(m_commandList->IsClosed());
}

// =============================================================================
// 性能测试
// =============================================================================

TEST_F(D3D12CommandListTest, Performance_ManyDrawCalls) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    
    // Act - 记录大量绘制调用时间
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; i++) {
        m_commandList->Draw(3, 0);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Assert - 10000 次绘制应该在合理时间内完成（例如 < 100ms）
    EXPECT_LT(duration.count(), 100000);
    
    m_commandList->End();
    
    // 输出性能信息
    std::cout << "10000 次绘制调用耗时: " << duration.count() << " 微秒" << std::endl;
}

// =============================================================================
// 移动语义测试
// =============================================================================

TEST_F(D3D12CommandListTest, MoveConstructor_Success) {
    // Arrange
    RHIResult result = CreateCommandList();
    ASSERT_EQ(result, RHIResult::Success);
    
    m_commandList->Begin();
    m_commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    m_commandList->End();
    
    // Act - 移动构造
    D3D12CommandList movedList = std::move(*m_commandList);
    
    // Assert
    EXPECT_TRUE(movedList.IsClosed());
    EXPECT_NE(movedList.GetD3D12CommandList(), nullptr);
}

// =============================================================================
// 命令列表类型测试
// =============================================================================

TEST_F(D3D12CommandListTest, CommandListType_Direct) {
    // Arrange & Act
    RHIResult result = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_commandList->GetType(), static_cast<uint32_t>(D3D12_COMMAND_LIST_TYPE_DIRECT));
}

TEST_F(D3D12CommandListTest, CommandListType_Compute) {
    // Arrange - 创建计算类型的命令分配器
    ComPtr<ID3D12CommandAllocator> computeAllocator;
    HRESULT hr = m_device->GetD3D12Device()->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        IID_PPV_ARGS(&computeAllocator)
    );
    
    if (FAILED(hr)) {
        GTEST_SKIP() << "计算命令分配器创建失败";
    }
    
    // Act
    m_commandList = std::make_unique<D3D12CommandList>();
    RHIResult result = m_commandList->Initialize(
        m_device.get(),
        computeAllocator.Get(),
        D3D12_COMMAND_LIST_TYPE_COMPUTE
    );
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_commandList->GetType(), static_cast<uint32_t>(D3D12_COMMAND_LIST_TYPE_COMPUTE));
}