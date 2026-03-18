// =============================================================================
// D3D12PipelineState.cpp - D3D12 管线状态实现
// =============================================================================

#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"

#if ENGINE_RHI_D3D12

namespace Engine::RHI::D3D12
{

// =============================================================================
// 辅助函数：RHI 枚举转换
// =============================================================================

namespace
{

DXGI_FORMAT ConvertFormat(Format format)
{
    switch (format)
    {
    case Format::Unknown:         return DXGI_FORMAT_UNKNOWN;
    case Format::R32G32B32A32Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case Format::R32G32B32Float:    return DXGI_FORMAT_R32G32B32_FLOAT;
    case Format::R16G16B16A16Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case Format::R32G32Float:       return DXGI_FORMAT_R32G32_FLOAT;
    case Format::R8G8B8A8Unorm:     return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Format::B8G8R8A8Unorm:     return DXGI_FORMAT_B8G8R8A8_UNORM;
    case Format::R16G16Float:       return DXGI_FORMAT_R16G16_FLOAT;
    case Format::R32Float:          return DXGI_FORMAT_R32_FLOAT;
    case Format::R16Float:          return DXGI_FORMAT_R16_FLOAT;
    case Format::D32Float:          return DXGI_FORMAT_D32_FLOAT;
    case Format::D24UnormS8Uint:    return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Format::D32FloatS8X24Uint: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    default:                        return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::PointList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case PrimitiveTopology::LineList:
    case PrimitiveTopology::LineStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case PrimitiveTopology::TriangleList:
    case PrimitiveTopology::TriangleStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case PrimitiveTopology::PatchList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    default:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

D3D12_FILL_MODE ConvertFillMode(FillMode mode)
{
    return (mode == FillMode::Wireframe) ? D3D12_FILL_MODE_WIREFRAME 
                                         : D3D12_FILL_MODE_SOLID;
}

D3D12_CULL_MODE ConvertCullMode(CullMode mode)
{
    switch (mode)
    {
    case CullMode::None:  return D3D12_CULL_MODE_NONE;
    case CullMode::Front: return D3D12_CULL_MODE_FRONT;
    case CullMode::Back:  return D3D12_CULL_MODE_BACK;
    default:              return D3D12_CULL_MODE_BACK;
    }
}

D3D12_COMPARISON_FUNC ConvertCompareFunc(CompareFunc func)
{
    switch (func)
    {
    case CompareFunc::Never:        return D3D12_COMPARISON_FUNC_NEVER;
    case CompareFunc::Less:         return D3D12_COMPARISON_FUNC_LESS;
    case CompareFunc::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case CompareFunc::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case CompareFunc::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case CompareFunc::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case CompareFunc::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case CompareFunc::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    default:                        return D3D12_COMPARISON_FUNC_LESS;
    }
}

D3D12_BLEND_OP ConvertBlendOp(BlendOp op)
{
    switch (op)
    {
    case BlendOp::Add:          return D3D12_BLEND_OP_ADD;
    case BlendOp::Subtract:     return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::RevSubtract:  return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Min:          return D3D12_BLEND_OP_MIN;
    case BlendOp::Max:          return D3D12_BLEND_OP_MAX;
    default:                    return D3D12_BLEND_OP_ADD;
    }
}

D3D12_BLEND ConvertBlendFactor(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::Zero:             return D3D12_BLEND_ZERO;
    case BlendFactor::One:              return D3D12_BLEND_ONE;
    case BlendFactor::SrcColor:         return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::InvSrcColor:      return D3D12_BLEND_INV_SRC_COLOR;
    case BlendFactor::SrcAlpha:         return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::InvSrcAlpha:      return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::DestAlpha:        return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::InvDestAlpha:     return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendFactor::DestColor:        return D3D12_BLEND_DEST_COLOR;
    case BlendFactor::InvDestColor:     return D3D12_BLEND_INV_DEST_COLOR;
    case BlendFactor::SrcAlphaSaturate: return D3D12_BLEND_SRC_ALPHA_SAT;
    default:                            return D3D12_BLEND_ONE;
    }
}

D3D12_INPUT_CLASSIFICATION ConvertInputClassification(bool instanceData)
{
    return instanceData ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA 
                        : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
}

} // anonymous namespace

// =============================================================================
// 构造/析构
// =============================================================================

D3D12PipelineState::D3D12PipelineState()
{
}

D3D12PipelineState::~D3D12PipelineState()
{
    Shutdown();
}

D3D12PipelineState::D3D12PipelineState(D3D12PipelineState&& other) noexcept
    : m_pipelineState(std::move(other.m_pipelineState)),
      m_isCompute(other.m_isCompute),
      m_debugName(std::move(other.m_debugName))
{
    other.m_isCompute = false;
}

D3D12PipelineState& D3D12PipelineState::operator=(D3D12PipelineState&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();
        m_pipelineState   = std::move(other.m_pipelineState);
        m_isCompute       = other.m_isCompute;
        m_debugName       = std::move(other.m_debugName);
        other.m_isCompute = false;
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12PipelineState::InitializeGraphics(D3D12Device* device, const GraphicsPipelineDesc& desc)
{
    if (!device)
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeGraphics: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    if (!desc.vertexShader || !desc.pixelShader)
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeGraphics: 需要顶点和像素着色器");
        return RHIResult::InvalidParameter;
    }

    if (!desc.rootSignature)
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeGraphics: 需要根签名");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    D3D12RootSignature* d3dRootSig = static_cast<D3D12RootSignature*>(desc.rootSignature);

    // 创建图形管线状态描述
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = d3dRootSig->GetD3D12RootSignature();

    // 着色器
    psoDesc.VS.pShaderBytecode = desc.vertexShader->GetBytecode();
    psoDesc.VS.BytecodeLength = desc.vertexShader->GetBytecodeSize();

    psoDesc.PS.pShaderBytecode = desc.pixelShader->GetBytecode();
    psoDesc.PS.BytecodeLength = desc.pixelShader->GetBytecodeSize();

    if (desc.geometryShader)
    {
        psoDesc.GS.pShaderBytecode = desc.geometryShader->GetBytecode();
        psoDesc.GS.BytecodeLength = desc.geometryShader->GetBytecodeSize();
    }

    if (desc.hullShader)
    {
        psoDesc.HS.pShaderBytecode = desc.hullShader->GetBytecode();
        psoDesc.HS.BytecodeLength = desc.hullShader->GetBytecodeSize();
    }

    if (desc.domainShader)
    {
        psoDesc.DS.pShaderBytecode = desc.domainShader->GetBytecode();
        psoDesc.DS.BytecodeLength = desc.domainShader->GetBytecodeSize();
    }

    // 输入布局
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    inputElements.reserve(desc.inputLayout.size());

    for (const auto& elem : desc.inputLayout)
    {
        D3D12_INPUT_ELEMENT_DESC d3dElem = {};
        d3dElem.SemanticName = elem.semanticName;
        d3dElem.SemanticIndex = elem.semanticIndex;
        d3dElem.Format = ConvertFormat(elem.format);
        d3dElem.InputSlot = elem.inputSlot;
        d3dElem.AlignedByteOffset = elem.alignedByteOffset;
        d3dElem.InputSlotClass = ConvertInputClassification(elem.instanceData);
        d3dElem.InstanceDataStepRate = elem.instanceData ? elem.instanceDataStepRate : 0;

        inputElements.push_back(d3dElem);
    }

    psoDesc.InputLayout.pInputElementDescs = inputElements.empty() ? nullptr : inputElements.data();
    psoDesc.InputLayout.NumElements = static_cast<UINT>(inputElements.size());

    // 图元拓扑
    psoDesc.PrimitiveTopologyType = ConvertPrimitiveTopologyType(desc.topology);

    // 光栅化状态
    psoDesc.RasterizerState.FillMode = ConvertFillMode(desc.rasterizerState.fillMode);
    psoDesc.RasterizerState.CullMode = ConvertCullMode(desc.rasterizerState.cullMode);
    psoDesc.RasterizerState.FrontCounterClockwise = desc.rasterizerState.frontCounterClockwise;
    psoDesc.RasterizerState.DepthBias = desc.rasterizerState.depthBias;
    psoDesc.RasterizerState.DepthBiasClamp = desc.rasterizerState.depthBiasClamp;
    psoDesc.RasterizerState.SlopeScaledDepthBias = desc.rasterizerState.slopeScaledDepthBias;
    psoDesc.RasterizerState.DepthClipEnable = desc.rasterizerState.depthClipEnable;
    psoDesc.RasterizerState.MultisampleEnable = desc.rasterizerState.multisampleEnable;
    psoDesc.RasterizerState.AntialiasedLineEnable = desc.rasterizerState.antialiasedLineEnable;

    // 深度模板状态
    psoDesc.DepthStencilState.DepthEnable = desc.depthStencilState.depthEnable;
    psoDesc.DepthStencilState.DepthWriteMask = desc.depthStencilState.depthWriteEnable 
        ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = ConvertCompareFunc(desc.depthStencilState.depthFunc);
    psoDesc.DepthStencilState.StencilEnable = desc.depthStencilState.stencilEnable;
    psoDesc.DepthStencilState.StencilReadMask = desc.depthStencilState.stencilReadMask;
    psoDesc.DepthStencilState.StencilWriteMask = desc.depthStencilState.stencilWriteMask;

    // 混合状态
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC& rtBlend = psoDesc.BlendState.RenderTarget[0];
    rtBlend.BlendEnable = desc.blendState.blendEnable;
    rtBlend.BlendOp = ConvertBlendOp(desc.blendState.blendOp);
    rtBlend.BlendOpAlpha = ConvertBlendOp(desc.blendState.blendOpAlpha);
    rtBlend.SrcBlend = ConvertBlendFactor(desc.blendState.srcBlend);
    rtBlend.DestBlend = ConvertBlendFactor(desc.blendState.destBlend);
    rtBlend.SrcBlendAlpha = ConvertBlendFactor(desc.blendState.srcBlendAlpha);
    rtBlend.DestBlendAlpha = ConvertBlendFactor(desc.blendState.destBlendAlpha);
    rtBlend.RenderTargetWriteMask = desc.blendState.renderTargetWriteMask;

    // 渲染目标格式
    psoDesc.NumRenderTargets = static_cast<UINT>(desc.renderTargetFormats.size());
    for (size_t i = 0; i < desc.renderTargetFormats.size() && i < 8; ++i)
    {
        psoDesc.RTVFormats[i] = ConvertFormat(desc.renderTargetFormats[i]);
    }

    psoDesc.DSVFormat = ConvertFormat(desc.depthStencilFormat);

    // 多重采样
    psoDesc.SampleDesc.Count = desc.sampleCount;
    psoDesc.SampleDesc.Quality = desc.sampleQuality;
    psoDesc.SampleMask = 0xFFFFFFFF;

    // 创建 PSO
    HRESULT hr = d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeGraphics: 创建管线状态失败, HRESULT=0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }

    m_isCompute = false;

    // 设置调试名称
    if (desc.debugName)
    {
        m_debugName = desc.debugName;
        m_pipelineState->SetName(StringToWString(desc.debugName).c_str());
    }

    RHI_LOG_INFO("D3D12PipelineState::InitializeGraphics: 成功创建图形管线状态");
    return RHIResult::Success;
}

RHIResult D3D12PipelineState::InitializeCompute(D3D12Device* device, const ComputePipelineDesc& desc)
{
    if (!device)
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeCompute: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    if (!desc.computeShader)
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeCompute: 需要计算着色器");
        return RHIResult::InvalidParameter;
    }

    if (!desc.rootSignature)
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeCompute: 需要根签名");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    D3D12RootSignature* d3dRootSig = static_cast<D3D12RootSignature*>(desc.rootSignature);

    // 创建计算管线状态描述
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = d3dRootSig->GetD3D12RootSignature();
    psoDesc.CS.pShaderBytecode = desc.computeShader->GetBytecode();
    psoDesc.CS.BytecodeLength = desc.computeShader->GetBytecodeSize();
    psoDesc.NodeMask = 0;
    psoDesc.CachedPSO = {};
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    // 创建 PSO
    HRESULT hr = d3dDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12PipelineState::InitializeCompute: 创建计算管线状态失败, HRESULT=0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }

    m_isCompute = true;

    // 设置调试名称
    if (desc.debugName)
    {
        m_debugName = desc.debugName;
        m_pipelineState->SetName(StringToWString(desc.debugName).c_str());
    }

    RHI_LOG_INFO("D3D12PipelineState::InitializeCompute: 成功创建计算管线状态");
    return RHIResult::Success;
}

void D3D12PipelineState::Shutdown()
{
    m_pipelineState.Reset();
    m_isCompute = false;
    m_debugName.clear();
}

// =============================================================================
// 接口实现
// =============================================================================

void* D3D12PipelineState::GetNativeHandle() const
{
    return m_pipelineState.Get();
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12