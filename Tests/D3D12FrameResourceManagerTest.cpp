// =============================================================================
// D3D12FrameResourceManagerTest.cpp - D3D12FrameResourceManager 单元测试
// =============================================================================

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "D3D12Device.h"
#include "D3D12FrameResourceManager.h"

#include <Windows.h>
#include <dxgi1_6.h>

using namespace Engine::RHI::D3D12;
using namespace Engine::RHI;

// =============================================================================
// 测试固件
// =============================================================================

class D3D12FrameResourceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建 D3D12 设备
        m_device = std::make_unique<D3D12Device>();
        
        DeviceDesc desc = {};
        desc.backend = Backend::D3D12;
        desc.enableDebug = true;
        desc.enableValidation = true;
        
        RHIResult result = m_device->Initialize(desc);
        ASSERT_EQ(result, RHIResult::Success) << "D3D12Device 初始化失败";
    }
    
    void TearDown() override {
        // 先清理帧资源管理器
        m_frameResourceManager.reset();
        
        // 再清理设备
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }
    
    // 创建帧资源管理器并验证结果
    RHIResult CreateFrameResourceManager(uint32_t numFrames = 3) {
        m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(
            m_device.get(), numFrames);
        return m_frameResourceManager->Initialize();
    }
    
    std::unique_ptr<D3D12Device> m_device;
    std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;
};

// =============================================================================
// 测试用例 1：初始化测试
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, Initialize_Success) {
    // 创建帧资源管理器
    RHIResult result = CreateFrameResourceManager(3);
    
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_frameResourceManager->IsInitialized());
    EXPECT_EQ(m_frameResourceManager->GetFrameCount(), 3u);
    EXPECT_EQ(m_frameResourceManager->GetCurrentFrameIndex(), 0u);
}

TEST_F(D3D12FrameResourceManagerTest, Initialize_TwoFrameBuffer) {
    // 创建双缓冲帧资源管理器
    RHIResult result = CreateFrameResourceManager(2);
    
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_frameResourceManager->GetFrameCount(), 2u);
}

TEST_F(D3D12FrameResourceManagerTest, Initialize_FourFrameBuffer) {
    // 创建四缓冲帧资源管理器
    RHIResult result = CreateFrameResourceManager(4);
    
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_frameResourceManager->GetFrameCount(), 4u);
}

TEST_F(D3D12FrameResourceManagerTest, Initialize_ZeroFrames_Failure) {
    // 零帧缓冲应该失败
    m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(
        m_device.get(), 0);
    RHIResult result = m_frameResourceManager->Initialize();
    
    EXPECT_NE(result, RHIResult::Success);
}

TEST_F(D3D12FrameResourceManagerTest, Initialize_TooManyFrames_Failure) {
    // 超过 16 帧应该失败
    m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(
        m_device.get(), 17);
    RHIResult result = m_frameResourceManager->Initialize();
    
    EXPECT_NE(result, RHIResult::Success);
}

TEST_F(D3D12FrameResourceManagerTest, Initialize_NullDevice_Failure) {
    // 空设备应该失败
    m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(
        nullptr, 3);
    RHIResult result = m_frameResourceManager->Initialize();
    
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

// =============================================================================
// 测试用例 2：BeginFrame/EndFrame 循环
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, BeginEndFrame_SingleFrame) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 单帧循环
    m_frameResourceManager->BeginFrame();
    EXPECT_EQ(m_frameResourceManager->GetCurrentFrameIndex(), 0u);
    
    m_frameResourceManager->EndFrame();
    EXPECT_EQ(m_frameResourceManager->GetCurrentFrameIndex(), 1u);
}

TEST_F(D3D12FrameResourceManagerTest, BeginEndFrame_MultipleFrames) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 模拟 10 帧循环
    for (int frame = 0; frame < 10; ++frame) {
        uint32_t expectedFrameIndex = frame % 3;
        
        EXPECT_EQ(m_frameResourceManager->GetCurrentFrameIndex(), expectedFrameIndex);
        
        m_frameResourceManager->BeginFrame();
        m_frameResourceManager->EndFrame();
    }
    
    // 最终帧索引应该回到 0（因为 10 % 3 = 1，再 EndFrame 变为 2，再 EndFrame 变为 0）
    // 实际上经过 10 次 EndFrame，帧索引是 10 % 3 = 1
    EXPECT_EQ(m_frameResourceManager->GetCurrentFrameIndex(), 10u % 3u);
}

TEST_F(D3D12FrameResourceManagerTest, BeginEndFrame_FrameIndexWraparound) {
    // 初始化三缓冲
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 验证帧索引循环
    // 帧 0 -> 1 -> 2 -> 0 -> 1 -> 2 -> ...
    for (int i = 0; i < 6; ++i) {
        uint32_t expectedBefore = i % 3;
        EXPECT_EQ(m_frameResourceManager->GetCurrentFrameIndex(), expectedBefore);
        
        m_frameResourceManager->BeginFrame();
        m_frameResourceManager->EndFrame();
    }
}

// =============================================================================
// 测试用例 3：围栏同步
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, FenceValue_Increment) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 初始围栏值应该为 0
    EXPECT_EQ(m_frameResourceManager->GetCurrentFenceValue(), 0u);
    
    // EndFrame 后围栏值应该递增
    m_frameResourceManager->BeginFrame();
    m_frameResourceManager->EndFrame();
    
    // 现在帧索引是 1，帧 0 的围栏值应该为 1
    EXPECT_EQ(m_frameResourceManager->GetFrameFenceValue(0), 1u);
    
    // 继续 EndFrame
    m_frameResourceManager->BeginFrame();
    m_frameResourceManager->EndFrame();
    
    // 帧索引 2，帧 1 的围栏值应该为 2
    EXPECT_EQ(m_frameResourceManager->GetFrameFenceValue(1), 2u);
}

TEST_F(D3D12FrameResourceManagerTest, FenceValue_MultipleFrames) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 执行多帧
    for (int i = 0; i < 5; ++i) {
        m_frameResourceManager->BeginFrame();
        m_frameResourceManager->EndFrame();
    }
    
    // 验证每个帧的围栏值
    // 帧 0: fenceValue = 1 (第一次)
    // 帧 1: fenceValue = 2 (第二次)
    // 帧 2: fenceValue = 3 (第三次)
    // 帧 0: fenceValue = 4 (第四次) - 覆盖第一次的值
    // 帧 1: fenceValue = 5 (第五次) - 覆盖第二次的值
    EXPECT_EQ(m_frameResourceManager->GetFrameFenceValue(0), 4u);
    EXPECT_EQ(m_frameResourceManager->GetFrameFenceValue(1), 5u);
    EXPECT_EQ(m_frameResourceManager->GetFrameFenceValue(2), 3u);
}

TEST_F(D3D12FrameResourceManagerTest, GetCurrentCommandAllocator_NotNull) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 命令分配器应该有效
    ID3D12CommandAllocator* allocator = m_frameResourceManager->GetCurrentCommandAllocator();
    EXPECT_NE(allocator, nullptr);
}

TEST_F(D3D12FrameResourceManagerTest, GetCurrentFence_NotNull) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 围栏应该有效
    ID3D12Fence* fence = m_frameResourceManager->GetCurrentFence();
    EXPECT_NE(fence, nullptr);
}

// =============================================================================
// 测试用例 4：资源注册和清理
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, RegisterUploadBuffer) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建测试上传缓冲区
    ComPtr<ID3D12Resource> uploadBuffer;
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = 1024;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );
    
    ASSERT_EQ(hr, S_OK) << "创建上传缓冲区失败";
    
    // 注册上传缓冲区
    m_frameResourceManager->RegisterUploadBuffer(uploadBuffer.Get());
    
    // 缓冲区引用计数应该增加
    // ComPtr 持有一个引用，RegisterUploadBuffer 增加一个引用
    // 所以引用计数应该 >= 2
}

TEST_F(D3D12FrameResourceManagerTest, RegisterTemporaryResource) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建测试资源
    ComPtr<ID3D12Resource> tempResource;
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = 512;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&tempResource)
    );
    
    ASSERT_EQ(hr, S_OK) << "创建临时资源失败";
    
    // 注册临时资源
    m_frameResourceManager->RegisterTemporaryResource(tempResource.Get());
}

TEST_F(D3D12FrameResourceManagerTest, DelayedDeleteResource) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建测试资源
    ComPtr<ID3D12Resource> resource;
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = 256;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&resource)
    );
    
    ASSERT_EQ(hr, S_OK) << "创建资源失败";
    
    // 增加引用计数，以便 DelayedDeleteResource 接管
    resource->AddRef();
    
    // 延迟删除
    m_frameResourceManager->DelayedDeleteResource(resource.Get());
    
    // 资源将在下一帧 BeginFrame 时被释放
    // 注意：此时 ComPtr 仍然持有引用，但 DelayedDeleteResource 会释放一个
    
    // 执行帧循环以触发清理
    m_frameResourceManager->EndFrame();
    m_frameResourceManager->BeginFrame();  // 这将清理延迟删除列表
}

TEST_F(D3D12FrameResourceManagerTest, ResourceCleanup_OnFrameWraparound) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建多个资源
    ComPtr<ID3D12Resource> resources[3];
    
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = 128;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    for (int i = 0; i < 3; ++i) {
        HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resources[i])
        );
        ASSERT_EQ(hr, S_OK);
    }
    
    // 在每一帧注册一个资源
    for (int i = 0; i < 3; ++i) {
        m_frameResourceManager->BeginFrame();
        m_frameResourceManager->RegisterUploadBuffer(resources[i].Get());
        m_frameResourceManager->EndFrame();
    }
    
    // 再次循环，此时第一帧的资源应该被清理
    m_frameResourceManager->BeginFrame();
    
    // 资源应该仍然有效（ComPtr 持有引用）
    EXPECT_NE(resources[0].Get(), nullptr);
}

// =============================================================================
// 测试用例 5：命令分配器重置
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, CommandAllocator_Reset) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 获取第一帧的命令分配器
    ID3D12CommandAllocator* allocator1 = m_frameResourceManager->GetCurrentCommandAllocator();
    EXPECT_NE(allocator1, nullptr);
    
    // 结束帧
    m_frameResourceManager->BeginFrame();
    m_frameResourceManager->EndFrame();
    
    // 获取第二帧的命令分配器
    ID3D12CommandAllocator* allocator2 = m_frameResourceManager->GetCurrentCommandAllocator();
    EXPECT_NE(allocator2, nullptr);
    
    // 应该是不同的分配器
    EXPECT_NE(allocator1, allocator2);
    
    // 继续 EndFrame
    m_frameResourceManager->BeginFrame();
    m_frameResourceManager->EndFrame();
    
    // 第三帧
    ID3D12CommandAllocator* allocator3 = m_frameResourceManager->GetCurrentCommandAllocator();
    EXPECT_NE(allocator3, nullptr);
    EXPECT_NE(allocator2, allocator3);
    
    // 继续 EndFrame，回到第一帧
    m_frameResourceManager->BeginFrame();
    m_frameResourceManager->EndFrame();
    
    // 应该回到第一帧的分配器
    ID3D12CommandAllocator* allocator4 = m_frameResourceManager->GetCurrentCommandAllocator();
    EXPECT_EQ(allocator1, allocator4);
}

TEST_F(D3D12FrameResourceManagerTest, CommandAllocator_ValidAfterReset) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 获取并使用命令分配器
    ID3D12CommandAllocator* allocator = m_frameResourceManager->GetCurrentCommandAllocator();
    
    // 创建命令列表来使用分配器
    ComPtr<ID3D12GraphicsCommandList> commandList;
    HRESULT hr = m_device->GetD3D12Device()->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator,
        nullptr,
        IID_PPV_ARGS(&commandList)
    );
    
    ASSERT_EQ(hr, S_OK);
    
    // 关闭命令列表
    commandList->Close();
    
    // 结束帧
    m_frameResourceManager->EndFrame();
    
    // 开始下一帧（会重置分配器）
    m_frameResourceManager->BeginFrame();
    
    // 分配器应该仍然有效
    EXPECT_NE(allocator, nullptr);
    
    // 重置命令列表以验证分配器可用
    hr = commandList->Reset(allocator, nullptr);
    EXPECT_EQ(hr, S_OK);
    
    commandList->Close();
}

// =============================================================================
// 测试用例 6：帧状态查询
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, IsFrameComplete_InitialState) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 所有帧初始状态应该是已完成（围栏值为 0）
    EXPECT_TRUE(m_frameResourceManager->IsFrameComplete(0));
    EXPECT_TRUE(m_frameResourceManager->IsFrameComplete(1));
    EXPECT_TRUE(m_frameResourceManager->IsFrameComplete(2));
}

TEST_F(D3D12FrameResourceManagerTest, IsFrameComplete_InvalidIndex) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 无效索引应该返回 false
    EXPECT_FALSE(m_frameResourceManager->IsFrameComplete(3));
    EXPECT_FALSE(m_frameResourceManager->IsFrameComplete(100));
}

TEST_F(D3D12FrameResourceManagerTest, GetFrameFenceValue_InvalidIndex) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 无效索引应该返回 0
    EXPECT_EQ(m_frameResourceManager->GetFrameFenceValue(3), 0u);
    EXPECT_EQ(m_frameResourceManager->GetFrameFenceValue(100), 0u);
}

// =============================================================================
// 测试用例 7：关闭和重新初始化
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, Shutdown) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_frameResourceManager->IsInitialized());
    
    // 关闭
    m_frameResourceManager->Shutdown();
    EXPECT_FALSE(m_frameResourceManager->IsInitialized());
}

TEST_F(D3D12FrameResourceManagerTest, Reinitialize) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 执行一些帧循环
    for (int i = 0; i < 5; ++i) {
        m_frameResourceManager->BeginFrame();
        m_frameResourceManager->EndFrame();
    }
    
    // 关闭
    m_frameResourceManager->Shutdown();
    EXPECT_FALSE(m_frameResourceManager->IsInitialized());
    
    // 重新初始化
    result = m_frameResourceManager->Initialize();
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_frameResourceManager->IsInitialized());
    EXPECT_EQ(m_frameResourceManager->GetCurrentFrameIndex(), 0u);
    EXPECT_EQ(m_frameResourceManager->GetCurrentFenceValue(), 0u);
}

// =============================================================================
// 测试用例 8：边界条件
// =============================================================================

TEST_F(D3D12FrameResourceManagerTest, BeginFrame_WithoutInitialize) {
    // 不初始化直接调用
    m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(
        m_device.get(), 3);
    
    // 不应该崩溃
    m_frameResourceManager->BeginFrame();
    EXPECT_FALSE(m_frameResourceManager->IsInitialized());
}

TEST_F(D3D12FrameResourceManagerTest, EndFrame_WithoutInitialize) {
    // 不初始化直接调用
    m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(
        m_device.get(), 3);
    
    // 不应该崩溃
    m_frameResourceManager->EndFrame();
    EXPECT_FALSE(m_frameResourceManager->IsInitialized());
}

TEST_F(D3D12FrameResourceManagerTest, RegisterResource_WithoutInitialize) {
    // 不初始化直接调用
    m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(
        m_device.get(), 3);
    
    // 创建测试资源
    ComPtr<ID3D12Resource> uploadBuffer;
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = 256;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );
    
    if (SUCCEEDED(hr)) {
        // 不应该崩溃
        m_frameResourceManager->RegisterUploadBuffer(uploadBuffer.Get());
        m_frameResourceManager->RegisterTemporaryResource(uploadBuffer.Get());
        m_frameResourceManager->DelayedDeleteResource(uploadBuffer.Get());
    }
}

TEST_F(D3D12FrameResourceManagerTest, WaitForFrame_InvalidIndex) {
    // 初始化
    RHIResult result = CreateFrameResourceManager(3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 无效索引不应该崩溃
    m_frameResourceManager->WaitForFrame(100);
}

// =============================================================================
// 主函数
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}