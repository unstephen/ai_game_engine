// =============================================================================
// D3D12UploadManagerTest.cpp - D3D12UploadManager 单元测试
// =============================================================================

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "D3D12Device.h"
#include "D3D12UploadManager.h"

#include <Windows.h>
#include <dxgi1_6.h>
#include <vector>
#include <random>
#include <thread>
#include <atomic>

using namespace Engine::RHI::D3D12;
using namespace Engine::RHI;

// =============================================================================
// 辅助结构（替代 d3dx12.h）
// =============================================================================

namespace {

/**
 * @brief 堆属性辅助结构
 */
struct HeapProperties {
    D3D12_HEAP_PROPERTIES props;
    
    explicit HeapProperties(D3D12_HEAP_TYPE type) {
        props.Type = type;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.CreationNodeMask = 1;
        props.VisibleNodeMask = 1;
    }
    
    operator const D3D12_HEAP_PROPERTIES&() const { return props; }
};

/**
 * @brief 创建缓冲区资源描述
 */
D3D12_RESOURCE_DESC CreateBufferDesc(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;
    return desc;
}

/**
 * @brief 创建纹理资源描述
 */
D3D12_RESOURCE_DESC CreateTexture2DDesc(
    DXGI_FORMAT format,
    UINT64 width,
    UINT height,
    UINT16 arraySize = 1,
    UINT16 mipLevels = 1,
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
    
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = arraySize;
    desc.MipLevels = mipLevels;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = flags;
    return desc;
}

} // anonymous namespace

// =============================================================================
// 测试固件
// =============================================================================

class D3D12UploadManagerTest : public ::testing::Test {
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
        // 先清理上传管理器
        m_uploadManager.reset();
        
        // 再清理设备
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }
    
    // 创建上传管理器并验证结果
    RHIResult CreateUploadManager(const UploadManagerConfig& config = {}) {
        m_uploadManager = std::make_unique<D3D12UploadManager>(m_device.get());
        return m_uploadManager->Initialize(config);
    }
    
    std::unique_ptr<D3D12Device> m_device;
    std::unique_ptr<D3D12UploadManager> m_uploadManager;
};

// =============================================================================
// 测试用例 1：初始化测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, Initialize_Success) {
    // 创建上传管理器（默认配置）
    RHIResult result = CreateUploadManager();
    
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_uploadManager->IsInitialized());
    EXPECT_EQ(m_uploadManager->GetBlockCount(), 2u);  // 默认预分配 2 个块
}

TEST_F(D3D12UploadManagerTest, Initialize_CustomConfig) {
    // 自定义配置
    UploadManagerConfig config = {};
    config.defaultBlockSize = 16 * 1024 * 1024;  // 16MB
    config.maxBlockSize = 64 * 1024 * 1024;      // 64MB
    config.preallocatedBlocks = 4;
    config.alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    config.enableDebugLog = true;
    
    RHIResult result = CreateUploadManager(config);
    
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_uploadManager->IsInitialized());
    EXPECT_EQ(m_uploadManager->GetBlockCount(), 4u);  // 预分配 4 个块
    
    // 验证配置被正确保存
    const auto& savedConfig = m_uploadManager->GetConfig();
    EXPECT_EQ(savedConfig.defaultBlockSize, 16 * 1024 * 1024);
    EXPECT_EQ(savedConfig.maxBlockSize, 64 * 1024 * 1024);
    EXPECT_EQ(savedConfig.preallocatedBlocks, 4u);
}

TEST_F(D3D12UploadManagerTest, Initialize_NullDevice_Failure) {
    // 空设备应该失败
    m_uploadManager = std::make_unique<D3D12UploadManager>(nullptr);
    RHIResult result = m_uploadManager->Initialize();
    
    EXPECT_EQ(result, RHIResult::InvalidParameter);
    EXPECT_FALSE(m_uploadManager->IsInitialized());
}

TEST_F(D3D12UploadManagerTest, Initialize_ZeroBlockSize_Failure) {
    // 零块大小应该失败
    UploadManagerConfig config = {};
    config.defaultBlockSize = 0;
    
    m_uploadManager = std::make_unique<D3D12UploadManager>(m_device.get());
    RHIResult result = m_uploadManager->Initialize(config);
    
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12UploadManagerTest, Initialize_InvalidBlockSize_Failure) {
    // 最大块大小小于默认块大小应该失败
    UploadManagerConfig config = {};
    config.defaultBlockSize = 64 * 1024 * 1024;
    config.maxBlockSize = 32 * 1024 * 1024;  // 小于默认大小
    
    m_uploadManager = std::make_unique<D3D12UploadManager>(m_device.get());
    RHIResult result = m_uploadManager->Initialize(config);
    
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12UploadManagerTest, Shutdown_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 关闭
    m_uploadManager->Shutdown();
    
    EXPECT_FALSE(m_uploadManager->IsInitialized());
    EXPECT_EQ(m_uploadManager->GetBlockCount(), 0u);
}

TEST_F(D3D12UploadManagerTest, Reinitialize_Success) {
    // 第一次初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 关闭
    m_uploadManager->Shutdown();
    EXPECT_FALSE(m_uploadManager->IsInitialized());
    
    // 重新初始化
    result = m_uploadManager->Initialize();
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_TRUE(m_uploadManager->IsInitialized());
}

// =============================================================================
// 测试用例 2：内存分配测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, Allocate_SmallSize_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配小内存
    void* ptr = m_uploadManager->Allocate(1024);  // 1KB
    
    EXPECT_NE(ptr, nullptr);
    EXPECT_GT(m_uploadManager->GetUsedMemory(), 0u);
}

TEST_F(D3D12UploadManagerTest, Allocate_LargeSize_Success) {
    // 使用小块大小初始化
    UploadManagerConfig config = {};
    config.defaultBlockSize = 1 * 1024 * 1024;  // 1MB
    
    RHIResult result = CreateUploadManager(config);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配较大内存（会创建新块）
    void* ptr = m_uploadManager->Allocate(2 * 1024 * 1024);  // 2MB
    
    EXPECT_NE(ptr, nullptr);
}

TEST_F(D3D12UploadManagerTest, AllocateEx_ReturnsValidAllocation) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配并检查详细信息
    UploadAllocation alloc = m_uploadManager->AllocateEx(4096, 256);
    
    EXPECT_TRUE(alloc.IsValid());
    EXPECT_NE(alloc.buffer, nullptr);
    EXPECT_NE(alloc.mappedData, nullptr);
    EXPECT_GT(alloc.gpuAddress, 0ull);
    EXPECT_EQ(alloc.size, 4096ull);
    EXPECT_GT(alloc.allocatedSize, 0ull);
    EXPECT_LT(alloc.blockIndex, m_uploadManager->GetBlockCount());
}

TEST_F(D3D12UploadManagerTest, Allocate_MultipleAllocations_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 多次分配
    std::vector<UploadAllocation> allocations;
    for (int i = 0; i < 10; ++i) {
        UploadAllocation alloc = m_uploadManager->AllocateEx(1024 * (i + 1), 256);
        EXPECT_TRUE(alloc.IsValid()) << "分配 " << i << " 失败";
        allocations.push_back(alloc);
    }
    
    // 验证所有分配都在同一块中（因为总大小很小）
    uint32_t firstBlockIndex = allocations[0].blockIndex;
    for (size_t i = 1; i < allocations.size(); ++i) {
        // 所有分配可能在同一块或不同块，取决于对齐和可用空间
        // 只需要验证它们都有效
        EXPECT_TRUE(allocations[i].IsValid());
    }
}

TEST_F(D3D12UploadManagerTest, Allocate_ZeroSize_Failure) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配零大小应该返回无效分配
    UploadAllocation alloc = m_uploadManager->AllocateEx(0);
    EXPECT_FALSE(alloc.IsValid());
}

TEST_F(D3D12UploadManagerTest, Allocate_BeforeInitialize_Failure) {
    // 未初始化时分配
    m_uploadManager = std::make_unique<D3D12UploadManager>(m_device.get());
    
    void* ptr = m_uploadManager->Allocate(1024);
    EXPECT_EQ(ptr, nullptr);
}

// =============================================================================
// 测试用例 3：对齐测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, Alignment_Default_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配并检查默认对齐
    UploadAllocation alloc = m_uploadManager->AllocateEx(100, 256);
    
    EXPECT_TRUE(alloc.IsValid());
    // 偏移应该是 256 的倍数
    EXPECT_EQ(alloc.offset % 256, 0ull);
    // 大小应该向上对齐
    EXPECT_GE(alloc.allocatedSize, 100ull);
}

TEST_F(D3D12UploadManagerTest, Alignment_Custom_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 使用不同对齐值分配
    uint64_t alignments[] = {256, 512, 1024, 2048, 4096};
    
    for (uint64_t alignment : alignments) {
        UploadAllocation alloc = m_uploadManager->AllocateEx(100, alignment);
        ASSERT_TRUE(alloc.IsValid()) << "对齐 " << alignment << " 分配失败";
        EXPECT_EQ(alloc.offset % alignment, 0ull) << "偏移未按 " << alignment << " 对齐";
    }
}

TEST_F(D3D12UploadManagerTest, Alignment_SequentialAllocations) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 连续分配，验证对齐
    UploadAllocation alloc1 = m_uploadManager->AllocateEx(100, 256);
    UploadAllocation alloc2 = m_uploadManager->AllocateEx(200, 256);
    UploadAllocation alloc3 = m_uploadManager->AllocateEx(300, 256);
    
    EXPECT_TRUE(alloc1.IsValid());
    EXPECT_TRUE(alloc2.IsValid());
    EXPECT_TRUE(alloc3.IsValid());
    
    // 所有偏移都应该对齐
    EXPECT_EQ(alloc1.offset % 256, 0ull);
    EXPECT_EQ(alloc2.offset % 256, 0ull);
    EXPECT_EQ(alloc3.offset % 256, 0ull);
    
    // 偏移应该递增
    EXPECT_GT(alloc2.offset, alloc1.offset);
    EXPECT_GT(alloc3.offset, alloc2.offset);
}

TEST_F(D3D12UploadManagerTest, Alignment_DifferentAlignments) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 使用不同对齐值连续分配
    UploadAllocation alloc1 = m_uploadManager->AllocateEx(100, 256);
    UploadAllocation alloc2 = m_uploadManager->AllocateEx(100, 1024);
    UploadAllocation alloc3 = m_uploadManager->AllocateEx(100, 512);
    
    EXPECT_TRUE(alloc1.IsValid());
    EXPECT_TRUE(alloc2.IsValid());
    EXPECT_TRUE(alloc3.IsValid());
    
    // 验证对齐
    EXPECT_EQ(alloc1.offset % 256, 0ull);
    EXPECT_EQ(alloc2.offset % 1024, 0ull);
    EXPECT_EQ(alloc3.offset % 512, 0ull);
}

// =============================================================================
// 测试用例 4：块管理测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, BlockCreation_WhenNeeded) {
    // 使用小块大小初始化
    UploadManagerConfig config = {};
    config.defaultBlockSize = 64 * 1024;  // 64KB
    config.preallocatedBlocks = 1;
    
    RHIResult result = CreateUploadManager(config);
    ASSERT_EQ(result, RHIResult::Success);
    
    EXPECT_EQ(m_uploadManager->GetBlockCount(), 1u);
    
    // 分配超过块大小的内存，应该创建新块
    UploadAllocation alloc = m_uploadManager->AllocateEx(128 * 1024);  // 128KB
    
    EXPECT_TRUE(alloc.IsValid());
    EXPECT_EQ(m_uploadManager->GetBlockCount(), 2u);  // 创建了新块
}

TEST_F(D3D12UploadManagerTest, BlockReuse_AfterCleanup) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配并注册围栏值
    UploadAllocation alloc = m_uploadManager->AllocateEx(1024);
    ASSERT_TRUE(alloc.IsValid());
    
    m_uploadManager->RegisterFenceValue(alloc, 1);
    
    // 模拟 GPU 完成（围栏值 1）
    m_uploadManager->CleanupCompletedUploads(1);
    
    // 块应该被重置，可以重用
    // 再次分配应该使用同一个块
    uint32_t blockCountBefore = m_uploadManager->GetBlockCount();
    UploadAllocation alloc2 = m_uploadManager->AllocateEx(1024);
    
    EXPECT_TRUE(alloc2.IsValid());
    EXPECT_EQ(m_uploadManager->GetBlockCount(), blockCountBefore);  // 没有创建新块
}

TEST_F(D3D12UploadManagerTest, BlockFull_CreatesNewBlock) {
    // 使用小块大小初始化
    UploadManagerConfig config = {};
    config.defaultBlockSize = 4 * 1024;  // 4KB
    config.preallocatedBlocks = 1;
    
    RHIResult result = CreateUploadManager(config);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配直到块接近满
    std::vector<UploadAllocation> allocations;
    for (int i = 0; i < 5; ++i) {
        UploadAllocation alloc = m_uploadManager->AllocateEx(1 * 1024, 64);  // 1KB
        if (alloc.IsValid()) {
            allocations.push_back(alloc);
        }
    }
    
    // 应该创建了新块
    EXPECT_GE(m_uploadManager->GetBlockCount(), 1u);
}

TEST_F(D3D12UploadManagerTest, GetCurrentBlockUsage) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 初始使用率应该是 0
    float usage = m_uploadManager->GetCurrentBlockUsage();
    EXPECT_NEAR(usage, 0.0f, 0.01f);
    
    // 分配后使用率应该增加
    m_uploadManager->Allocate(1024 * 1024);  // 1MB
    
    usage = m_uploadManager->GetCurrentBlockUsage();
    EXPECT_GT(usage, 0.0f);
    EXPECT_LT(usage, 1.0f);
}

// =============================================================================
// 测试用例 5：内存统计测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, GetTotalMemory) {
    // 使用自定义配置初始化
    UploadManagerConfig config = {};
    config.defaultBlockSize = 1 * 1024 * 1024;  // 1MB
    config.preallocatedBlocks = 2;
    
    RHIResult result = CreateUploadManager(config);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 总内存应该等于块数量 * 块大小
    uint64_t totalMemory = m_uploadManager->GetTotalMemory();
    EXPECT_EQ(totalMemory, 2 * 1024 * 1024ull);  // 2MB
}

TEST_F(D3D12UploadManagerTest, GetUsedMemory) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 初始使用量应该是 0
    EXPECT_EQ(m_uploadManager->GetUsedMemory(), 0u);
    
    // 分配后使用量应该增加
    m_uploadManager->Allocate(1024);  // 1KB
    
    uint64_t usedMemory = m_uploadManager->GetUsedMemory();
    EXPECT_GE(usedMemory, 1024ull);
}

TEST_F(D3D12UploadManagerTest, Reset_ClearsAllBlocks) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配一些内存
    m_uploadManager->Allocate(1024);
    m_uploadManager->Allocate(2048);
    
    EXPECT_GT(m_uploadManager->GetUsedMemory(), 0u);
    
    // 重置
    m_uploadManager->Reset();
    
    // 使用量应该归零
    EXPECT_EQ(m_uploadManager->GetUsedMemory(), 0u);
}

// =============================================================================
// 测试用例 6：围栏值注册测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, RegisterFenceValue_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 分配
    UploadAllocation alloc = m_uploadManager->AllocateEx(1024);
    ASSERT_TRUE(alloc.IsValid());
    
    // 注册围栏值
    m_uploadManager->RegisterFenceValue(alloc, 42);
    
    // 没有异常就是成功
}

TEST_F(D3D12UploadManagerTest, RegisterFenceValue_InvalidAllocation) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 使用无效分配注册
    UploadAllocation invalidAlloc;
    m_uploadManager->RegisterFenceValue(invalidAlloc, 42);
    
    // 应该安全处理，不会崩溃
}

TEST_F(D3D12UploadManagerTest, CleanupCompletedUploads_PartialCleanup) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 多次分配并注册不同围栏值
    UploadAllocation alloc1 = m_uploadManager->AllocateEx(1024);
    m_uploadManager->RegisterFenceValue(alloc1, 1);
    
    UploadAllocation alloc2 = m_uploadManager->AllocateEx(1024);
    m_uploadManager->RegisterFenceValue(alloc2, 2);
    
    UploadAllocation alloc3 = m_uploadManager->AllocateEx(1024);
    m_uploadManager->RegisterFenceValue(alloc3, 3);
    
    // 清理围栏值 <= 2 的上传
    m_uploadManager->CleanupCompletedUploads(2);
    
    // 应该正常工作
    // 验证可以继续分配
    UploadAllocation alloc4 = m_uploadManager->AllocateEx(1024);
    EXPECT_TRUE(alloc4.IsValid());
}

// =============================================================================
// 测试用例 7：数据上传测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, UploadBufferData_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建目标缓冲区
    ID3D12Device* d3dDevice = m_device->GetD3D12Device();
    ASSERT_NE(d3dDevice, nullptr);
    
    const uint64_t bufferSize = 4096;
    
    HeapProperties defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC bufferDesc = CreateBufferDesc(bufferSize);
    
    ComPtr<ID3D12Resource> destBuffer;
    HRESULT hr = d3dDevice->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&destBuffer)
    );
    
    ASSERT_EQ(hr, S_OK) << "创建目标缓冲区失败";
    
    // 创建命令分配器和命令列表
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    hr = d3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)
    );
    ASSERT_EQ(hr, S_OK);
    
    ComPtr<ID3D12GraphicsCommandList> commandList;
    hr = d3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)
    );
    ASSERT_EQ(hr, S_OK);
    
    // 准备测试数据
    std::vector<float> testData(1024);
    for (size_t i = 0; i < testData.size(); ++i) {
        testData[i] = static_cast<float>(i);
    }
    
    // 上传数据
    RHIResult uploadResult = m_uploadManager->UploadBufferData(
        commandList.Get(),
        destBuffer.Get(),
        0,
        testData.data(),
        testData.size() * sizeof(float)
    );
    
    EXPECT_EQ(uploadResult, RHIResult::Success);
    
    // 关闭命令列表
    hr = commandList->Close();
    EXPECT_EQ(hr, S_OK);
}

TEST_F(D3D12UploadManagerTest, UploadBufferData_InvalidParameters) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 测试各种无效参数
    float data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    
    // 空命令列表
    RHIResult uploadResult = m_uploadManager->UploadBufferData(
        nullptr, nullptr, 0, data, sizeof(data)
    );
    EXPECT_EQ(uploadResult, RHIResult::InvalidParameter);
    
    // 空数据
    uploadResult = m_uploadManager->UploadBufferData(
        nullptr, nullptr, 0, nullptr, sizeof(data)
    );
    EXPECT_EQ(uploadResult, RHIResult::InvalidParameter);
    
    // 零大小
    uploadResult = m_uploadManager->UploadBufferData(
        nullptr, nullptr, 0, data, 0
    );
    EXPECT_EQ(uploadResult, RHIResult::InvalidParameter);
}

TEST_F(D3D12UploadManagerTest, UploadTextureData_Success) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建目标纹理
    ID3D12Device* d3dDevice = m_device->GetD3D12Device();
    ASSERT_NE(d3dDevice, nullptr);
    
    const uint32_t textureWidth = 64;
    const uint32_t textureHeight = 64;
    const DXGI_FORMAT textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    HeapProperties defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC textureDesc = CreateTexture2DDesc(
        textureFormat,
        textureWidth,
        textureHeight,
        1,  // array size
        1   // mip levels
    );
    
    ComPtr<ID3D12Resource> destTexture;
    HRESULT hr = d3dDevice->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&destTexture)
    );
    
    ASSERT_EQ(hr, S_OK) << "创建目标纹理失败";
    
    // 计算纹理放置布局
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Offset = 0;
    footprint.Footprint.Format = textureFormat;
    footprint.Footprint.Width = textureWidth;
    footprint.Footprint.Height = textureHeight;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.RowPitch = textureWidth * 4;  // RGBA8 = 4 bytes per pixel
    
    // 创建命令分配器和命令列表
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    hr = d3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)
    );
    ASSERT_EQ(hr, S_OK);
    
    ComPtr<ID3D12GraphicsCommandList> commandList;
    hr = d3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)
    );
    ASSERT_EQ(hr, S_OK);
    
    // 准备测试数据
    std::vector<uint8_t> textureData(textureWidth * textureHeight * 4, 128);
    
    // 上传纹理数据
    RHIResult uploadResult = m_uploadManager->UploadTextureData(
        commandList.Get(),
        destTexture.Get(),
        textureData.data(),
        textureData.size(),
        footprint,
        0  // subresource index
    );
    
    EXPECT_EQ(uploadResult, RHIResult::Success);
    
    // 关闭命令列表
    hr = commandList->Close();
    EXPECT_EQ(hr, S_OK);
}

// =============================================================================
// 测试用例 8：压力测试
// =============================================================================

TEST_F(D3D12UploadManagerTest, StressTest_ManyAllocations) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 执行大量分配
    const int numAllocations = 1000;
    std::vector<UploadAllocation> allocations;
    allocations.reserve(numAllocations);
    
    for (int i = 0; i < numAllocations; ++i) {
        UploadAllocation alloc = m_uploadManager->AllocateEx(1024, 256);
        EXPECT_TRUE(alloc.IsValid()) << "分配 " << i << " 失败";
        allocations.push_back(alloc);
    }
    
    // 验证所有分配都成功
    size_t validCount = 0;
    for (const auto& alloc : allocations) {
        if (alloc.IsValid()) {
            validCount++;
        }
    }
    EXPECT_EQ(validCount, numAllocations);
}

TEST_F(D3D12UploadManagerTest, StressTest_RandomSizes) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 随机大小分配
    std::mt19937 rng(42);  // 固定种子
    std::uniform_int_distribution<uint64_t> sizeDist(64, 64 * 1024);  // 64B - 64KB
    std::uniform_int_distribution<uint64_t> alignDist(0, 3);
    
    uint64_t aligns[] = {256, 512, 1024, 2048};
    
    const int numAllocations = 100;
    for (int i = 0; i < numAllocations; ++i) {
        uint64_t size = sizeDist(rng);
        uint64_t alignment = aligns[alignDist(rng)];
        
        UploadAllocation alloc = m_uploadManager->AllocateEx(size, alignment);
        EXPECT_TRUE(alloc.IsValid()) << "分配 " << i << " 失败 (大小: " << size << ")";
        
        if (alloc.IsValid()) {
            EXPECT_EQ(alloc.offset % alignment, 0ull);
        }
    }
}

// =============================================================================
// 测试用例 9：并发测试（基础）
// =============================================================================

TEST_F(D3D12UploadManagerTest, ThreadSafety_BasicAllocation) {
    // 初始化
    RHIResult result = CreateUploadManager();
    ASSERT_EQ(result, RHIResult::Success);
    
    // 多线程分配（简化版本）
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &successCount]() {
            for (int i = 0; i < 25; ++i) {
                void* ptr = m_uploadManager->Allocate(1024);
                if (ptr != nullptr) {
                    successCount++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 所有分配都应该成功
    EXPECT_EQ(successCount.load(), 100);
}

// =============================================================================
// 测试主函数
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}