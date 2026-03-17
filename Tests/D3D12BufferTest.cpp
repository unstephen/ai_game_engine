// =============================================================================
// D3D12BufferTest.cpp - D3D12Buffer 单元测试
// =============================================================================

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "D3D12Buffer.h"
#include "D3D12Device.h"

#include <Windows.h>
#include <d3d12.h>

using namespace Engine::RHI;
using namespace Engine::RHI::D3D12;

// =============================================================================
// 测试固件
// =============================================================================

class D3D12BufferTest : public ::testing::Test {
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
        m_buffer.reset();
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }
    
    // 创建缓冲区并验证结果
    RHIResult CreateBuffer(const BufferDesc& desc) {
        m_buffer = std::make_unique<D3D12Buffer>();
        return m_buffer->Initialize(m_device.get(), desc);
    }
    
    std::unique_ptr<D3D12Device> m_device;
    std::unique_ptr<D3D12Buffer> m_buffer;
};

// =============================================================================
// 测试用例 1：上传堆缓冲区创建
// =============================================================================

TEST_F(D3D12BufferTest, UploadHeap_CreateSuccess) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Constant;
    desc.debugName = "TestUploadBuffer";
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_buffer, nullptr);
    
    // 验证基本属性
    EXPECT_GE(m_buffer->GetSize(), 1024u);  // 可能因对齐而更大
    EXPECT_TRUE(m_buffer->IsCPUAccessible());
    
    // 验证 GPU 地址有效
    EXPECT_NE(m_buffer->GetGPUVirtualAddress(), 0u);
    
    // 验证 D3D12 资源
    EXPECT_NE(m_buffer->GetD3D12Resource(), nullptr);
}

TEST_F(D3D12BufferTest, UploadHeap_MapAndWrite) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Constant;
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 映射缓冲区
    void* mappedPtr = m_buffer->Map();
    
    // Assert
    ASSERT_NE(mappedPtr, nullptr);
    
    // 写入数据
    float testData[] = {1.0f, 2.0f, 3.0f, 4.0f};
    memcpy(mappedPtr, testData, sizeof(testData));
    
    // 验证数据
    float readData[4];
    memcpy(readData, mappedPtr, sizeof(readData));
    
    EXPECT_FLOAT_EQ(readData[0], 1.0f);
    EXPECT_FLOAT_EQ(readData[1], 2.0f);
    EXPECT_FLOAT_EQ(readData[2], 3.0f);
    EXPECT_FLOAT_EQ(readData[3], 4.0f);
    
    // Unmap
    m_buffer->Unmap();
}

TEST_F(D3D12BufferTest, UploadHeap_UpdateData) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Constant;
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 使用 UpdateData
    float testData[] = {5.0f, 6.0f, 7.0f, 8.0f};
    m_buffer->UpdateData(testData, sizeof(testData));
    
    // Assert - 验证数据
    void* mappedPtr = m_buffer->Map();
    ASSERT_NE(mappedPtr, nullptr);
    
    float readData[4];
    memcpy(readData, mappedPtr, sizeof(readData));
    
    EXPECT_FLOAT_EQ(readData[0], 5.0f);
    EXPECT_FLOAT_EQ(readData[1], 6.0f);
    EXPECT_FLOAT_EQ(readData[2], 7.0f);
    EXPECT_FLOAT_EQ(readData[3], 8.0f);
}

// =============================================================================
// 测试用例 2：默认堆缓冲区创建
// =============================================================================

TEST_F(D3D12BufferTest, DefaultHeap_CreateSuccess) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 4096;
    desc.usage = BufferUsage::Vertex | BufferUsage::Index;
    desc.debugName = "TestDefaultBuffer";
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_buffer, nullptr);
    
    // 验证基本属性
    EXPECT_EQ(m_buffer->GetSize(), 4096u);
    EXPECT_FALSE(m_buffer->IsCPUAccessible());
    
    // 验证 GPU 地址有效
    EXPECT_NE(m_buffer->GetGPUVirtualAddress(), 0u);
    
    // 验证 D3D12 资源
    EXPECT_NE(m_buffer->GetD3D12Resource(), nullptr);
}

TEST_F(D3D12BufferTest, DefaultHeap_MapFails) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 4096;
    desc.usage = BufferUsage::Vertex;  // 默认堆
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 尝试映射默认堆缓冲区
    void* mappedPtr = m_buffer->Map();
    
    // Assert - 应该返回 nullptr
    EXPECT_EQ(mappedPtr, nullptr);
}

TEST_F(D3D12BufferTest, DefaultHeap_UpdateDataFails) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 4096;
    desc.usage = BufferUsage::Vertex;  // 默认堆
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 尝试更新默认堆缓冲区（应该失败或无操作）
    float testData[] = {1.0f, 2.0f, 3.0f, 4.0f};
    
    // 这不应该崩溃，但也不会更新数据
    m_buffer->UpdateData(testData, sizeof(testData));
    
    // 验证缓冲区仍然存在
    EXPECT_NE(m_buffer->GetD3D12Resource(), nullptr);
}

// =============================================================================
// 测试用例 3：常量缓冲区
// =============================================================================

TEST_F(D3D12BufferTest, ConstantBuffer_SizeAlignment) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 100;  // 非 256 对齐
    desc.usage = BufferUsage::Constant;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    
    // 常量缓冲区应该是 256 字节对齐
    EXPECT_GE(m_buffer->GetSize(), 256u);
    EXPECT_EQ(m_buffer->GetSize() % 256, 0u);
}

TEST_F(D3D12BufferTest, ConstantBuffer_CreateSuccess) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 256;
    desc.usage = BufferUsage::Constant;
    desc.debugName = "TestConstantBuffer";
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_buffer->IsCPUAccessible());  // 常量缓冲区应该是上传堆
    
    // 验证用途
    EXPECT_EQ(m_buffer->GetUsage(), BufferUsage::Constant);
}

// =============================================================================
// 测试用例 4：顶点和索引缓冲区
// =============================================================================

TEST_F(D3D12BufferTest, VertexBuffer_CreateSuccess) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.stride = 32;  // 每个顶点 32 字节
    desc.usage = BufferUsage::Vertex;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_buffer->GetStride(), 32u);
    EXPECT_EQ(m_buffer->GetUsage(), BufferUsage::Vertex);
    
    // 获取顶点缓冲区视图
    D3D12_VERTEX_BUFFER_VIEW vbv = m_buffer->GetVertexBufferView();
    EXPECT_NE(vbv.BufferLocation, 0ull);
    EXPECT_EQ(vbv.SizeInBytes, 1024u);
    EXPECT_EQ(vbv.StrideInBytes, 32u);
}

TEST_F(D3D12BufferTest, IndexBuffer_CreateSuccess) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 2048;
    desc.stride = 4;  // 32 位索引
    desc.usage = BufferUsage::Index;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_buffer->GetStride(), 4u);
    EXPECT_EQ(m_buffer->GetUsage(), BufferUsage::Index);
    
    // 获取索引缓冲区视图
    D3D12_INDEX_BUFFER_VIEW ibv = m_buffer->GetIndexBufferView();
    EXPECT_NE(ibv.BufferLocation, 0ull);
    EXPECT_EQ(ibv.SizeInBytes, 2048u);
    EXPECT_EQ(ibv.Format, DXGI_FORMAT_R32_UINT);
}

TEST_F(D3D12BufferTest, IndexBuffer_16BitFormat) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.stride = 2;  // 16 位索引
    desc.usage = BufferUsage::Index;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    
    // 验证索引格式
    D3D12_INDEX_BUFFER_VIEW ibv = m_buffer->GetIndexBufferView();
    EXPECT_EQ(ibv.Format, DXGI_FORMAT_R16_UINT);
}

// =============================================================================
// 测试用例 5：错误处理
// =============================================================================

TEST_F(D3D12BufferTest, ErrorHandling_ZeroSize) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 0;  // 无效大小
    desc.usage = BufferUsage::Vertex;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12BufferTest, ErrorHandling_NoUsage) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::None;  // 无用途
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12BufferTest, ErrorHandling_NullDevice) {
    // Arrange
    D3D12Buffer buffer;
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Vertex;
    
    // Act - 使用空设备
    RHIResult result = buffer.Initialize(nullptr, desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12BufferTest, ErrorHandling_UpdateDataOutOfRange) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 100;
    desc.usage = BufferUsage::Constant;
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 尝试写入超出范围的数据
    uint8_t testData[200] = {};
    
    // 这不应该崩溃
    m_buffer->UpdateData(testData, 200, 0);  // 偏移 0，大小 200 > 100
    
    // 验证缓冲区仍然存在
    EXPECT_NE(m_buffer->GetD3D12Resource(), nullptr);
}

// =============================================================================
// 测试用例 6：初始数据上传
// =============================================================================

TEST_F(D3D12BufferTest, InitialData_UploadSuccess) {
    // Arrange
    float initialData[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    
    BufferDesc desc = {};
    desc.size = sizeof(initialData);
    desc.usage = BufferUsage::Constant;
    desc.initialData = initialData;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    
    // 验证数据已上传
    void* mappedPtr = m_buffer->Map();
    ASSERT_NE(mappedPtr, nullptr);
    
    float readData[8];
    memcpy(readData, mappedPtr, sizeof(readData));
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_FLOAT_EQ(readData[i], initialData[i]);
    }
}

// =============================================================================
// 测试用例 7：移动语义
// =============================================================================

TEST_F(D3D12BufferTest, MoveConstructor_TransferOwnership) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Constant;
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    uint64_t originalGPUAddress = m_buffer->GetGPUVirtualAddress();
    ID3D12Resource* originalResource = m_buffer->GetD3D12Resource();
    
    // Act - 移动构造
    D3D12Buffer movedBuffer(std::move(*m_buffer));
    
    // Assert
    EXPECT_EQ(movedBuffer.GetGPUVirtualAddress(), originalGPUAddress);
    EXPECT_EQ(movedBuffer.GetD3D12Resource(), originalResource);
    EXPECT_EQ(m_buffer->GetD3D12Resource(), nullptr);  // 原对象应该为空
}

TEST_F(D3D12BufferTest, MoveAssignment_TransferOwnership) {
    // Arrange
    BufferDesc desc1 = {};
    desc1.size = 1024;
    desc1.usage = BufferUsage::Constant;
    
    RHIResult result = CreateBuffer(desc1);
    ASSERT_EQ(result, RHIResult::Success);
    
    uint64_t originalGPUAddress = m_buffer->GetGPUVirtualAddress();
    
    // 创建另一个缓冲区
    D3D12Buffer anotherBuffer;
    BufferDesc desc2 = {};
    desc2.size = 512;
    desc2.usage = BufferUsage::Constant;
    anotherBuffer.Initialize(m_device.get(), desc2);
    
    // Act - 移动赋值
    anotherBuffer = std::move(*m_buffer);
    
    // Assert
    EXPECT_EQ(anotherBuffer.GetGPUVirtualAddress(), originalGPUAddress);
    EXPECT_EQ(anotherBuffer.GetSize(), 1024u);
}

// =============================================================================
// 测试用例 8：多次映射和取消映射
// =============================================================================

TEST_F(D3D12BufferTest, MultipleMapCalls_ReturnSamePointer) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Constant;
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 多次映射
    void* ptr1 = m_buffer->Map();
    void* ptr2 = m_buffer->Map();
    void* ptr3 = m_buffer->Map();
    
    // Assert - 应该返回相同的指针
    EXPECT_NE(ptr1, nullptr);
    EXPECT_EQ(ptr1, ptr2);
    EXPECT_EQ(ptr2, ptr3);
}

TEST_F(D3D12BufferTest, MapUnmapSequence) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Constant;
    
    RHIResult result = CreateBuffer(desc);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 多次映射/取消映射
    for (int i = 0; i < 5; ++i) {
        void* ptr = m_buffer->Map();
        EXPECT_NE(ptr, nullptr);
        m_buffer->Unmap();
    }
}

// =============================================================================
// 测试用例 9：大型缓冲区
// =============================================================================

TEST_F(D3D12BufferTest, LargeBuffer_CreateSuccess) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 16 * 1024 * 1024;  // 16 MB
    desc.usage = BufferUsage::Vertex;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_buffer->GetSize(), 16 * 1024 * 1024ull);
}

// =============================================================================
// 测试用例 10：混合用途缓冲区
// =============================================================================

TEST_F(D3D12BufferTest, MixedUsage_VertexAndIndex) {
    // Arrange
    BufferDesc desc = {};
    desc.size = 4096;
    desc.stride = 32;
    desc.usage = BufferUsage::Vertex | BufferUsage::Index;
    
    // Act
    RHIResult result = CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_buffer->GetUsage(), BufferUsage::Vertex | BufferUsage::Index);
}

// =============================================================================
// 主函数
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}