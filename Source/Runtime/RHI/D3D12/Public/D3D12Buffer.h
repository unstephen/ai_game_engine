// =============================================================================
// D3D12Buffer.h - D3D12 缓冲区实现
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "D3D12Device.h"

namespace Engine::RHI::D3D12
{

// =============================================================================
// D3D12Buffer - D3D12 缓冲区实现
// =============================================================================

/**
 * @brief D3D12 缓冲区实现
 *
 * 支持多种缓冲区类型：
 * - 顶点缓冲区 (Vertex Buffer)
 * - 索引缓冲区 (Index Buffer)
 * - 常量缓冲区 (Constant Buffer)
 * - 存储/结构化缓冲区 (Storage/Structured Buffer)
 *
 * 支持两种堆类型：
 * - 上传堆 (Upload Heap): CPU 可访问，用于动态数据
 * - 默认堆 (Default Heap): GPU 优化，用于静态数据
 */
class D3D12Buffer : public IBuffer
{
  public:
    // ========== 构造/析构 ==========

    D3D12Buffer();
    virtual ~D3D12Buffer() override;

    // 禁止拷贝
    D3D12Buffer(const D3D12Buffer&)            = delete;
    D3D12Buffer& operator=(const D3D12Buffer&) = delete;

    // 允许移动
    D3D12Buffer(D3D12Buffer&& other) noexcept;
    D3D12Buffer& operator=(D3D12Buffer&& other) noexcept;

    // ========== 初始化 ==========

    /**
     * @brief 初始化缓冲区
     * @param device D3D12 设备指针
     * @param desc 缓冲区描述
     * @return RHIResult 结果码
     */
    RHIResult Initialize(D3D12Device* device, const BufferDesc& desc);

    /**
     * @brief 关闭缓冲区，释放资源
     */
    void Shutdown();

    // ========== IBuffer 接口实现 ==========

    virtual uint64_t    GetSize() const override { return m_size; }
    virtual uint32_t    GetStride() const override { return m_stride; }
    virtual BufferUsage GetUsage() const override { return m_usage; }
    virtual uint64_t    GetGPUVirtualAddress() const override;
    virtual void*       Map() override;
    virtual void        Unmap() override;
    virtual void        UpdateData(const void* data, uint64_t size, uint64_t offset = 0) override;

    // ========== D3D12 特定接口 ==========

    /**
     * @brief 获取 D3D12 资源
     * @return ID3D12Resource* D3D12 资源指针
     */
    ID3D12Resource* GetD3D12Resource() const { return m_resource.Get(); }

    /**
     * @brief 获取顶点缓冲区视图
     * @return D3D12_VERTEX_BUFFER_VIEW 顶点缓冲区视图
     */
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

    /**
     * @brief 获取索引缓冲区视图
     * @return D3D12_INDEX_BUFFER_VIEW 索引缓冲区视图
     */
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

    /**
     * @brief 检查是否为 CPU 可访问（上传堆）
     * @return true 如果是上传堆
     */
    bool IsCPUAccessible() const { return m_cpuAccessible; }

    /**
     * @brief 获取资源状态
     * @return D3D12_RESOURCE_STATES 当前资源状态
     */
    D3D12_RESOURCE_STATES GetResourceState() const { return m_resourceState; }

    /**
     * @brief 设置资源状态（用于状态转换追踪）
     * @param state 新的资源状态
     */
    void SetResourceState(D3D12_RESOURCE_STATES state) { m_resourceState = state; }

  private:
    // ========== 内部方法 ==========

    /**
     * @brief 创建 D3D12 资源
     * @param device D3D12 设备
     * @param desc 缓冲区描述
     * @return RHIResult 结果码
     */
    RHIResult CreateResource(D3D12Device* device, const BufferDesc& desc);

    /**
     * @brief 根据用途确定资源状态
     * @param usage 缓冲区用途
     * @param initialState 初始状态
     * @return D3D12_RESOURCE_STATES 资源状态
     */
    static D3D12_RESOURCE_STATES DetermineResourceState(BufferUsage usage, ResourceState initialState);

    /**
     * @brief 根据用途确定资源标志
     * @param usage 缓冲区用途
     * @return D3D12_RESOURCE_FLAGS 资源标志
     */
    static D3D12_RESOURCE_FLAGS DetermineResourceFlags(BufferUsage usage);

  private:
    // ========== 成员变量 ==========

    // D3D12 资源
    ComPtr<ID3D12Resource> m_resource;

    // 缓冲区属性
    uint64_t    m_size   = 0;                 // 缓冲区大小（字节）
    uint32_t    m_stride = 0;                 // 结构化缓冲区的元素大小
    BufferUsage m_usage  = BufferUsage::None; // 缓冲区用途

    // 映射状态
    void* m_mappedData    = nullptr; // 映射后的 CPU 指针
    bool  m_isMapped      = false;   // 是否已映射
    bool  m_cpuAccessible = false;   // 是否为上传堆（CPU 可访问）

    // 资源状态
    D3D12_RESOURCE_STATES m_resourceState = D3D12_RESOURCE_STATE_COMMON;

    // 调试名称
    std::string m_debugName;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12