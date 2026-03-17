// =============================================================================
// D3D12PipelineState.h - D3D12 管线状态实现
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

#include "D3D12Device.h"

namespace Engine::RHI::D3D12
{

// =============================================================================
// D3D12PipelineState - D3D12 管线状态实现
// =============================================================================

/**
 * @brief D3D12 管线状态实现
 *
 * 支持图形管线和计算管线状态对象 (PSO)
 * - 图形管线：VS、PS、GS、HS、DS、输入布局、光栅化状态、混合状态等
 * - 计算管线：CS
 */
class D3D12PipelineState : public IPipelineState
{
  public:
    // ========== 构造/析构 ==========

    D3D12PipelineState();
    virtual ~D3D12PipelineState() override;

    // 禁止拷贝
    D3D12PipelineState(const D3D12PipelineState&) = delete;
    D3D12PipelineState& operator=(const D3D12PipelineState&) = delete;

    // 允许移动
    D3D12PipelineState(D3D12PipelineState&& other) noexcept;
    D3D12PipelineState& operator=(D3D12PipelineState&& other) noexcept;

    // ========== 初始化 ==========

    /**
     * @brief 初始化图形管线状态
     * @param device D3D12 设备指针
     * @param desc 图形管线描述
     * @return RHIResult 结果码
     */
    RHIResult InitializeGraphics(D3D12Device* device, const GraphicsPipelineDesc& desc);

    /**
     * @brief 初始化计算管线状态
     * @param device D3D12 设备指针
     * @param desc 计算管线描述
     * @return RHIResult 结果码
     */
    RHIResult InitializeCompute(D3D12Device* device, const ComputePipelineDesc& desc);

    /**
     * @brief 关闭管线状态，释放资源
     */
    void Shutdown();

    // ========== IPipelineState 接口实现 ==========

    virtual void* GetNativeHandle() const override;

    // ========== D3D12 特定接口 ==========

    /**
     * @brief 获取 D3D12 管线状态
     * @return ID3D12PipelineState* D3D12 管线状态指针
     */
    ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }

    /**
     * @brief 检查是否为计算管线
     * @return true 如果是计算管线
     */
    bool IsCompute() const { return m_isCompute; }

  private:
    // D3D12 管线状态
    ComPtr<ID3D12PipelineState> m_pipelineState;

    // 管线类型
    bool m_isCompute = false;

    // 调试名称
    std::string m_debugName;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12