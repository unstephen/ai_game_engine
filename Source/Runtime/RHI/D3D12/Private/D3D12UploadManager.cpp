// =============================================================================
// D3D12UploadManager.cpp - D3D12 上传管理器实现
// =============================================================================

#include "D3D12UploadManager.h"

#if ENGINE_RHI_D3D12

#include <algorithm>
#include <sstream>

namespace Engine::RHI::D3D12
{

// =============================================================================
// 辅助结构（替代 d3dx12.h）
// =============================================================================

namespace
{

/**
 * @brief 堆属性辅助结构
 */
struct HeapProperties
{
    D3D12_HEAP_PROPERTIES props;

    explicit HeapProperties(D3D12_HEAP_TYPE type)
    {
        props.Type                 = type;
        props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.CreationNodeMask     = 1;
        props.VisibleNodeMask      = 1;
    }

    operator const D3D12_HEAP_PROPERTIES&() const { return props; }
};

/**
 * @brief 创建缓冲区资源描述
 */
D3D12_RESOURCE_DESC CreateBufferDesc(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment           = 0;
    desc.Width               = width;
    desc.Height              = 1;
    desc.DepthOrArraySize    = 1;
    desc.MipLevels           = 1;
    desc.Format              = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count    = 1;
    desc.SampleDesc.Quality  = 0;
    desc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags               = flags;
    return desc;
}

/**
 * @brief 资源范围辅助结构（用于 Map/Unmap）
 */
struct ResourceRange
{
    SIZE_T Begin;
    SIZE_T End;

    ResourceRange(SIZE_T begin, SIZE_T end) : Begin(begin), End(end) {}
    operator const D3D12_RANGE*() const
    {
        static D3D12_RANGE range;
        range.Begin = Begin;
        range.End   = End;
        return &range;
    }
};

} // anonymous namespace

// =============================================================================
// 构造/析构
// =============================================================================

D3D12UploadManager::D3D12UploadManager(D3D12Device* device) : m_device(device)
{
    RHI_LOG_DEBUG("D3D12UploadManager: 构造函数");
}

D3D12UploadManager::~D3D12UploadManager()
{
    RHI_LOG_DEBUG("D3D12UploadManager: 析构函数");
    Shutdown();
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12UploadManager::Initialize(const UploadManagerConfig& config)
{
    RHI_LOG_INFO("D3D12UploadManager: 开始初始化...");

    // 参数验证
    if (!m_device)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = m_device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12UploadManager: D3D12 设备未初始化");
        return RHIResult::InvalidParameter;
    }

    // 验证配置参数
    if (config.defaultBlockSize == 0)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 默认块大小不能为 0");
        return RHIResult::InvalidParameter;
    }

    if (config.maxBlockSize < config.defaultBlockSize)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 最大块大小不能小于默认块大小");
        return RHIResult::InvalidParameter;
    }

    // 如果已经初始化，先关闭
    if (m_isInitialized)
    {
        RHI_LOG_WARNING("D3D12UploadManager: 已初始化，重新初始化");
        Shutdown();
    }

    // 保存配置
    m_config = config;

    // 确保对齐满足 D3D12 要求（至少 64KB）
    if (m_config.alignment < D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
    {
        m_config.alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        RHI_LOG_DEBUG("D3D12UploadManager: 对齐调整为 %llu 字节", m_config.alignment);
    }

    // 预分配块
    m_blocks.clear();
    m_blocks.reserve(m_config.preallocatedBlocks + 4); // 预留额外空间

    for (uint32_t i = 0; i < m_config.preallocatedBlocks; ++i)
    {
        uint32_t blockIndex = CreateNewBlock(m_config.defaultBlockSize);
        if (blockIndex == UINT32_MAX)
        {
            RHI_LOG_ERROR("D3D12UploadManager: 预分配块 %u 失败", i);
            Shutdown();
            return RHIResult::OutOfMemory;
        }
        RHI_LOG_DEBUG("D3D12UploadManager: 预分配块 %u 创建成功", i);
    }

    // 设置当前块
    m_currentBlockIndex = 0;
    m_pendingUploads.clear();

    // 重置统计信息
    m_totalAllocations    = 0;
    m_totalBytesAllocated = 0;

    m_isInitialized = true;

    RHI_LOG_INFO("D3D12UploadManager: 初始化完成");
    RHI_LOG_INFO("  - 默认块大小: %.2f MB", m_config.defaultBlockSize / (1024.0 * 1024.0));
    RHI_LOG_INFO("  - 最大块大小: %.2f MB", m_config.maxBlockSize / (1024.0 * 1024.0));
    RHI_LOG_INFO("  - 预分配块数: %u", m_config.preallocatedBlocks);
    RHI_LOG_INFO("  - 内存对齐: %llu 字节", m_config.alignment);

    return RHIResult::Success;
}

void D3D12UploadManager::Shutdown()
{
    RHI_LOG_INFO("D3D12UploadManager: 关闭...");

    std::lock_guard<std::mutex> lock(m_mutex);

    // 解除所有块的映射
    for (auto& block : m_blocks)
    {
        if (block.buffer && block.mappedPtr)
        {
            block.buffer->Unmap(0, nullptr);
            block.mappedPtr = nullptr;
        }
        block.buffer.Reset();
    }

    m_blocks.clear();
    m_pendingUploads.clear();
    m_currentBlockIndex = 0;
    m_isInitialized     = false;

    RHI_LOG_INFO("D3D12UploadManager: 关闭完成");
}

// =============================================================================
// IUploadManager 接口实现
// =============================================================================

void* D3D12UploadManager::Allocate(uint64_t size, uint64_t alignment)
{
    UploadAllocation alloc = AllocateEx(size, alignment);
    return alloc.mappedData;
}

void D3D12UploadManager::Submit()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isInitialized)
    {
        RHI_LOG_WARNING("D3D12UploadManager: 未初始化，跳过提交");
        return;
    }

    // 标记当前块为已满（如果使用了超过 95%）
    if (m_currentBlockIndex < m_blocks.size())
    {
        UploadBlock& currentBlock = m_blocks[m_currentBlockIndex];
        if (currentBlock.used >= currentBlock.capacity * 0.95)
        {
            currentBlock.isFull = true;
            RHI_LOG_DEBUG("D3D12UploadManager: 块 %u 标记为已满", m_currentBlockIndex);
        }
    }

    RHI_LOG_DEBUG("D3D12UploadManager: 提交完成");
}

void D3D12UploadManager::WaitForCompletion()
{
    // 等待完成由外部围栏机制处理
    // 这里只是标记状态
    RHI_LOG_DEBUG("D3D12UploadManager: 等待完成");
}

uint64_t D3D12UploadManager::GetTotalMemory() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    uint64_t total = 0;
    for (const auto& block : m_blocks)
    {
        total += block.capacity;
    }
    return total;
}

uint64_t D3D12UploadManager::GetUsedMemory() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    uint64_t used = 0;
    for (const auto& block : m_blocks)
    {
        used += block.used;
    }
    return used;
}

void D3D12UploadManager::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    RHI_LOG_INFO("D3D12UploadManager: 重置所有块");

    // 重置所有块
    for (auto& block : m_blocks)
    {
        block.Reset();
    }

    // 清空待处理队列
    m_pendingUploads.clear();

    // 重置当前块索引
    m_currentBlockIndex = 0;

    // 重置统计信息
    m_totalAllocations    = 0;
    m_totalBytesAllocated = 0;
}

// =============================================================================
// 扩展接口
// =============================================================================

UploadAllocation D3D12UploadManager::AllocateEx(uint64_t size, uint64_t alignment)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    UploadAllocation result = {};

    if (!m_isInitialized)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 未初始化，无法分配");
        return result;
    }

    if (size == 0)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 分配大小不能为 0");
        return result;
    }

    // 确保对齐满足最小要求
    alignment = std::max(alignment, m_config.alignment);

    // 查找或创建合适的块
    uint32_t blockIndex = FindAvailableBlock(size, alignment);
    if (blockIndex == UINT32_MAX)
    {
        // 创建新块
        blockIndex = CreateNewBlock(size);
        if (blockIndex == UINT32_MAX)
        {
            RHI_LOG_ERROR("D3D12UploadManager: 无法创建新块（大小: %llu 字节）", size);
            return result;
        }
    }

    UploadBlock& block = m_blocks[blockIndex];

    // 计算对齐后的偏移和大小
    uint64_t alignedOffset = AlignOffset(block.used, alignment);
    uint64_t alignedSize   = AlignSize(size, alignment);

    // 验证空间足够
    if (alignedOffset + alignedSize > block.capacity)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 块空间不足（需要: %llu, 可用: %llu）", alignedOffset + alignedSize,
                      block.capacity);
        return result;
    }

    // 填充分配结果
    result.buffer        = block.buffer.Get();
    result.mappedData    = static_cast<uint8_t*>(block.mappedPtr) + alignedOffset;
    result.gpuAddress    = block.buffer->GetGPUVirtualAddress() + alignedOffset;
    result.offset        = alignedOffset;
    result.size          = size;
    result.allocatedSize = alignedSize;
    result.blockIndex    = blockIndex;
    result.alignment     = alignment;

    // 更新块状态
    block.used  = alignedOffset + alignedSize;
    block.inUse = true;

    // 更新当前块索引
    m_currentBlockIndex = blockIndex;

    // 检查块是否接近满（95% 阈值）
    if (block.used >= block.capacity * 0.95)
    {
        block.isFull = true;
        RHI_LOG_DEBUG("D3D12UploadManager: 块 %u 已满 (%.1f%%)", blockIndex, block.used * 100.0 / block.capacity);
    }

    // 更新统计信息
    ++m_totalAllocations;
    m_totalBytesAllocated += alignedSize;

    if (m_config.enableDebugLog)
    {
        RHI_LOG_DEBUG("D3D12UploadManager: 分配成功");
        RHI_LOG_DEBUG("  - 大小: %llu 字节 (对齐后: %llu)", size, alignedSize);
        RHI_LOG_DEBUG("  - 偏移: %llu 字节", alignedOffset);
        RHI_LOG_DEBUG("  - 块索引: %u", blockIndex);
    }

    return result;
}

void D3D12UploadManager::RegisterFenceValue(const UploadAllocation& allocation, uint64_t fenceValue)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!allocation.IsValid())
    {
        RHI_LOG_WARNING("D3D12UploadManager: 无效分配，跳过围栏注册");
        return;
    }

    // 添加到待处理队列
    PendingUpload pending;
    pending.blockIndex = allocation.blockIndex;
    pending.fenceValue = fenceValue;
    pending.offset     = allocation.offset;
    pending.size       = allocation.allocatedSize;

    m_pendingUploads.push_back(pending);

    // 更新块的围栏值
    if (allocation.blockIndex < m_blocks.size())
    {
        m_blocks[allocation.blockIndex].fenceValue = fenceValue;
    }

    RHI_LOG_DEBUG("D3D12UploadManager: 注册围栏值 %llu (块 %u)", fenceValue, allocation.blockIndex);
}

void D3D12UploadManager::CleanupCompletedUploads(uint64_t completedFenceValue)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t cleanedCount = 0;

    // 清理已完成的待处理上传
    auto it = m_pendingUploads.begin();
    while (it != m_pendingUploads.end())
    {
        if (it->fenceValue <= completedFenceValue)
        {
            // GPU 已完成，可以清理
            cleanedCount++;
            it = m_pendingUploads.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 重置已完成的块
    for (auto& block : m_blocks)
    {
        if (block.fenceValue > 0 && block.fenceValue <= completedFenceValue)
        {
            block.Reset();
            RHI_LOG_DEBUG("D3D12UploadManager: 块 %u 已重置（围栏值: %llu <= %llu）", block.blockIndex,
                          block.fenceValue, completedFenceValue);
        }
    }

    if (cleanedCount > 0 || m_config.enableDebugLog)
    {
        RHI_LOG_DEBUG("D3D12UploadManager: 清理完成（围栏值: %llu，清理数: %zu）", completedFenceValue, cleanedCount);
    }
}

// =============================================================================
// 数据上传辅助函数
// =============================================================================

RHIResult D3D12UploadManager::UploadBufferData(ID3D12GraphicsCommandList* commandList, ID3D12Resource* destBuffer,
                                               uint64_t destOffset, const void* data, uint64_t dataSize)
{
    if (!m_isInitialized)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 未初始化，无法上传缓冲区数据");
        return RHIResult::InternalError;
    }

    if (!commandList || !destBuffer || !data || dataSize == 0)
    {
        RHI_LOG_ERROR("D3D12UploadManager: UploadBufferData 参数无效");
        return RHIResult::InvalidParameter;
    }

    // 分配上传内存
    UploadAllocation alloc = AllocateEx(dataSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    if (!alloc.IsValid())
    {
        RHI_LOG_ERROR("D3D12UploadManager: 无法分配上传内存（大小: %llu）", dataSize);
        return RHIResult::OutOfMemory;
    }

    // 复制数据到上传内存
    memcpy(alloc.mappedData, data, static_cast<size_t>(dataSize));

    // 录制 CopyBufferRegion 命令
    commandList->CopyBufferRegion(destBuffer,   // 目标缓冲区
                                  destOffset,   // 目标偏移
                                  alloc.buffer, // 源缓冲区（上传堆）
                                  alloc.offset, // 源偏移
                                  dataSize      // 大小
    );

    if (m_config.enableDebugLog)
    {
        RHI_LOG_DEBUG("D3D12UploadManager: 缓冲区数据上传成功");
        RHI_LOG_DEBUG("  - 数据大小: %llu 字节", dataSize);
        RHI_LOG_DEBUG("  - 目标偏移: %llu", destOffset);
        RHI_LOG_DEBUG("  - 源偏移: %llu", alloc.offset);
    }

    return RHIResult::Success;
}

RHIResult D3D12UploadManager::UploadTextureData(ID3D12GraphicsCommandList* commandList, ID3D12Resource* destTexture,
                                                const void* data, uint64_t dataSize,
                                                const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint,
                                                uint32_t                                  subresourceIndex)
{
    if (!m_isInitialized)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 未初始化，无法上传纹理数据");
        return RHIResult::InternalError;
    }

    if (!commandList || !destTexture || !data || dataSize == 0)
    {
        RHI_LOG_ERROR("D3D12UploadManager: UploadTextureData 参数无效");
        return RHIResult::InvalidParameter;
    }

    // 分配上传内存（需要考虑纹理行间距对齐）
    UploadAllocation alloc = AllocateEx(dataSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    if (!alloc.IsValid())
    {
        RHI_LOG_ERROR("D3D12UploadManager: 无法分配上传内存（大小: %llu）", dataSize);
        return RHIResult::OutOfMemory;
    }

    // 复制数据到上传内存（考虑行间距）
    // 注意：调用者应确保数据布局正确
    memcpy(alloc.mappedData, data, static_cast<size_t>(dataSize));

    // 设置目标位置
    D3D12_TEXTURE_COPY_LOCATION destLocation = {};
    destLocation.pResource                   = destTexture;
    destLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destLocation.SubresourceIndex            = subresourceIndex;

    // 设置源位置（上传堆中的放置布局）
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource                   = alloc.buffer;
    srcLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint.Offset      = alloc.offset;
    srcLocation.PlacedFootprint.Footprint   = footprint.Footprint;

    // 录制 CopyTextureRegion 命令
    commandList->CopyTextureRegion(&destLocation, // 目标位置
                                   0, 0, 0,       // 目标 x, y, z
                                   &srcLocation,  // 源位置
                                   nullptr        // 源盒（nullptr = 整个纹理）
    );

    if (m_config.enableDebugLog)
    {
        RHI_LOG_DEBUG("D3D12UploadManager: 纹理数据上传成功");
        RHI_LOG_DEBUG("  - 数据大小: %llu 字节", dataSize);
        RHI_LOG_DEBUG("  - 子资源索引: %u", subresourceIndex);
        RHI_LOG_DEBUG("  - 源偏移: %llu", alloc.offset);
    }

    return RHIResult::Success;
}

// =============================================================================
// 状态查询
// =============================================================================

uint32_t D3D12UploadManager::GetBlockCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32_t>(m_blocks.size());
}

float D3D12UploadManager::GetCurrentBlockUsage() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_blocks.empty() || m_currentBlockIndex >= m_blocks.size())
    {
        return 0.0f;
    }

    const UploadBlock& block = m_blocks[m_currentBlockIndex];
    return static_cast<float>(block.used) / static_cast<float>(block.capacity);
}

// =============================================================================
// 内部辅助方法
// =============================================================================

uint32_t D3D12UploadManager::CreateNewBlock(uint64_t minSize)
{
    if (!m_device)
    {
        RHI_LOG_ERROR("D3D12UploadManager: 设备为空，无法创建块");
        return UINT32_MAX;
    }

    ID3D12Device* d3dDevice = m_device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12UploadManager: D3D12 设备为空，无法创建块");
        return UINT32_MAX;
    }

    // 计算块大小
    uint64_t blockSize = std::max(m_config.defaultBlockSize, minSize);
    blockSize          = std::min(blockSize, m_config.maxBlockSize);

    // 确保块大小满足 D3D12 对齐要求
    blockSize = AlignSize(blockSize, m_config.alignment);

    RHI_LOG_DEBUG("D3D12UploadManager: 创建新块（大小: %.2f MB）", blockSize / (1024.0 * 1024.0));

    // 创建上传堆属性
    HeapProperties uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    // 创建缓冲区描述
    D3D12_RESOURCE_DESC bufferDesc = CreateBufferDesc(blockSize);

    // 创建上传堆资源
    ComPtr<ID3D12Resource> resource;
    HRESULT                hr =
        d3dDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                           D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));

    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12UploadManager: 创建上传堆资源失败 (HRESULT: 0x%08X)", hr);
        return UINT32_MAX;
    }

    // 设置资源名称（用于调试）
    std::wostringstream nameStream;
    nameStream << L"UploadBlock_" << m_blocks.size();
    resource->SetName(nameStream.str().c_str());

    // 映射内存
    void*         mappedPtr = nullptr;
    ResourceRange readRange(0, 0); // 我们不会从 CPU 读取
    hr = resource->Map(0, readRange, &mappedPtr);

    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12UploadManager: 映射上传堆失败 (HRESULT: 0x%08X)", hr);
        return UINT32_MAX;
    }

    // 创建块
    UploadBlock block;
    block.buffer     = resource;
    block.mappedPtr  = mappedPtr;
    block.capacity   = blockSize;
    block.used       = 0;
    block.fenceValue = 0;
    block.isFull     = false;
    block.inUse      = false;
    block.blockIndex = static_cast<uint32_t>(m_blocks.size());

    m_blocks.push_back(block);

    RHI_LOG_DEBUG("D3D12UploadManager: 块 %u 创建成功", block.blockIndex);

    return block.blockIndex;
}

uint32_t D3D12UploadManager::FindAvailableBlock(uint64_t size, uint64_t alignment)
{
    // 首先检查当前块
    if (m_currentBlockIndex < m_blocks.size())
    {
        UploadBlock& currentBlock = m_blocks[m_currentBlockIndex];
        if (currentBlock.CanAllocate(size, alignment))
        {
            return m_currentBlockIndex;
        }
    }

    // 搜索所有块（从最旧的开始，优先重用）
    for (uint32_t i = 0; i < m_blocks.size(); ++i)
    {
        // 跳过当前块（已经检查过）
        if (i == m_currentBlockIndex)
        {
            continue;
        }

        UploadBlock& block = m_blocks[i];
        if (block.CanAllocate(size, alignment))
        {
            return i;
        }
    }

    // 没有找到可用块
    return UINT32_MAX;
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12