// =============================================================================
// D3D12SwapChain.h - D3D12 交换链实现
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "D3D12Device.h"
#include "D3D12Texture.h"

namespace Engine::RHI::D3D12
{

// =============================================================================
// D3D12SwapChain - D3D12 交换链实现
// =============================================================================

/**
 * @brief D3D12 交换链实现
 *
 * 功能：
 * - DXGI 交换链创建（FLIP_DISCARD 模式）
 * - 后备缓冲区访问
 * - RTV 创建（每个后备缓冲区）
 * - Present 呈现（支持 VSync 控制）
 * - Resize 调整大小
 * - 帧索引追踪
 *
 * 使用 ComPtr 管理 COM 对象，确保资源安全释放。
 */
class D3D12SwapChain : public ISwapChain
{
  public:
    // ========== 构造/析构 ==========

    D3D12SwapChain();
    virtual ~D3D12SwapChain() override;

    // 禁止拷贝
    D3D12SwapChain(const D3D12SwapChain&) = delete;
    D3D12SwapChain& operator=(const D3D12SwapChain&) = delete;

    // 允许移动
    D3D12SwapChain(D3D12SwapChain&& other) noexcept;
    D3D12SwapChain& operator=(D3D12SwapChain&& other) noexcept;

    // ========== 初始化 ==========

    /**
     * @brief 初始化交换链
     * @param device D3D12 设备指针
     * @param desc 交换链描述
     * @return RHIResult 结果码
     */
    RHIResult Initialize(D3D12Device* device, const SwapChainDesc& desc);

    /**
     * @brief 关闭交换链，释放资源
     */
    void Shutdown();

    // ========== ISwapChain 接口实现 ==========

    virtual uint32_t GetCurrentBackBufferIndex() const override;
    virtual uint32_t GetBufferCount() const override { return m_bufferCount; }
    virtual uint32_t GetWidth() const override { return m_width; }
    virtual uint32_t GetHeight() const override { return m_height; }
    virtual Format   GetFormat() const override { return m_format; }

    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual void Present() override;

    virtual ITexture* GetBackBuffer(uint32_t index) override;

    // ========== 扩展接口 ==========

    /**
     * @brief 设置 VSync 开关
     * @param enable 是否启用 VSync
     */
    void SetVSync(bool enable) { m_vsync = enable; }

    /**
     * @brief 检查是否启用 VSync
     * @return true 如果启用 VSync
     */
    bool IsVSyncEnabled() const { return m_vsync; }

    /**
     * @brief 获取当前后备缓冲区的 RTV 句柄
     * @return D3D12_CPU_DESCRIPTOR_HANDLE RTV 句柄
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

    /**
     * @brief 获取指定索引的 RTV 句柄
     * @param index 后备缓冲区索引
     * @return D3D12_CPU_DESCRIPTOR_HANDLE RTV 句柄
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(uint32_t index) const;

    // ========== D3D12 特定接口 ==========

    /**
     * @brief 获取 DXGI 交换链
     * @return IDXGISwapChain3* DXGI 交换链指针
     */
    IDXGISwapChain3* GetDXGISwapChain() const { return m_swapChain.Get(); }

    /**
     * @brief 获取 RTV 描述符堆
     * @return ID3D12DescriptorHeap* RTV 描述符堆指针
     */
    ID3D12DescriptorHeap* GetRTVHeap() const { return m_rtvHeap.Get(); }

    /**
     * @brief 获取当前后备缓冲区的 D3D12 资源
     * @return ID3D12Resource* D3D12 资源指针
     */
    ID3D12Resource* GetCurrentBackBufferResource() const;

    /**
     * @brief 获取指定索引的后备缓冲区的 D3D12 资源
     * @param index 后备缓冲区索引
     * @return ID3D12Resource* D3D12 资源指针
     */
    ID3D12Resource* GetBackBufferResource(uint32_t index) const;

  private:
    // ========== 内部方法 ==========

    /**
     * @brief 创建 DXGI 交换链
     * @param device D3D12 设备
     * @param windowHandle 窗口句柄
     * @return RHIResult 结果码
     */
    RHIResult CreateSwapChain(D3D12Device* device, void* windowHandle);

    /**
     * @brief 创建渲染目标视图
     * @param device D3D12 设备
     * @return RHIResult 结果码
     */
    RHIResult CreateRenderTargetViews(D3D12Device* device);

    /**
     * @brief 释放后备缓冲区资源
     */
    void ReleaseBackBuffers();

    /**
     * @brief 将 RHI 格式转换为 DXGI 格式
     * @param format RHI 格式
     * @return DXGI_FORMAT DXGI 格式
     */
    static DXGI_FORMAT ToDXGIFormat(Format format);

    /**
     * @brief 检查是否支持撕裂
     * @param factory DXGI 工厂
     * @return true 如果支持撕裂
     */
    static bool CheckTearingSupport(IDXGIFactory4* factory);

  private:
    // ========== 成员变量 ==========

    // DXGI 交换链
    ComPtr<IDXGISwapChain3> m_swapChain;

    // 后备缓冲区
    std::vector<ComPtr<ID3D12Resource>>        m_backBuffers;
    std::vector<std::unique_ptr<D3D12Texture>> m_backBufferTextures;

    // RTV 描述符堆
    ComPtr<ID3D12DescriptorHeap>             m_rtvHeap;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;

    // 交换链属性
    uint32_t m_width        = 0;                      // 宽度
    uint32_t m_height       = 0;                      // 高度
    uint32_t m_bufferCount  = 2;                      // 缓冲区数量
    Format   m_format       = Format::R8G8B8A8_UNorm; // 像素格式
    bool     m_vsync        = true;                   // VSync 开关
    bool     m_allowTearing = false;                  // 是否允许撕裂

    // 当前帧索引
    uint32_t m_currentFrameIndex = 0;

    // 设备引用（不拥有）
    D3D12Device* m_device = nullptr;

    // 窗口句柄
    void* m_windowHandle = nullptr;

    // 调试名称
    std::string m_debugName;

    // RTV 描述符增量大小
    uint32_t m_rtvDescriptorSize = 0;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12