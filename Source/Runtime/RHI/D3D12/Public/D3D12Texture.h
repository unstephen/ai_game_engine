// =============================================================================
// D3D12Texture.h - D3D12 纹理实现
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "D3D12Device.h"

namespace Engine::RHI::D3D12 {

// =============================================================================
// D3D12Texture - D3D12 纹理实现
// =============================================================================

/**
 * @brief D3D12 纹理实现
 * 
 * 支持多种纹理类型：
 * - 2D 纹理
 * - 2D 纹理数组
 * - CubeMap 纹理
 * 
 * 支持资源视图：
 * - SRV (Shader Resource View) - 着色器资源视图
 * - RTV (Render Target View) - 渲染目标视图
 * - DSV (Depth Stencil View) - 深度模板视图
 * - UAV (Unordered Access View) - 无序访问视图
 * 
 * 支持两种堆类型：
 * - 上传堆 (Upload Heap): CPU 可访问，用于动态纹理
 * - 默认堆 (Default Heap): GPU 优化，用于静态纹理
 */
class D3D12Texture : public ITexture {
public:
    // ========== 构造/析构 ==========
    
    D3D12Texture();
    virtual ~D3D12Texture() override;
    
    // 禁止拷贝
    D3D12Texture(const D3D12Texture&) = delete;
    D3D12Texture& operator=(const D3D12Texture&) = delete;
    
    // 允许移动
    D3D12Texture(D3D12Texture&& other) noexcept;
    D3D12Texture& operator=(D3D12Texture&& other) noexcept;
    
    // ========== 初始化 ==========
    
    /**
     * @brief 初始化纹理
     * @param device D3D12 设备指针
     * @param desc 纹理描述
     * @return RHIResult 结果码
     */
    RHIResult Initialize(D3D12Device* device, const TextureDesc& desc);
    
    /**
     * @brief 关闭纹理，释放资源
     */
    void Shutdown();
    
    // ========== ITexture 接口实现 ==========
    
    virtual uint32_t GetWidth() const override { return m_width; }
    virtual uint32_t GetHeight() const override { return m_height; }
    virtual uint32_t GetDepth() const override { return m_depth; }
    virtual uint32_t GetMipLevels() const override { return m_mipLevels; }
    virtual uint32_t GetArraySize() const override { return m_arraySize; }
    virtual Format GetFormat() const override { return m_format; }
    virtual TextureUsage GetUsage() const override { return m_usage; }
    
    // ========== 资源视图创建 ==========
    
    /**
     * @brief 创建着色器资源视图 (SRV)
     * @param device D3D12 设备
     * @param handle CPU 描述符句柄
     * @param mostDetailedMip 最详细的 mip 级别（默认 0）
     * @param mipLevels mip 级别数（默认 -1 表示全部）
     * @return RHIResult 结果码
     */
    RHIResult CreateSRV(
        D3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE handle,
        uint32_t mostDetailedMip = 0,
        int32_t mipLevels = -1);
    
    /**
     * @brief 创建渲染目标视图 (RTV)
     * @param device D3D12 设备
     * @param handle CPU 描述符句柄
     * @param mipSlice mip 切片（默认 0）
     * @param arraySlice 数组切片（默认 0）
     * @return RHIResult 结果码
     */
    RHIResult CreateRTV(
        D3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE handle,
        uint32_t mipSlice = 0,
        uint32_t arraySlice = 0);
    
    /**
     * @brief 创建深度模板视图 (DSV)
     * @param device D3D12 设备
     * @param handle CPU 描述符句柄
     * @param mipSlice mip 切片（默认 0）
     * @return RHIResult 结果码
     */
    RHIResult CreateDSV(
        D3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE handle,
        uint32_t mipSlice = 0);
    
    /**
     * @brief 创建无序访问视图 (UAV)
     * @param device D3D12 设备
     * @param handle CPU 描述符句柄
     * @param mipSlice mip 切片（默认 0）
     * @return RHIResult 结果码
     */
    RHIResult CreateUAV(
        D3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE handle,
        uint32_t mipSlice = 0);
    
    // ========== 数据上传 ==========
    
    /**
     * @brief 上传纹理数据（用于默认堆纹理）
     * @param device D3D12 设备
     * @param commandList 命令列表
     * @param data 数据指针
     * @param dataSize 数据大小
     * @param rowPitch 行间距（字节）
     * @param arraySlice 数组切片（默认 0）
     * @param mipLevel mip 级别（默认 0）
     * @return RHIResult 结果码
     */
    RHIResult UploadData(
        D3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const void* data,
        size_t dataSize,
        uint32_t rowPitch,
        uint32_t arraySlice = 0,
        uint32_t mipLevel = 0);
    
    /**
     * @brief 映射纹理到 CPU 可访问内存
     * @param subresource 子资源索引
     * @param range 可选的读取范围
     * @return 映射后的指针，失败返回 nullptr
     */
    void* Map(uint32_t subresource = 0, const D3D12_RANGE* range = nullptr);
    
    /**
     * @brief 取消映射
     * @param subresource 子资源索引
     * @param range 可选的写入范围
     */
    void Unmap(uint32_t subresource = 0, const D3D12_RANGE* range = nullptr);
    
    // ========== D3D12 特定接口 ==========
    
    /**
     * @brief 获取 D3D12 资源
     * @return ID3D12Resource* D3D12 资源指针
     */
    ID3D12Resource* GetD3D12Resource() const { return m_resource.Get(); }
    
    /**
     * @brief 检查是否为 CPU 可访问（上传堆）
     * @return true 如果是上传堆
     */
    bool IsCPUAccessible() const { return m_cpuAccessible; }
    
    /**
     * @brief 获取 DXGI 格式
     * @return DXGI_FORMAT DXGI 格式
     */
    DXGI_FORMAT GetDXGIFormat() const { return m_dxgiFormat; }
    
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
    
    /**
     * @brief 获取指定 mip 级别的宽度
     * @param mipLevel mip 级别
     * @return 宽度
     */
    uint32_t GetWidth(uint32_t mipLevel) const;
    
    /**
     * @brief 获取指定 mip 级别的高度
     * @param mipLevel mip 级别
     * @return 高度
     */
    uint32_t GetHeight(uint32_t mipLevel) const;
    
    /**
     * @brief 获取子资源数量
     * @return 子资源数量 (mipLevels * arraySize)
     */
    uint32_t GetSubresourceCount() const;
    
    /**
     * @brief 计算子资源索引
     * @param mipLevel mip 级别
     * @param arraySlice 数组切片
     * @return 子资源索引
     */
    uint32_t CalcSubresource(uint32_t mipLevel, uint32_t arraySlice) const;
    
    /**
     * @brief 检查是否为 CubeMap
     * @return true 如果是 CubeMap
     */
    bool IsCubeMap() const { return m_isCubeMap; }
    
private:
    // ========== 内部方法 ==========
    
    /**
     * @brief 创建 D3D12 资源
     * @param device D3D12 设备
     * @param desc 纹理描述
     * @return RHIResult 结果码
     */
    RHIResult CreateResource(D3D12Device* device, const TextureDesc& desc);
    
    /**
     * @brief 根据用途确定资源状态
     * @param usage 纹理用途
     * @param initialState 初始状态
     * @return D3D12_RESOURCE_STATES 资源状态
     */
    static D3D12_RESOURCE_STATES DetermineResourceState(
        TextureUsage usage, 
        ResourceState initialState);
    
    /**
     * @brief 根据用途确定资源标志
     * @param usage 纹理用途
     * @return D3D12_RESOURCE_FLAGS 资源标志
     */
    static D3D12_RESOURCE_FLAGS DetermineResourceFlags(TextureUsage usage);
    
    /**
     * @brief 将 RHI 格式转换为 DXGI 格式
     * @param format RHI 格式
     * @return DXGI_FORMAT DXGI 格式
     */
    static DXGI_FORMAT ToDXGIFormat(Format format);
    
    /**
     * @brief 获取格式的字节每像素
     * @param format RHI 格式
     * @return 字节每像素
     */
    static uint32_t GetBytesPerPixel(Format format);
    
    /**
     * @brief 计算 mip 级别数
     * @param width 宽度
     * @param height 高度
     * @return mip 级别数
     */
    static uint32_t CalcMipLevels(uint32_t width, uint32_t height);
    
private:
    // ========== 成员变量 ==========
    
    // D3D12 资源
    ComPtr<ID3D12Resource> m_resource;
    
    // 纹理属性
    uint32_t m_width = 0;                   // 宽度
    uint32_t m_height = 0;                 // 高度
    uint32_t m_depth = 1;                  // 深度（3D 纹理）
    uint32_t m_mipLevels = 1;              // mip 级别数
    uint32_t m_arraySize = 1;              // 数组大小
    Format m_format = Format::Unknown;     // 像素格式
    TextureUsage m_usage = TextureUsage::None; // 纹理用途
    
    // DXGI 格式
    DXGI_FORMAT m_dxgiFormat = DXGI_FORMAT_UNKNOWN;
    
    // 映射状态
    void* m_mappedData = nullptr;          // 映射后的 CPU 指针
    bool m_isMapped = false;               // 是否已映射
    bool m_cpuAccessible = false;          // 是否为上传堆（CPU 可访问）
    bool m_isCubeMap = false;              // 是否为 CubeMap
    
    // 资源状态
    D3D12_RESOURCE_STATES m_resourceState = D3D12_RESOURCE_STATE_COMMON;
    
    // 调试名称
    std::string m_debugName;
    
    // 上传缓冲区（用于默认堆纹理上传）
    ComPtr<ID3D12Resource> m_uploadBuffer;
    uint64_t m_uploadBufferSize = 0;    // 上传缓冲区大小
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12