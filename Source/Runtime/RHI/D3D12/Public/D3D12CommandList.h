// =============================================================================
// D3D12CommandList.h - D3D12 命令列表实现
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "D3D12Buffer.h"
#include "D3D12Device.h"
#include "D3D12Texture.h"

namespace Engine::RHI::D3D12
{

// =============================================================================
// D3D12CommandList - D3D12 命令列表实现
// =============================================================================

/**
 * @brief D3D12 命令列表实现
 *
 * 支持功能：
 * - 命令列表生命周期管理（Begin/End/Reset）
 * - 管线状态和根签名绑定
 * - 资源绑定（顶点/索引/常量缓冲区）
 * - 绘制命令（Draw/DrawIndexed/DrawInstanced）
 * - 计算调度（Dispatch）
 * - 资源屏障和状态转换
 * - 渲染目标设置和清除
 * - 视口和裁剪矩形设置
 * - 复制命令
 * - 调试标记（PIX/RenderDoc 支持）
 *
 * 特点：
 * - 线程安全的录制状态追踪
 * - 完整的错误检查
 * - 详细的调试日志
 * - 状态缓存避免冗余 API 调用
 */
class D3D12CommandList : public ICommandList
{
  public:
    // ========== 构造/析构 ==========

    D3D12CommandList();
    virtual ~D3D12CommandList() override;

    // 禁止拷贝
    D3D12CommandList(const D3D12CommandList&)            = delete;
    D3D12CommandList& operator=(const D3D12CommandList&) = delete;

    // 允许移动
    D3D12CommandList(D3D12CommandList&& other) noexcept;
    D3D12CommandList& operator=(D3D12CommandList&& other) noexcept;

    // ========== 初始化 ==========

    /**
     * @brief 初始化命令列表
     * @param device D3D12 设备指针
     * @param allocator 命令分配器
     * @param type 命令列表类型（默认为 DIRECT）
     * @return RHIResult 结果码
     */
    RHIResult Initialize(D3D12Device* device, ID3D12CommandAllocator* allocator,
                         D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    /**
     * @brief 关闭命令列表，释放资源
     */
    void Shutdown();

    // ========== ICommandList 接口实现 ==========

    // 生命周期
    virtual void Begin() override;
    virtual void End() override;

    // 原生接口
    virtual void*    GetNativeCommandList() const override;
    virtual uint32_t GetType() const override;

    // 资源屏障
    virtual void ResourceBarrier(ITexture* texture, ResourceState newState) override;
    virtual void ResourceBarrier(IBuffer* buffer, ResourceState newState) override;

    // 清除操作
    virtual void ClearRenderTarget(ITexture* renderTarget, const float clearColor[4]) override;
    virtual void ClearDepthStencil(ITexture* depthStencil, bool clearDepth, bool clearStencil, float depth,
                                   uint8_t stencil) override;

    // 状态设置
    virtual void SetPipelineState(IPipelineState* pipeline) override;
    virtual void SetRootSignature(IRootSignature* rootSignature) override;
    virtual void SetRenderTargets(ITexture* const* renderTargets, uint32_t count,
                                  ITexture* depthStencil = nullptr) override;
    virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f,
                             float maxDepth = 1.0f) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;
    virtual void SetPrimitiveTopology(PrimitiveTopology topology) override;

    // 资源绑定
    virtual void SetVertexBuffer(uint32_t slot, IBuffer* buffer, uint64_t offset = 0) override;
    virtual void SetIndexBuffer(IBuffer* buffer, uint64_t offset = 0) override;

    // 绘制调用
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) override;
    virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex = 0,
                               uint32_t startInstance = 0) override;
    virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex = 0,
                                      int32_t baseVertex = 0, uint32_t startInstance = 0) override;

    // ========== D3D12 特定接口 ==========

    /**
     * @brief 获取 D3D12 命令列表
     * @return ID3D12GraphicsCommandList* D3D12 命令列表指针
     */
    ID3D12GraphicsCommandList* GetD3D12CommandList() const { return m_commandList.Get(); }

    /**
     * @brief 获取命令分配器
     * @return ID3D12CommandAllocator* 命令分配器指针
     */
    ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocator; }

    /**
     * @brief 检查是否正在录制
     * @return true 如果正在录制
     */
    bool IsRecording() const { return m_isRecording; }

    /**
     * @brief 检查命令列表是否已关闭
     * @return true 如果已关闭
     */
    bool IsClosed() const { return m_isClosed; }

    /**
     * @brief 重置命令列表（使用相同的分配器）
     * @param pso 可选的初始管线状态
     */
    void Reset(ID3D12PipelineState* pso = nullptr);

    // ========== 高级资源绑定 ==========

    /**
     * @brief 设置图形根常量缓冲区视图
     * @param rootParameterIndex 根参数索引
     * @param address GPU 虚拟地址
     */
    void SetGraphicsRootConstantBufferView(uint32_t rootParameterIndex, uint64_t address);

    /**
     * @brief 设置图形根着色器资源视图
     * @param rootParameterIndex 根参数索引
     * @param address GPU 虚拟地址
     */
    void SetGraphicsRootShaderResourceView(uint32_t rootParameterIndex, uint64_t address);

    /**
     * @brief 设置图形根无序访问视图
     * @param rootParameterIndex 根参数索引
     * @param address GPU 虚拟地址
     */
    void SetGraphicsRootUnorderedAccessView(uint32_t rootParameterIndex, uint64_t address);

    /**
     * @brief 设置计算根常量缓冲区视图
     * @param rootParameterIndex 根参数索引
     * @param address GPU 虚拟地址
     */
    void SetComputeRootConstantBufferView(uint32_t rootParameterIndex, uint64_t address);

    // ========== 描述符堆绑定 ==========

    /**
     * @brief 设置描述符堆
     * @param heap 描述符堆
     */
    void SetDescriptorHeaps(ID3D12DescriptorHeap* heap);

    /**
     * @brief 设置多个描述符堆
     * @param heaps 描述符堆数组
     * @param count 数量
     */
    void SetDescriptorHeaps(ID3D12DescriptorHeap* const* heaps, uint32_t count);

    // ========== 绘制扩展 ==========

    /**
     * @brief 计算调度
     * @param groupCountX X 维度工作组数
     * @param groupCountY Y 维度工作组数
     * @param groupCountZ Z 维度工作组数
     */
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    // ========== 复制命令 ==========

    /**
     * @brief 复制缓冲区区域
     * @param dest 目标缓冲区
     * @param destOffset 目标偏移
     * @param src 源缓冲区
     * @param srcOffset 源偏移
     * @param size 复制大小
     */
    void CopyBufferRegion(ID3D12Resource* dest, uint64_t destOffset, ID3D12Resource* src, uint64_t srcOffset,
                          uint64_t size);

    /**
     * @brief 复制纹理区域
     * @param dest 目标纹理
     * @param destOffset 目标偏移
     * @param src 源纹理
     * @param srcOffset 源偏移
     * @param width 宽度
     * @param height 高度
     */
    void CopyTextureRegion(ID3D12Resource* dest, uint32_t destX, uint32_t destY, uint32_t destZ, ID3D12Resource* src,
                           uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t width, uint32_t height,
                           uint32_t depth);

    /**
     * @brief 复制资源（完整资源）
     * @param dest 目标资源
     * @param src 源资源
     */
    void CopyResource(ID3D12Resource* dest, ID3D12Resource* src);

    // ========== 资源屏障扩展 ==========

    /**
     * @brief 添加资源转换屏障
     * @param resource D3D12 资源
     * @param before 转换前状态
     * @param after 转换后状态
     * @param subresource 子资源索引（默认为全部）
     */
    void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
                           UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    /**
     * @brief 添加 UAV 屏障
     * @param resource D3D12 资源（可选）
     */
    void UAVBarrier(ID3D12Resource* resource = nullptr);

    /**
     * @brief 添加别名屏障
     * @param before 之前的资源（可选）
     * @param after 之后的资源（可选）
     */
    void AliasBarrier(ID3D12Resource* before = nullptr, ID3D12Resource* after = nullptr);

    /**
     * @brief 批量添加资源屏障
     * @param barriers 屏障数组
     * @param count 数量
     */
    void ResourceBarriers(const D3D12_RESOURCE_BARRIER* barriers, uint32_t count);

    // ========== 调试支持 ==========

    /**
     * @brief 设置 PIX 调试标记
     * @param name 标记名称
     */
    void SetMarker(const char* name);

    /**
     * @brief 开始 PIX 调试事件
     * @param name 事件名称
     */
    void BeginEvent(const char* name);

    /**
     * @brief 结束 PIX 调试事件
     */
    void EndEvent();

    // ========== 索引缓冲区格式 ==========

    /**
     * @brief 设置索引缓冲区（带格式）
     * @param buffer 索引缓冲区
     * @param offset 偏移
     * @param format 索引格式
     */
    void SetIndexBuffer(IBuffer* buffer, uint64_t offset, DXGI_FORMAT format);

  private:
    // ========== 内部方法 ==========

    /**
     * @brief 检查命令列表是否在录制状态
     * @return true 如果正在录制
     */
    bool CheckRecordingState() const;

    /**
     * @brief 将 RHI 图元拓扑转换为 D3D12 拓扑
     * @param topology RHI 图元拓扑
     * @return D3D12_PRIMITIVE_TOPOLOGY D3D12 拓扑
     */
    static D3D12_PRIMITIVE_TOPOLOGY ToD3D12PrimitiveTopology(PrimitiveTopology topology);

    /**
     * @brief 将 RHI 资源状态转换为 D3D12 资源状态
     * @param state RHI 资源状态
     * @return D3D12_RESOURCE_STATES D3D12 资源状态
     */
    static D3D12_RESOURCE_STATES ToD3D12ResourceState(ResourceState state);

    /**
     * @brief 将 RHI 格式转换为 DXGI 格式（用于索引缓冲区）
     * @param format RHI 格式
     * @return DXGI_FORMAT DXGI 格式
     */
    static DXGI_FORMAT ToDXGIFormat(Format format);

  private:
    // ========== 成员变量 ==========

    // D3D12 核心对象
    ComPtr<ID3D12GraphicsCommandList> m_commandList;                // D3D12 命令列表
    D3D12Device*                      m_device           = nullptr; // 设备引用（不拥有）
    ID3D12CommandAllocator*           m_commandAllocator = nullptr; // 命令分配器（不拥有）
    D3D12_COMMAND_LIST_TYPE           m_type;                       // 命令列表类型

    // 录制状态
    bool m_isRecording   = false; // 是否正在录制
    bool m_isClosed      = false; // 是否已关闭
    bool m_isInitialized = false; // 是否已初始化

    // 当前绑定状态缓存
    D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology    = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D12PipelineState*     m_currentPipelineState = nullptr;
    ID3D12RootSignature*     m_currentRootSignature = nullptr;

    // 当前绑定的描述符堆
    ID3D12DescriptorHeap* m_descriptorHeaps[2] = {nullptr, nullptr}; // CBV_SRV_UAV, Sampler

    // 索引缓冲区格式缓存
    DXGI_FORMAT m_indexBufferFormat = DXGI_FORMAT_R32_UINT;

    // 调试名称
    std::string m_debugName;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12