// =============================================================================
// D3D12FrameResourceManager.h - D3D12 帧资源管理器
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "D3D12Device.h"
#include "IFrameResourceManager.h"

// Windows/DX12 头文件
#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl/client.h>

namespace Engine::RHI::D3D12
{

using Microsoft::WRL::ComPtr;

// =============================================================================
// D3D12FrameContext - 单帧上下文数据
// =============================================================================

/**
 * @brief 存储 CPU 和 GPU 同步所需的所有资源
 *
 * 每个帧索引对应一个帧上下文，使用三缓冲来避免 CPU 等待 GPU
 */
struct D3D12FrameContext
{
    /// 帧索引（0 到 numFrames-1）
    uint32_t frameIndex = 0;

    /// 该帧最后一次提交时的围栏值
    uint64_t fenceValue = 0;

    /// D3D12 围栏对象
    ComPtr<ID3D12Fence> fence;

    /// 围栏事件句柄（用于 CPU 等待）
    HANDLE fenceEvent = nullptr;

    /// 命令分配器（每帧重置）
    ComPtr<ID3D12CommandAllocator> commandAllocator;

    // ========== 临时资源（GPU 完成后清理） ==========

    /// 上传缓冲区列表（临时上传资源）
    std::vector<ComPtr<ID3D12Resource>> uploadBuffers;

    /// 临时资源列表（通用临时资源）
    std::vector<ComPtr<ID3D12Resource>> temporaryResources;

    /// 延迟删除资源列表（原始指针，不持有引用）
    std::vector<ID3D12Resource*> pendingDeletions;

    /// 是否已初始化
    bool isInitialized = false;
};

// =============================================================================
// D3D12FrameResourceManager - D3D12 帧资源管理器实现
// =============================================================================

/**
 * @brief D3D12 帧资源管理器
 *
 * 实现三缓冲帧资源管理：
 * - 每帧独立的命令分配器
 * - GPU/CPU 同步围栏
 * - 临时资源生命周期管理
 * - 延迟删除机制
 *
 * 使用方式：
 * 1. 初始化时调用 Initialize()
 * 2. 每帧开始时调用 BeginFrame()
 * 3. 录制命令到当前帧的命令分配器
 * 4. 每帧结束时调用 EndFrame()
 * 5. 提交命令队列后，围栏会自动信号
 */
class D3D12FrameResourceManager : public IFrameResourceManager
{
  public:
    // ========== 构造/析构 ==========

    /**
     * @brief 构造帧资源管理器
     * @param device D3D12 设备指针
     * @param numFrames 帧缓冲数量（默认 3）
     */
    D3D12FrameResourceManager(D3D12Device* device, uint32_t numFrames = 3);

    /**
     * @brief 析构函数
     */
    virtual ~D3D12FrameResourceManager() override;

    // ========== 禁止拷贝 ==========

    D3D12FrameResourceManager(const D3D12FrameResourceManager&) = delete;
    D3D12FrameResourceManager& operator=(const D3D12FrameResourceManager&) = delete;

    // ========== 移动语义 ==========

    D3D12FrameResourceManager(D3D12FrameResourceManager&&) noexcept = default;
    D3D12FrameResourceManager& operator=(D3D12FrameResourceManager&&) noexcept = default;

    // ========== 初始化 ==========

    /**
     * @brief 初始化帧资源管理器
     * @return RHIResult::Success 成功
     * @return RHIResult::OutOfMemory 内存不足
     * @return RHIResult::InternalError D3D12 调用失败
     */
    RHIResult Initialize();

    /**
     * @brief 关闭帧资源管理器
     */
    void Shutdown();

    // ========== IFrameResourceManager 接口实现 ==========

    /**
     * @brief 开始新帧
     *
     * 执行以下操作：
     * 1. 等待 GPU 完成该帧索引的上一轮渲染
     * 2. 重置命令分配器
     * 3. 清理临时资源
     * 4. 处理延迟删除
     */
    virtual void BeginFrame() override;

    /**
     * @brief 结束当前帧
     *
     * 执行以下操作：
     * 1. 递增围栏值
     * 2. 切换到下一帧索引
     */
    virtual void EndFrame() override;

    /**
     * @brief 获取当前帧索引
     * @return 帧索引（0 到 numFrames-1）
     */
    virtual uint32_t GetCurrentFrameIndex() const override { return m_currentFrameIndex; }

    /**
     * @brief 获取帧缓冲数量
     * @return 帧缓冲数量
     */
    virtual uint32_t GetFrameCount() const override { return m_numFrames; }

    /**
     * @brief 等待指定帧完成
     * @param frameIndex 帧索引
     */
    virtual void WaitForFrame(uint32_t frameIndex) override;

    /**
     * @brief 获取指定帧的围栏值
     * @param frameIndex 帧索引
     * @return 围栏值
     */
    virtual uint64_t GetFrameFenceValue(uint32_t frameIndex) const override;

    // ========== D3D12 特定接口 ==========

    /**
     * @brief 获取当前帧的命令分配器
     * @return ID3D12CommandAllocator* 命令分配器指针
     */
    ID3D12CommandAllocator* GetCurrentCommandAllocator() const;

    /**
     * @brief 获取当前帧的围栏
     * @return ID3D12Fence* 围栏指针
     */
    ID3D12Fence* GetCurrentFence() const;

    /**
     * @brief 获取当前帧的围栏值（下次信号值）
     * @return 围栏值
     */
    uint64_t GetCurrentFenceValue() const;

    /**
     * @brief 获取当前帧的围栏事件
     * @return HANDLE 事件句柄
     */
    HANDLE GetCurrentFenceEvent() const;

    // ========== 资源注册 ==========

    /**
     * @brief 注册上传缓冲区
     *
     * 上传缓冲区会在该帧 GPU 完成后自动释放
     *
     * @param buffer 上传缓冲区（引用计数会增加）
     */
    void RegisterUploadBuffer(ID3D12Resource* buffer);

    /**
     * @brief 注册临时资源
     *
     * 临时资源会在该帧 GPU 完成后自动释放
     *
     * @param resource 临时资源（引用计数会增加）
     */
    void RegisterTemporaryResource(ID3D12Resource* resource);

    /**
     * @brief 延迟删除资源
     *
     * 资源会在该帧 GPU 完成后调用 Release()
     * 注意：此方法接管资源的所有权，调用者不应再持有引用
     *
     * @param resource 要删除的资源
     */
    void DelayedDeleteResource(ID3D12Resource* resource);

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否已初始化
     * @return true 如果已初始化
     */
    bool IsInitialized() const { return m_isInitialized; }

    /**
     * @brief 检查指定帧是否 GPU 已完成
     * @param frameIndex 帧索引
     * @return true 如果 GPU 已完成
     */
    bool IsFrameComplete(uint32_t frameIndex) const;

    /**
     * @brief 等待所有帧完成
     *
     * 用于关闭前的安全清理
     */
    void WaitForAllFrames();

  private:
    // ========== 内部辅助方法 ==========

    /**
     * @brief 等待围栏达到指定值
     * @param fence 围栏对象
     * @param fenceValue 目标围栏值
     * @param fenceEvent 围栏事件
     */
    void WaitForFence(ID3D12Fence* fence, uint64_t fenceValue, HANDLE fenceEvent);

    /**
     * @brief 清理帧上下文的临时资源
     * @param ctx 帧上下文引用
     */
    void CleanupFrameResources(D3D12FrameContext& ctx);

    /**
     * @brief 验证帧索引有效性
     * @param frameIndex 帧索引
     * @return true 如果有效
     */
    bool IsValidFrameIndex(uint32_t frameIndex) const { return frameIndex < m_numFrames; }

  private:
    // ========== 成员变量 ==========

    /// D3D12 设备（非拥有指针）
    D3D12Device* m_device = nullptr;

    /// 帧缓冲数量
    uint32_t m_numFrames = 3;

    /// 当前帧索引
    uint32_t m_currentFrameIndex = 0;

    /// 下一个围栏值（全局递增）
    uint64_t m_nextFenceValue = 1;

    /// 帧上下文数组
    std::vector<D3D12FrameContext> m_frameContexts;

    /// 是否已初始化
    bool m_isInitialized = false;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12