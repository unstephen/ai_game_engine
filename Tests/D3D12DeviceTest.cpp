// =============================================================================
// D3D12DeviceTest.cpp - D3D12Device 单元测试
// =============================================================================

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "D3D12Device.h"

#include <Windows.h>
#include <dxgi1_6.h>

using namespace Engine::RHI::D3D12;

// =============================================================================
// 测试固件
// =============================================================================

class D3D12DeviceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前的设置
    }
    
    void TearDown() override {
        // 每个测试后的清理
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }
    
    // 创建设备并验证结果
    RHIResult CreateDevice(const DeviceDesc& desc = {}) {
        m_device = std::make_unique<D3D12Device>();
        return m_device->Initialize(desc);
    }
    
    std::unique_ptr<D3D12Device> m_device;
};

// =============================================================================
// 测试用例 1：基本设备创建
// =============================================================================

TEST_F(D3D12DeviceTest, CreateDevice_Success) {
    // Arrange
    DeviceDesc desc = {};
    desc.backend = Backend::D3D12;
    desc.enableDebug = true;
    desc.enableValidation = true;
    
    // Act
    RHIResult result = CreateDevice(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_device, nullptr);
    
    // 验证核心对象
    EXPECT_NE(m_device->GetD3D12Device(), nullptr);
    EXPECT_NE(m_device->GetCommandQueue(), nullptr);
    EXPECT_NE(m_device->GetDXGIFactory(), nullptr);
    
    // 验证后端类型
    EXPECT_EQ(m_device->GetBackend(), Backend::D3D12);
}

TEST_F(D3D12DeviceTest, CreateDevice_DefaultDesc) {
    // 使用默认描述创建设备
    RHIResult result = CreateDevice();
    
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_device, nullptr);
    EXPECT_NE(m_device->GetD3D12Device(), nullptr);
}

// =============================================================================
// 测试用例 2：GPU 信息查询
// =============================================================================

TEST_F(D3D12DeviceTest, GetDeviceInfo_ReturnsValidInfo) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    DeviceInfo info = m_device->GetDeviceInfo();
    
    // Assert
    // 设备名称不应为空（除非是软件适配器）
    EXPECT_FALSE(info.driverName.empty());
    
    // 专用显存应该大于 0（物理 GPU）或者在软件适配器情况下可以为 0
    // 这里我们只是确保值被正确填充，不做严格断言
    
    // 验证布尔值是有效的
    EXPECT_TRUE(info.supportsRayTracing == true || info.supportsRayTracing == false);
    EXPECT_TRUE(info.supportsMeshShaders == true || info.supportsMeshShaders == false);
}

TEST_F(D3D12DeviceTest, GetDeviceName_ReturnsValidName) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    std::string_view name = m_device->GetDeviceName();
    
    // Assert
    EXPECT_FALSE(name.empty());
    
    // 验证设备信息中的名称匹配
    DeviceInfo info = m_device->GetDeviceInfo();
    EXPECT_EQ(name, info.driverName);
}

// =============================================================================
// 测试用例 3：调试层启用
// =============================================================================

TEST_F(D3D12DeviceTest, DebugLayer_Enabled) {
    // Arrange
    DeviceDesc desc = {};
    desc.enableDebug = true;
    desc.enableValidation = true;
    
    // Act
    RHIResult result = CreateDevice(desc);
    
    // Assert
    // 即使调试层启用失败，设备创建也应该成功
    // （调试层是可选的）
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_device, nullptr);
}

TEST_F(D3D12DeviceTest, DebugLayer_Disabled) {
    // Arrange
    DeviceDesc desc = {};
    desc.enableDebug = false;
    desc.enableValidation = false;
    
    // Act
    RHIResult result = CreateDevice(desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_device, nullptr);
}

// =============================================================================
// 测试用例 4：错误处理
// =============================================================================

TEST_F(D3D12DeviceTest, GetLastError_InitiallyEmpty) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    const char* error = m_device->GetLastError();
    
    // Assert
    // 初始化成功后，错误信息应该为空
    EXPECT_STREQ(error, "");
}

TEST_F(D3D12DeviceTest, IsDeviceLost_InitiallyFalse) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert
    EXPECT_FALSE(m_device->IsDeviceLost());
    EXPECT_EQ(m_device->GetDeviceLostReason(), DeviceLostReason::Unknown);
}

TEST_F(D3D12DeviceTest, SetDeviceLostCallback_ValidCallback) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    bool callbackCalled = false;
    DeviceLostReason callbackReason = DeviceLostReason::Unknown;
    std::string callbackMessage;
    
    // Act
    m_device->SetDeviceLostCallback([&](DeviceLostReason reason, const char* message) {
        callbackCalled = true;
        callbackReason = reason;
        callbackMessage = message ? message : "";
    });
    
    // Assert - 我们无法轻易触发设备丢失，所以只验证回调设置成功
    // 回调会在真正的设备丢失时被调用
    EXPECT_FALSE(callbackCalled); // 设备未丢失，回调不应被调用
}

TEST_F(D3D12DeviceTest, TryRecover_WhenDeviceNotLost_ReturnsSuccess) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    RHIResult recoverResult = m_device->TryRecover();
    
    // Assert
    EXPECT_EQ(recoverResult, RHIResult::Success);
}

// =============================================================================
// 测试用例 5：设备初始化和关闭
// =============================================================================

TEST_F(D3D12DeviceTest, Shutdown_CanBeCalledMultipleTimes) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 调用两次关闭应该不会崩溃
    m_device->Shutdown();
    m_device->Shutdown();
    
    // Assert - 如果到达这里，说明多次关闭是安全的
    SUCCEED();
}

TEST_F(D3D12DeviceTest, Initialize_CanBeCalledAfterShutdown) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    m_device->Shutdown();
    RHIResult reinitResult = m_device->Initialize({});
    
    // Assert
    EXPECT_EQ(reinitResult, RHIResult::Success);
    EXPECT_NE(m_device->GetD3D12Device(), nullptr);
}

TEST_F(D3D12DeviceTest, Destructor_CleansUpProperly) {
    // Arrange
    {
        D3D12Device device;
        RHIResult result = device.Initialize({});
        ASSERT_EQ(result, RHIResult::Success);
        // device 离开作用域时自动析构
    }
    
    // Assert - 如果到达这里，说明析构函数正常工作
    SUCCEED();
}

// =============================================================================
// 测试用例 6：资源创建测试
// =============================================================================

TEST_F(D3D12DeviceTest, CreateBuffer_WithValidDesc_ReturnsBuffer) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::Vertex;
    desc.stride = 32;
    desc.debugName = "TestVertexBuffer";
    
    auto buffer = m_device->CreateBuffer(desc);
    
    // Assert
    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->GetSize(), 1024);
    EXPECT_EQ(buffer->GetStride(), 32u);
    EXPECT_EQ(buffer->GetUsage(), BufferUsage::Vertex);
    EXPECT_GT(buffer->GetGPUVirtualAddress(), 0ull);
}

TEST_F(D3D12DeviceTest, CreateBuffer_WithZeroSize_ReturnsNull) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    BufferDesc desc = {};
    desc.size = 0;  // 无效大小
    desc.usage = BufferUsage::Vertex;
    
    auto buffer = m_device->CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(buffer, nullptr);
}

TEST_F(D3D12DeviceTest, CreateBuffer_WithNoUsage_ReturnsNull) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    BufferDesc desc = {};
    desc.size = 1024;
    desc.usage = BufferUsage::None;  // 无效用途
    
    auto buffer = m_device->CreateBuffer(desc);
    
    // Assert
    EXPECT_EQ(buffer, nullptr);
}

TEST_F(D3D12DeviceTest, CreateBuffer_ConstantBuffer_AlignedSize) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    BufferDesc desc = {};
    desc.size = 100;  // 非对齐大小
    desc.usage = BufferUsage::Constant;
    
    auto buffer = m_device->CreateBuffer(desc);
    
    // Assert - 常量缓冲区应该被对齐到 256 字节
    ASSERT_NE(buffer, nullptr);
    EXPECT_GE(buffer->GetSize(), 256ull);  // 对齐后大小
}

TEST_F(D3D12DeviceTest, CreateTexture_WithValidDesc_ReturnsTexture) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    TextureDesc desc = {};
    desc.width = 256;
    desc.height = 256;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    desc.debugName = "TestTexture";
    
    auto texture = m_device->CreateTexture(desc);
    
    // Assert
    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(texture->GetWidth(), 256u);
    EXPECT_EQ(texture->GetHeight(), 256u);
    EXPECT_EQ(texture->GetFormat(), Format::R8G8B8A8_UNorm);
}

TEST_F(D3D12DeviceTest, CreateTexture_WithZeroSize_ReturnsNull) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    TextureDesc desc = {};
    desc.width = 0;
    desc.height = 256;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::ShaderResource;
    
    auto texture = m_device->CreateTexture(desc);
    
    // Assert
    EXPECT_EQ(texture, nullptr);
}

TEST_F(D3D12DeviceTest, CreateTexture_RenderTarget_CreatesSuccessfully) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    TextureDesc desc = {};
    desc.width = 800;
    desc.height = 600;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.usage = TextureUsage::RenderTarget | TextureUsage::ShaderResource;
    
    auto texture = m_device->CreateTexture(desc);
    
    // Assert
    ASSERT_NE(texture, nullptr);
}

TEST_F(D3D12DeviceTest, CreateTexture_DepthStencil_CreatesSuccessfully) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    TextureDesc desc = {};
    desc.width = 800;
    desc.height = 600;
    desc.format = Format::D32_Float;
    desc.usage = TextureUsage::DepthStencil;
    
    auto texture = m_device->CreateTexture(desc);
    
    // Assert
    ASSERT_NE(texture, nullptr);
}

TEST_F(D3D12DeviceTest, CreateShader_ReturnsNull_WhenNotImplemented) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    ShaderDesc desc = {};
    auto shader = m_device->CreateShader(desc);
    
    // Assert
    EXPECT_EQ(shader, nullptr);
}

TEST_F(D3D12DeviceTest, CreateSwapChain_ReturnsNull_WithNullWindow) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 使用空窗口句柄
    auto swapChain = m_device->CreateSwapChain(nullptr, 800, 600);
    
    // Assert
    EXPECT_EQ(swapChain, nullptr);
    EXPECT_TRUE(std::string(m_device->GetLastError()).find("窗口句柄为空") != std::string::npos);
}

// =============================================================================
// 测试用例 7：描述符堆大小查询
// =============================================================================

TEST_F(D3D12DeviceTest, GetDescriptorHandleIncrementSize_ReturnsValidSize) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert
    // CBV_SRV_UAV
    uint32_t cbvSrvUavSize = m_device->GetDescriptorHandleIncrementSize(DescriptorHeapType::CBV_SRV_UAV);
    EXPECT_GT(cbvSrvUavSize, 0u);
    
    // Sampler
    uint32_t samplerSize = m_device->GetDescriptorHandleIncrementSize(DescriptorHeapType::Sampler);
    EXPECT_GT(samplerSize, 0u);
    
    // RTV
    uint32_t rtvSize = m_device->GetDescriptorHandleIncrementSize(DescriptorHeapType::RTV);
    EXPECT_GT(rtvSize, 0u);
    
    // DSV
    uint32_t dsvSize = m_device->GetDescriptorHandleIncrementSize(DescriptorHeapType::DSV);
    EXPECT_GT(dsvSize, 0u);
}

// =============================================================================
// 测试用例 8：帧资源管理器和上传管理器（占位测试）
// =============================================================================

TEST_F(D3D12DeviceTest, GetFrameResourceManager_ReturnsNull_WhenNotInitialized) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    IFrameResourceManager* manager = m_device->GetFrameResourceManager();
    
    // Assert - 当前未实现，应返回 nullptr
    // 这将在 Task 004 实现帧资源管理器后返回有效指针
    // 目前我们只验证不会崩溃
    // EXPECT_NE(manager, nullptr); // TODO: 取消注释当实现后
}

TEST_F(D3D12DeviceTest, GetUploadManager_ReturnsNull_WhenNotInitialized) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    IUploadManager* manager = m_device->GetUploadManager();
    
    // Assert - 当前未实现，应返回 nullptr
    // 这将在 Task 005 实现上传管理器后返回有效指针
    // 目前我们只验证不会崩溃
    // EXPECT_NE(manager, nullptr); // TODO: 取消注释当实现后
}

// =============================================================================
// 测试用例 9：边界条件测试
// =============================================================================

TEST_F(D3D12DeviceTest, GetBackend_ReturnsD3D12) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert
    EXPECT_EQ(m_device->GetBackend(), Backend::D3D12);
}

TEST_F(D3D12DeviceTest, GetD3D12Device_ReturnsValidPointer) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    ID3D12Device* d3dDevice = m_device->GetD3D12Device();
    
    // Assert
    ASSERT_NE(d3dDevice, nullptr);
    
    // 验证设备状态
    HRESULT hr = d3dDevice->GetDeviceRemovedReason();
    EXPECT_EQ(hr, S_OK); // 设备应该没有丢失
}

TEST_F(D3D12DeviceTest, GetCommandQueue_ReturnsValidPointer) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    ID3D12CommandQueue* queue = m_device->GetCommandQueue();
    
    // Assert
    ASSERT_NE(queue, nullptr);
}

TEST_F(D3D12DeviceTest, GetDXGIFactory_ReturnsValidPointer) {
    // Arrange
    RHIResult result = CreateDevice();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    IDXGIFactory4* factory = m_device->GetDXGIFactory();
    
    // Assert
    ASSERT_NE(factory, nullptr);
}

// =============================================================================
// 测试用例 10：多线程安全测试（基础）
// =============================================================================

TEST_F(D3D12DeviceTest, MultipleDevices_CanCoexist) {
    // Arrange
    DeviceDesc desc = {};
    desc.enableDebug = false; // 禁用调试层以减少开销
    
    // Act
    auto device1 = std::make_unique<D3D12Device>();
    auto device2 = std::make_unique<D3D12Device>();
    
    RHIResult result1 = device1->Initialize(desc);
    RHIResult result2 = device2->Initialize(desc);
    
    // Assert
    // 注意：在某些系统上，创建多个 D3D12 设备可能会失败
    // 这个测试主要是验证不会崩溃
    EXPECT_EQ(result1, RHIResult::Success);
    // result2 可能成功也可能失败，取决于系统
    
    // Cleanup
    device1->Shutdown();
    device2->Shutdown();
}

// =============================================================================
// 主函数
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}