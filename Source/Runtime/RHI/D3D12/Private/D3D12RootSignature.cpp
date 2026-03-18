// =============================================================================
// D3D12RootSignature.cpp - D3D12 根签名实现
// =============================================================================

#include "D3D12RootSignature.h"

#if ENGINE_RHI_D3D12

namespace Engine::RHI::D3D12
{

// =============================================================================
// 辅助函数
// =============================================================================

namespace
{

D3D12_DESCRIPTOR_RANGE_TYPE ConvertDescriptorRangeType(DescriptorType type)
{
    switch (type)
    {
    case DescriptorType::CBV:     return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case DescriptorType::SRV:     return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case DescriptorType::UAV:     return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case DescriptorType::Sampler: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    default:                      return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

D3D12_SHADER_VISIBILITY ConvertShaderVisibility(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex:  return D3D12_SHADER_VISIBILITY_VERTEX;
    case ShaderStage::Pixel:   return D3D12_SHADER_VISIBILITY_PIXEL;
    case ShaderStage::Geometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case ShaderStage::Hull:    return D3D12_SHADER_VISIBILITY_HULL;
    case ShaderStage::Domain:  return D3D12_SHADER_VISIBILITY_DOMAIN;
    case ShaderStage::Compute: return D3D12_SHADER_VISIBILITY_ALL;
    case ShaderStage::All:     return D3D12_SHADER_VISIBILITY_ALL;
    default:                   return D3D12_SHADER_VISIBILITY_ALL;
    }
}

} // anonymous namespace

// =============================================================================
// 构造/析构
// =============================================================================

D3D12RootSignature::D3D12RootSignature()
{
}

D3D12RootSignature::~D3D12RootSignature()
{
    Shutdown();
}

D3D12RootSignature::D3D12RootSignature(D3D12RootSignature&& other) noexcept
    : m_rootSignature(std::move(other.m_rootSignature)),
      m_numParameters(other.m_numParameters),
      m_debugName(std::move(other.m_debugName))
{
    other.m_numParameters = 0;
}

D3D12RootSignature& D3D12RootSignature::operator=(D3D12RootSignature&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();
        m_rootSignature       = std::move(other.m_rootSignature);
        m_numParameters       = other.m_numParameters;
        m_debugName           = std::move(other.m_debugName);
        other.m_numParameters = 0;
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12RootSignature::Initialize(D3D12Device* device, const RootSignatureDesc& desc)
{
    if (!device)
    {
        RHI_LOG_ERROR("D3D12RootSignature::Initialize: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();

    // 构建根参数
    std::vector<D3D12_ROOT_PARAMETER1> rootParameters;
    rootParameters.reserve(desc.parameters.size());

    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> descriptorRangesPerParam;

    for (const auto& param : desc.parameters)
    {
        D3D12_ROOT_PARAMETER1 d3dParam = {};

        switch (param.type)
        {
        case RootParameterType::Constants:
            d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            d3dParam.Constants.ShaderRegister = param.shaderRegister;
            d3dParam.Constants.RegisterSpace = param.registerSpace;
            d3dParam.Constants.Num32BitValues = param.num32BitValues;
            d3dParam.ShaderVisibility = ConvertShaderVisibility(param.visibility);
            break;

        case RootParameterType::CBV:
            d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            d3dParam.Descriptor.ShaderRegister = param.shaderRegister;
            d3dParam.Descriptor.RegisterSpace = param.registerSpace;
            d3dParam.ShaderVisibility = ConvertShaderVisibility(param.visibility);
            break;

        case RootParameterType::SRV:
            d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            d3dParam.Descriptor.ShaderRegister = param.shaderRegister;
            d3dParam.Descriptor.RegisterSpace = param.registerSpace;
            d3dParam.ShaderVisibility = ConvertShaderVisibility(param.visibility);
            break;

        case RootParameterType::UAV:
            d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
            d3dParam.Descriptor.ShaderRegister = param.shaderRegister;
            d3dParam.Descriptor.RegisterSpace = param.registerSpace;
            d3dParam.ShaderVisibility = ConvertShaderVisibility(param.visibility);
            break;

        case RootParameterType::DescriptorTable:
            {
                d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

                // 创建描述符范围
                std::vector<D3D12_DESCRIPTOR_RANGE1> ranges;
                ranges.reserve(param.ranges.size());

                for (const auto& range : param.ranges)
                {
                    D3D12_DESCRIPTOR_RANGE1 d3dRange = {};
                    d3dRange.RangeType = ConvertDescriptorRangeType(range.type);
                    d3dRange.NumDescriptors = range.numDescriptors;
                    d3dRange.BaseShaderRegister = range.baseShaderRegister;
                    d3dRange.RegisterSpace = range.registerSpace;
                    d3dRange.OffsetInDescriptorsFromTableStart = range.offset;

                    if (range.numDescriptors == UINT_MAX)
                    {
                        d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
                    }
                    else
                    {
                        d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                    }

                    ranges.push_back(d3dRange);
                }

                descriptorRangesPerParam.push_back(std::move(ranges));

                d3dParam.DescriptorTable.NumDescriptorRanges = 
                    static_cast<UINT>(descriptorRangesPerParam.back().size());
                d3dParam.DescriptorTable.pDescriptorRanges = 
                    descriptorRangesPerParam.back().data();
                d3dParam.ShaderVisibility = ConvertShaderVisibility(param.visibility);
            }
            break;
        }

        rootParameters.push_back(d3dParam);
    }

    // 构建静态采样器
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    staticSamplers.reserve(desc.staticSamplers.size());

    for (const auto& sampler : desc.staticSamplers)
    {
        D3D12_STATIC_SAMPLER_DESC d3dSampler = {};
        d3dSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 默认线性过滤
        d3dSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d3dSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d3dSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d3dSampler.MipLODBias = 0.0f;
        d3dSampler.MaxAnisotropy = 1;
        d3dSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        d3dSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        d3dSampler.MinLOD = 0.0f;
        d3dSampler.MaxLOD = D3D12_FLOAT32_MAX;
        d3dSampler.ShaderRegister = sampler.shaderRegister;
        d3dSampler.RegisterSpace = sampler.registerSpace;
        d3dSampler.ShaderVisibility = ConvertShaderVisibility(sampler.visibility);

        staticSamplers.push_back(d3dSampler);
    }

    // 创建根签名描述
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

    D3D12_ROOT_SIGNATURE_DESC1& desc1 = rootSigDesc.Desc_1_1;
    desc1.NumParameters = static_cast<UINT>(rootParameters.size());
    desc1.pParameters = rootParameters.empty() ? nullptr : rootParameters.data();
    desc1.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
    desc1.pStaticSamplers = staticSamplers.empty() ? nullptr : staticSamplers.data();
    desc1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // 序列化根签名
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            RHI_LOG_ERROR("D3D12RootSignature::Initialize: 序列化失败: %s",
                          static_cast<const char*>(error->GetBufferPointer()));
        }
        return RHIResult::ResourceCreationFailed;
    }

    // 创建根签名
    hr = d3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), 
                                        signature->GetBufferSize(), 
                                        IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12RootSignature::Initialize: 创建根签名失败, HRESULT=0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }

    m_numParameters = static_cast<uint32_t>(rootParameters.size());

    // 设置调试名称
    if (desc.debugName)
    {
        m_debugName = desc.debugName;
        m_rootSignature->SetName(StringToWString(desc.debugName).c_str());
    }

    RHI_LOG_INFO("D3D12RootSignature::Initialize: 成功创建根签名, 参数数量=%u, 静态采样器=%u",
                 m_numParameters, staticSamplers.size());
    return RHIResult::Success;
}

RHIResult D3D12RootSignature::InitializeFromBytecode(D3D12Device* device, const void* bytecode, size_t size)
{
    if (!device)
    {
        RHI_LOG_ERROR("D3D12RootSignature::InitializeFromBytecode: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    if (!bytecode || size == 0)
    {
        RHI_LOG_ERROR("D3D12RootSignature::InitializeFromBytecode: 字节码无效");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();

    HRESULT hr = d3dDevice->CreateRootSignature(0, bytecode, size, IID_PPV_ARGS(&m_rootSignature));

    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12RootSignature::InitializeFromBytecode: 创建根签名失败, HRESULT=0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }

    return RHIResult::Success;
}

void D3D12RootSignature::Shutdown()
{
    m_rootSignature.Reset();
    m_numParameters = 0;
    m_debugName.clear();
}

// =============================================================================
// 接口实现
// =============================================================================

void* D3D12RootSignature::GetNativeHandle() const
{
    return m_rootSignature.Get();
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12