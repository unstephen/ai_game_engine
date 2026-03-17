// =============================================================================
// D3D12RootSignature.h - D3D12 根签名实现
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "D3D12Device.h"

namespace Engine::RHI::D3D12
{

// =============================================================================
// D3D12RootSignature - D3D12 根签名实现
// =============================================================================

/**
 * @brief D3D12 根签名实现
 *
 * 根签名定义了着色器如何访问资源：
 * - 根常量：32 位常量值
 * - 根描述符：内联 CBV/SRV/UAV
 * - 描述符表：指向描述符堆的范围
 */
class D3D12RootSignature : public IRootSignature
{
  public:
    // ========== 构造/析构 ==========

    D3D12RootSignature();
    virtual ~D3D12RootSignature() override;

    // 禁止拷贝
    D3D12RootSignature(const D3D12RootSignature&) = delete;
    D3D12RootSignature& operator=(const D3D12RootSignature&) = delete;

    // 允许移动
    D3D12RootSignature(D3D12RootSignature&& other) noexcept;
    D3D12RootSignature& operator=(D3D12RootSignature&& other) noexcept;

    // ========== 初始化 ==========

    /**
     * @brief 初始化根签名
     * @param device D3D12 设备指针
     * @param desc 根签名描述
     * @return RHIResult 结果码
     */
    RHIResult Initialize(D3D12Device* device, const RootSignatureDesc& desc);

    /**
     * @brief 从字节码初始化根签名
     * @param device D3D12 设备指针
     * @param bytecode 根签名字节码
     * @param size 字节码大小
     * @return RHIResult 结果码
     */
    RHIResult InitializeFromBytecode(D3D12Device* device, const void* bytecode, size_t size);

    /**
     * @brief 关闭根签名，释放资源
     */
    void Shutdown();

    // ========== IRootSignature 接口实现 ==========

    virtual void* GetNativeHandle() const override;

    // ========== D3D12 特定接口 ==========

    /**
     * @brief 获取 D3D12 根签名
     * @return ID3D12RootSignature* D3D12 根签名指针
     */
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }

    /**
     * @brief 获取根参数数量
     * @return 根参数数量
     */
    uint32_t GetNumParameters() const { return m_numParameters; }

  private:
    // D3D12 根签名
    ComPtr<ID3D12RootSignature> m_rootSignature;

    // 根参数数量
    uint32_t m_numParameters = 0;

    // 调试名称
    std::string m_debugName;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12