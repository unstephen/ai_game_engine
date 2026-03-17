// =============================================================================
// D3D12UploadManager.h - D3D12 上传管理器
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "IUploadManager.h"
#include "D3D12Device.h"

// Windows/DX12 头文件
#include <d3d12.h>
#include <wrl/client.h>

#include <vector>
#include <mutex>
#include <deque>

namespace Engine::RHI::D3D12 {

using Microsoft::WRL::ComPtr;

// =============================================================================
// 上传管理器配置
// =============================================================================

/**
 * @brief 上传管理器配置
 */
struct UploadManagerConfig {
    /// 默认块大小（字节），默认 64MB
    uint64_t defaultBlockSize = 64 * 1024 * 1024;
    
    /// 最大块大小（字节），默认 256MB
    uint64_t maxBlockSize = 256 * 1024 * 1024;
    
    /// 预分配块数量
    uint32_t preallocatedBlocks = 2;
    
    /// 内存对齐（字节），D3D12 要求 64KB 对齐
    uint64_t alignment = 64 * 1024;
    
    /// 是否启用调试日志
    bool enableDebugLog = false;
};

// =============================================================================
// UploadBlock - 上传块
// =============================================================================

/**
 * @brief 上传块
 * 
 * 表示一块连续的上传堆内存，用于批量上传数据到 GPU。
 * 当块使用率超过阈值时，标记为已满并创建新块。
 * GPU 完成使用后，块可以被重置和重用。
 */
struct UploadBlock {
    /// D3D12 上传堆资源
    ComPtr<ID3D12Resource> buffer;
    
    /// 映射的 CPU 可见内存指针
    void* mappedPtr = nullptr;
    
    /// 块容量（字节）
    uint64_t capacity = 0;
    
    /// 已使用量（字节）
    uint64_t used = 0;
    
    /// 可重用的围栏值（GPU 完成后）
    uint64_t fenceValue = 0;
    
    /// 是否已满
    bool isFull = false;
    
    /// 是否在使用中
    bool inUse = false;
    
    /// 块索引
    uint32_t blockIndex = 0;
    
    /// 重置块以供重用
    void Reset() {
        used = 0;
        isFull = false;
        inUse = false;
        fenceValue = 0;
    }
    
    /// 获取可用空间
    uint64_t GetAvailableSpace() const {
        return capacity - used;
    }
    
    /// 检查是否可以分配指定大小
    bool CanAllocate(uint64_t size, uint64_t align) const {
        if (isFull || inUse) return false;
        
        // 计算对齐后的偏移
        uint64_t alignedOffset = (used + align - 1) & ~(align - 1);
        uint64_t endOffset = alignedOffset + size;
        
        return endOffset <= capacity;
    }
};

// =============================================================================
// UploadAllocation - 上传分配结果
// =============================================================================

/**
 * @brief 上传分配结果
 * 
 * 包含一次上传内存分配的所有信息，包括：
 * - 上传缓冲区指针
 * - 映射的 CPU 内存指针
 * - GPU 虚拟地址
 * - 分配偏移和大小
 */
struct UploadAllocation {
    /// 上传缓冲区（D3D12 资源）
    ID3D12Resource* buffer = nullptr;
    
    /// 映射的 CPU 内存指针
    void* mappedData = nullptr;
    
    /// GPU 虚拟地址
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
    
    /// 在块中的偏移（字节）
    uint64_t offset = 0;
    
    /// 分配大小（字节）
    uint64_t size = 0;
    
    /// 实际分配大小（含对齐）
    uint64_t allocatedSize = 0;
    
    /// 块索引
    uint32_t blockIndex = 0;
    
    /// 对齐
    uint64_t alignment = 256;
    
    /// 检查分配是否有效
    bool IsValid() const { return buffer != nullptr && mappedData != nullptr; }
    
    /// 获取数据的 CPU 指针
    template<typename T = void>
    T* GetData() const {
        return static_cast<T*>(mappedData);
    }
};

// =============================================================================
// PendingUpload - 待处理上传
// =============================================================================

/**
 * @brief 待处理上传
 * 
 * 跟踪已提交但 GPU 尚未完成的上传，用于资源生命周期管理。
 */
struct PendingUpload {
    /// 块索引
    uint32_t blockIndex = 0;
    
    /// 围栏值
    uint64_t fenceValue = 0;
    
    /// 分配偏移
    uint64_t offset = 0;
    
    /// 分配大小
    uint64_t size = 0;
};

// =============================================================================
// D3D12UploadManager - D3D12 上传管理器实现
// =============================================================================

/**
 * @brief D3D12 上传管理器
 * 
 * 实现上传堆内存的块状管理：
 * - 默认使用 64MB 块
 * - 自动创建新块当当前块满
 * - GPU 完成后自动重置和重用块
 * - 支持缓冲区和纹理数据上传
 * 
 * 线程安全：所有公共方法都使用互斥锁保护。
 * 
 * 使用方式：
 * 1. 初始化时调用 Initialize()
 * 2. 使用 Allocate() 分配上传内存
 * 3. 复制数据到映射的 CPU 内存
 * 4. 使用 UploadBufferData/UploadTextureData 录制复制命令
 * 5. 调用 Submit() 提交上传
 * 6. GPU 完成后调用 CleanupCompletedUploads() 清理
 */
class D3D12UploadManager : public IUploadManager {
public:
    // ========== 构造/析构 ==========
    
    /**
     * @brief 构造上传管理器
     * @param device D3D12 设备指针（非拥有）
     */
    explicit D3D12UploadManager(D3D12Device* device);
    
    /**
     * @brief 析构函数
     */
    virtual ~D3D12UploadManager() override;
    
    // ========== 禁止拷贝 ==========
    
    D3D12UploadManager(const D3D12UploadManager&) = delete;
    D3D12UploadManager& operator=(const D3D12UploadManager&) = delete;
    
    // ========== 移动语义 ==========
    
    D3D12UploadManager(D3D12UploadManager&&) noexcept = default;
    D3D12UploadManager& operator=(D3D12UploadManager&&) noexcept = default;
    
    // ========== 初始化 ==========
    
    /**
     * @brief 初始化上传管理器
     * @param config 配置参数
     * @return RHIResult::Success 成功
     * @return RHIResult::InvalidParameter 参数无效
     * @return RHIResult::OutOfMemory 内存不足
     * @return RHIResult::InternalError D3D12 调用失败
     */
    RHIResult Initialize(const UploadManagerConfig& config = {});
    
    /**
     * @brief 关闭上传管理器
     */
    void Shutdown();
    
    // ========== IUploadManager 接口实现 ==========
    
    /**
     * @brief 分配上传内存
     * @param size 请求大小（字节）
     * @param alignment 对齐（字节），默认 256
     * @return CPU 可访问内存指针，失败返回 nullptr
     */
    virtual void* Allocate(uint64_t size, uint64_t alignment = 256) override;
    
    /**
     * @brief 提交上传
     */
    virtual void Submit() override;
    
    /**
     * @brief 等待所有上传完成
     */
    virtual void WaitForCompletion() override;
    
    /**
     * @brief 获取总上传内存大小
     * @return 总内存（字节）
     */
    virtual uint64_t GetTotalMemory() const override;
    
    /**
     * @brief 获取已使用上传内存大小
     * @return 已使用内存（字节）
     */
    virtual uint64_t GetUsedMemory() const override;
    
    /**
     * @brief 重置上传缓冲区
     */
    virtual void Reset() override;
    
    // ========== 扩展接口 ==========
    
    /**
     * @brief 分配上传内存并返回详细信息
     * @param size 请求大小（字节）
     * @param alignment 对齐（字节），默认 256
     * @return UploadAllocation 分配结果
     */
    UploadAllocation AllocateEx(uint64_t size, uint64_t alignment = 256);
    
    /**
     * @brief 注册围栏值
     * 
     * 当 GPU 达到指定围栏值时，相关块可以被重置和重用。
     * 
     * @param allocation 分配结果
     * @param fenceValue 围栏值
     */
    void RegisterFenceValue(const UploadAllocation& allocation, uint64_t fenceValue);
    
    /**
     * @brief 清理已完成的上传
     * @param completedFenceValue GPU 已完成的围栏值
     */
    void CleanupCompletedUploads(uint64_t completedFenceValue);
    
    /**
     * @brief 上传缓冲区数据
     * 
     * 分配上传内存、复制数据、录制 CopyBufferRegion 命令。
     * 
     * @param commandList D3D12 命令列表
     * @param destBuffer 目标缓冲区（默认堆）
     * @param destOffset 目标偏移（字节）
     * @param data 源数据指针
     * @param dataSize 数据大小（字节）
     * @return RHIResult::Success 成功
     * @return RHIResult::OutOfMemory 内存不足
     */
    RHIResult UploadBufferData(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* destBuffer,
        uint64_t destOffset,
        const void* data,
        uint64_t dataSize);
    
    /**
     * @brief 上传纹理数据
     * 
     * 分配上传内存、复制数据、录制 CopyTextureRegion 命令。
     * 
     * @param commandList D3D12 命令列表
     * @param destTexture 目标纹理（默认堆）
     * @param data 源数据指针
     * @param dataSize 数据大小（字节）
     * @param footprint 纹理放置布局
     * @param subresourceIndex 子资源索引
     * @return RHIResult::Success 成功
     * @return RHIResult::OutOfMemory 内存不足
     */
    RHIResult UploadTextureData(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* destTexture,
        const void* data,
        uint64_t dataSize,
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint,
        uint32_t subresourceIndex);
    
    // ========== 状态查询 ==========
    
    /**
     * @brief 检查是否已初始化
     * @return true 如果已初始化
     */
    bool IsInitialized() const { return m_isInitialized; }
    
    /**
     * @brief 获取块数量
     * @return 块数量
     */
    uint32_t GetBlockCount() const;
    
    /**
     * @brief 获取当前块使用率
     * @return 使用率（0.0 - 1.0）
     */
    float GetCurrentBlockUsage() const;
    
    /**
     * @brief 获取配置
     * @return 配置常引用
     */
    const UploadManagerConfig& GetConfig() const { return m_config; }

private:
    // ========== 内部辅助方法 ==========
    
    /**
     * @brief 创建新的上传块
     * @param minSize 最小大小（字节）
     * @return 新块的索引，失败返回 UINT32_MAX
     */
    uint32_t CreateNewBlock(uint64_t minSize);
    
    /**
     * @brief 查找可用的块
     * @param size 需要的大小
     * @param alignment 对齐
     * @return 块索引，未找到返回 UINT32_MAX
     */
    uint32_t FindAvailableBlock(uint64_t size, uint64_t alignment);
    
    /**
     * @brief 获取块引用
     * @param index 块索引
     * @return 块引用
     */
    UploadBlock& GetBlock(uint32_t index) { return m_blocks[index]; }
    const UploadBlock& GetBlock(uint32_t index) const { return m_blocks[index]; }
    
    /**
     * @brief 计算对齐后的偏移
     * @param offset 原始偏移
     * @param alignment 对齐
     * @return 对齐后的偏移
     */
    static uint64_t AlignOffset(uint64_t offset, uint64_t alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }
    
    /**
     * @brief 计算对齐后的大小
     * @param size 原始大小
     * @param alignment 对齐
     * @return 对齐后的大小
     */
    static uint64_t AlignSize(uint64_t size, uint64_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

private:
    // ========== 成员变量 ==========
    
    /// D3D12 设备（非拥有指针）
    D3D12Device* m_device = nullptr;
    
    /// 配置
    UploadManagerConfig m_config;
    
    /// 上传块数组
    std::vector<UploadBlock> m_blocks;
    
    /// 当前块索引
    uint32_t m_currentBlockIndex = 0;
    
    /// 待处理上传队列
    std::deque<PendingUpload> m_pendingUploads;
    
    /// 是否已初始化
    bool m_isInitialized = false;
    
    /// 线程安全锁
    mutable std::mutex m_mutex;
    
    /// 统计信息
    uint64_t m_totalAllocations = 0;
    uint64_t m_totalBytesAllocated = 0;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12