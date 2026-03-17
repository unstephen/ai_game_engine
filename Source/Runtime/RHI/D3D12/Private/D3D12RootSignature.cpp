// =============================================================================
// D3D12RootSignature.cpp - D3D12 根签名实现
// =============================================================================

#include "D3D12RootSignature.h"

#if ENGINE_RHI_D3D12

namespace Engine::RHI::D3D12 {

// =============================================================================
// 构造/析构
// =============================================================================

D3D12RootSignature::D3D12RootSignature() {
}

D3D12RootSignature::~D3D12RootSignature() {
    Shutdown();
}

D3D12RootSignature::D3D12RootSignature(D3D12RootSignature&& other) noexcept
    : m_rootSignature(std::move(other.m_rootSignature))
    , m_numParameters(other.m_numParameters)
    , m_debugName(std::move(other.m_debugName))
{
    other.m_numParameters = 0;
}

D3D12RootSignature& D3D12RootSignature::operator=(D3D12RootSignature&& other) noexcept {
    if (this != &other) {
        Shutdown();
        m_rootSignature = std::move(other.m_rootSignature);
        m_numParameters = other.m_numParameters;
        m_debugName = std::move(other.m_debugName);
        other.m_numParameters = 0;
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12RootSignature::Initialize(
    D3D12Device* device,
    const RootSignatureDesc& desc)
{
    if (!device) {
        RHI_LOG_ERROR("D3D12RootSignature::Initialize: 设备指针为空");
        return RHIResult::InvalidParameter;
    }
    
    // TODO: 完整实现需要处理：
    // - 根参数配置
    // - 静态采样器配置
    // - 标志设置
    
    RHI_LOG_WARNING("D3D12RootSignature::Initialize: 完整实现待完成");
    return RHIResult::NotSupported;
}

RHIResult D3D12RootSignature::InitializeFromBytecode(
    D3D12Device* device,
    const void* bytecode,
    size_t size)
{
    if (!device) {
        RHI_LOG_ERROR("D3D12RootSignature::InitializeFromBytecode: 设备指针为空");
        return RHIResult::InvalidParameter;
    }
    
    if (!bytecode || size == 0) {
        RHI_LOG_ERROR("D3D12RootSignature::InitializeFromBytecode: 字节码无效");
        return RHIResult::InvalidParameter;
    }
    
    ID3D12Device* d3dDevice = device->GetD3D12Device();
    
    HRESULT hr = d3dDevice->CreateRootSignature(
        0,  // nodeMask
        bytecode,
        size,
        IID_PPV_ARGS(&m_rootSignature)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12RootSignature::InitializeFromBytecode: 创建根签名失败, HRESULT=0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }
    
    return RHIResult::Success;
}

void D3D12RootSignature::Shutdown() {
    m_rootSignature.Reset();
    m_numParameters = 0;
    m_debugName.clear();
}

// =============================================================================
// 接口实现
// =============================================================================

void* D3D12RootSignature::GetNativeHandle() const {
    return m_rootSignature.Get();
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12