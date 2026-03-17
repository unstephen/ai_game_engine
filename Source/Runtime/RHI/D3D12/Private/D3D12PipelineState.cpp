// =============================================================================
// D3D12PipelineState.cpp - D3D12 管线状态实现
// =============================================================================

#include "D3D12PipelineState.h"

#if ENGINE_RHI_D3D12

namespace Engine::RHI::D3D12 {

// =============================================================================
// 构造/析构
// =============================================================================

D3D12PipelineState::D3D12PipelineState() {
}

D3D12PipelineState::~D3D12PipelineState() {
    Shutdown();
}

D3D12PipelineState::D3D12PipelineState(D3D12PipelineState&& other) noexcept
    : m_pipelineState(std::move(other.m_pipelineState))
    , m_isCompute(other.m_isCompute)
    , m_debugName(std::move(other.m_debugName))
{
    other.m_isCompute = false;
}

D3D12PipelineState& D3D12PipelineState::operator=(D3D12PipelineState&& other) noexcept {
    if (this != &other) {
        Shutdown();
        m_pipelineState = std::move(other.m_pipelineState);
        m_isCompute = other.m_isCompute;
        m_debugName = std::move(other.m_debugName);
        other.m_isCompute = false;
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12PipelineState::InitializeGraphics(
    D3D12Device* device,
    const GraphicsPipelineDesc& desc)
{
    if (!device) {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeGraphics: 设备指针为空");
        return RHIResult::InvalidParameter;
    }
    
    // TODO: 完整实现需要处理：
    // - 着色器编译和绑定
    // - 输入布局
    // - 光栅化状态
    // - 深度模板状态
    // - 混合状态
    // - 渲染目标格式
    
    RHI_LOG_WARNING("D3D12PipelineState::InitializeGraphics: 完整实现待完成");
    return RHIResult::NotSupported;
}

RHIResult D3D12PipelineState::InitializeCompute(
    D3D12Device* device,
    const ComputePipelineDesc& desc)
{
    if (!device) {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeCompute: 设备指针为空");
        return RHIResult::InvalidParameter;
    }
    
    // TODO: 完整实现需要处理：
    // - 计算着色器编译
    // - 根签名绑定
    
    RHI_LOG_WARNING("D3D12PipelineState::InitializeCompute: 完整实现待完成");
    return RHIResult::NotSupported;
}

void D3D12PipelineState::Shutdown() {
    m_pipelineState.Reset();
    m_isCompute = false;
    m_debugName.clear();
}

// =============================================================================
// 接口实现
// =============================================================================

void* D3D12PipelineState::GetNativeHandle() const {
    return m_pipelineState.Get();
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12