// =============================================================================
// IPipelineState.h - 管线状态接口
// =============================================================================

#pragma once

#include "RHICore.h"

#include <vector>

namespace Engine::RHI
{

// =============================================================================
// 输入元素描述
// =============================================================================

struct InputElementDesc
{
    const char* semanticName         = "";
    uint32_t    semanticIndex        = 0;
    Format      format               = Format::Unknown;
    uint32_t    inputSlot            = 0;
    uint32_t    alignedByteOffset    = 0;
    bool        instanceData         = false;
    uint32_t    instanceDataStepRate = 0;
};

// =============================================================================
// 光栅化状态
// =============================================================================

struct RasterizerState
{
    FillMode fillMode              = FillMode::Solid;
    CullMode cullMode              = CullMode::Back;
    bool     frontCounterClockwise = false;
    int32_t  depthBias             = 0;
    float    depthBiasClamp        = 0.0f;
    float    slopeScaledDepthBias  = 0.0f;
    bool     depthClipEnable       = true;
    bool     multisampleEnable     = false;
    bool     antialiasedLineEnable = false;
};

// =============================================================================
// 深度模板状态
// =============================================================================

struct DepthStencilState
{
    bool        depthEnable      = true;
    bool        depthWriteEnable = true;
    CompareFunc depthFunc        = CompareFunc::Less;
    bool        stencilEnable    = false;
    uint8_t     stencilReadMask  = 0xFF;
    uint8_t     stencilWriteMask = 0xFF;
    // TODO: 模板操作
};

// =============================================================================
// 混合状态
// =============================================================================

struct BlendState
{
    bool        blendEnable           = false;
    BlendOp     blendOp               = BlendOp::Add;
    BlendOp     blendOpAlpha          = BlendOp::Add;
    BlendFactor srcBlend              = BlendFactor::One;
    BlendFactor destBlend             = BlendFactor::Zero;
    BlendFactor srcBlendAlpha         = BlendFactor::One;
    BlendFactor destBlendAlpha        = BlendFactor::Zero;
    uint8_t     renderTargetWriteMask = 0xFF;
};

// =============================================================================
// 图形管线描述
// =============================================================================

struct GraphicsPipelineDesc
{
    // 着色器
    IShader* vertexShader   = nullptr;
    IShader* pixelShader    = nullptr;
    IShader* geometryShader = nullptr;
    IShader* hullShader     = nullptr;
    IShader* domainShader   = nullptr;

    // 输入布局
    std::vector<InputElementDesc> inputLayout;

    // 拓扑
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;

    // 光栅化状态
    RasterizerState rasterizerState;

    // 深度模板状态
    DepthStencilState depthStencilState;

    // 混合状态
    BlendState blendState;

    // 渲染目标格式
    std::vector<Format> renderTargetFormats;
    Format              depthStencilFormat = Format::Unknown;

    // 多重采样
    uint32_t sampleCount   = 1;
    uint32_t sampleQuality = 0;

    // 根签名
    class IRootSignature* rootSignature = nullptr;

    // 调试名称
    const char* debugName = nullptr;
};

// =============================================================================
// 计算管线描述
// =============================================================================

struct ComputePipelineDesc
{
    IShader*        computeShader = nullptr;
    IRootSignature* rootSignature = nullptr;
    const char*     debugName     = nullptr;
};

// =============================================================================
// IPipelineState - 管线状态接口
// =============================================================================

class RHI_API IPipelineState
{
  public:
    virtual ~IPipelineState() = default;

    /// 获取管线状态句柄
    virtual void* GetNativeHandle() const = 0;
};

} // namespace Engine::RHI