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
// 根参数类型
// =============================================================================

enum class RootParameterType : uint8_t
{
    Constants,       // 32位常量
    CBV,             // 常量缓冲区视图
    SRV,             // 着色器资源视图
    UAV,             // 无序访问视图
    DescriptorTable  // 描述符表
};

// =============================================================================
// 描述符范围描述
// =============================================================================

struct DescriptorRangeDesc
{
    DescriptorType type             = DescriptorType::SRV;
    uint32_t       numDescriptors   = 1;
    uint32_t       baseShaderRegister = 0;
    uint32_t       registerSpace    = 0;
    uint32_t       offset           = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
};

// =============================================================================
// 根参数描述
// =============================================================================

struct RootParameterDesc
{
    RootParameterType type             = RootParameterType::DescriptorTable;
    uint32_t          shaderRegister   = 0;
    uint32_t          registerSpace    = 0;
    uint32_t          num32BitValues   = 0; // 仅用于 Constants
    ShaderStage       visibility       = ShaderStage::All;

    // 描述符范围（仅用于 DescriptorTable）
    std::vector<DescriptorRangeDesc> ranges;
};

// =============================================================================
// 静态采样器描述
// =============================================================================

struct StaticSamplerDesc
{
    uint32_t    shaderRegister = 0;
    uint32_t    registerSpace  = 0;
    ShaderStage visibility     = ShaderStage::All;
};

// =============================================================================
// 根签名描述
// =============================================================================

struct RootSignatureDesc
{
    std::vector<RootParameterDesc>  parameters;
    std::vector<StaticSamplerDesc>  staticSamplers;
    const char*                     debugName = nullptr;
};

// =============================================================================
// D3D12RootSignature - D3D12 根签名实现
// =============================================================================

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

    RHIResult Initialize(D3D12Device* device, const RootSignatureDesc& desc);
    RHIResult InitializeFromBytecode(D3D12Device* device, const void* bytecode, size_t size);
    void Shutdown();

    // ========== IRootSignature 接口实现 ==========

    virtual void* GetNativeHandle() const override;

    // ========== D3D12 特定接口 ==========

    ID3D12RootSignature* GetD3D12RootSignature() const { return m_rootSignature.Get(); }
    uint32_t GetNumParameters() const { return m_numParameters; }

  private:
    ComPtr<ID3D12RootSignature> m_rootSignature;
    uint32_t m_numParameters = 0;
    std::string m_debugName;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12