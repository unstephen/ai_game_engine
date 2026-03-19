// =============================================================================
// D3D12Texture.cpp - D3D12 纹理实现
// =============================================================================

#include "D3D12Texture.h"

#if ENGINE_RHI_D3D12

#include <algorithm>

namespace Engine::RHI::D3D12
{

// =============================================================================
// 辅助结构（替代 d3dx12.h）
// =============================================================================

namespace
{

/**
 * @brief 堆属性辅助结构
 */
struct HeapProperties
{
    D3D12_HEAP_PROPERTIES props;

    explicit HeapProperties(D3D12_HEAP_TYPE type)
    {
        props.Type                 = type;
        props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.CreationNodeMask     = 1;
        props.VisibleNodeMask      = 1;
    }

    operator const D3D12_HEAP_PROPERTIES&() const { return props; }
};

/**
 * @brief 资源描述辅助结构（简化版）
 */
struct TextureResourceDesc
{
    D3D12_RESOURCE_DESC desc;

    TextureResourceDesc()
    {
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment          = 0;
        desc.Width              = 1;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    }

    operator const D3D12_RESOURCE_DESC&() const { return desc; }
    operator D3D12_RESOURCE_DESC*() { return &desc; }
};

/**
 * @brief 创建缓冲区资源描述（用于上传缓冲区）
 */
D3D12_RESOURCE_DESC CreateBufferDesc(UINT64 width)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment           = 0;
    desc.Width               = width;
    desc.Height              = 1;
    desc.DepthOrArraySize    = 1;
    desc.MipLevels           = 1;
    desc.Format              = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count    = 1;
    desc.SampleDesc.Quality  = 0;
    desc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags               = D3D12_RESOURCE_FLAG_NONE;
    return desc;
}

/**
 * @brief 计算纹理上传所需的中间缓冲区大小
 */
UINT64 GetRequiredIntermediateSize(ID3D12Resource* destinationResource, UINT firstSubresource, UINT numSubresources)
{

    auto device = reinterpret_cast<ID3D12Device*>(nullptr); // 我们实际上不需要设备指针

    D3D12_RESOURCE_DESC desc = destinationResource->GetDesc();

    UINT64 totalSize = 0;

    // 简化计算：估算所需大小
    UINT64 rowSize = 0;
    UINT   numRows = 0;

    for (UINT i = 0; i < numSubresources; ++i)
    {
        UINT subresource = firstSubresource + i;

        // 计算子资源的尺寸
        UINT   mipLevel = subresource % desc.MipLevels;
        UINT64 width    = desc.Width >> mipLevel;
        UINT   height   = desc.Height >> mipLevel;
        if (width == 0)
            width = 1;
        if (height == 0)
            height = 1;

        // 根据格式计算行大小
        UINT bytesPerPixel = 4; // 默认 4 字节
        switch (desc.Format)
        {
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_UINT:
            case DXGI_FORMAT_R32G32B32A32_SINT:
                bytesPerPixel = 16;
                break;
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SINT:
                bytesPerPixel = 8;
                break;
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_R32_UINT:
            case DXGI_FORMAT_D32_FLOAT:
                bytesPerPixel = 4;
                break;
            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_D16_UNORM:
                bytesPerPixel = 2;
                break;
            default:
                bytesPerPixel = 4;
                break;
        }

        UINT64 rowPitch = width * bytesPerPixel;
        // 对齐到 256 字节
        rowPitch = (rowPitch + 255) & ~255ULL;

        totalSize += rowPitch * height;
    }

    // 添加一些余量
    return totalSize + 1024;
}

/**
 * @brief 更新子资源（简化版）
 */
inline UINT64 UpdateSubresources(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestinationResource,
                                 ID3D12Resource* pIntermediate, UINT64 IntermediateOffset, UINT FirstSubresource,
                                 UINT NumSubresources, D3D12_SUBRESOURCE_DATA* pSrcData)
{

    // 获取目标资源描述
    D3D12_RESOURCE_DESC destDesc = pDestinationResource->GetDesc();

    // 对每个子资源执行复制
    for (UINT i = 0; i < NumSubresources; ++i)
    {
        UINT subresource = FirstSubresource + i;

        // 计算子资源的位置和大小
        UINT   mipLevel = subresource % destDesc.MipLevels;
        UINT64 width    = destDesc.Width >> mipLevel;
        UINT   height   = destDesc.Height >> mipLevel;
        if (width == 0)
            width = 1;
        if (height == 0)
            height = 0;

        // 计算行间距（对齐到 D3D12 要求）
        UINT64 rowPitch   = pSrcData[i].RowPitch;
        UINT64 slicePitch = pSrcData[i].SlicePitch;

        // 创建目标区域
        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource                   = pDestinationResource;
        dstLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex            = subresource;

        // 创建源区域
        D3D12_TEXTURE_COPY_LOCATION srcLoc        = {};
        srcLoc.pResource                          = pIntermediate;
        srcLoc.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset             = IntermediateOffset;
        srcLoc.PlacedFootprint.Footprint.Format   = destDesc.Format;
        srcLoc.PlacedFootprint.Footprint.Width    = static_cast<UINT>(width);
        srcLoc.PlacedFootprint.Footprint.Height   = static_cast<UINT>(height);
        srcLoc.PlacedFootprint.Footprint.Depth    = 1;
        srcLoc.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(rowPitch);

        // 执行复制
        pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        // 更新偏移
        IntermediateOffset += slicePitch;
    }

    return 0;
}

} // anonymous namespace

// =============================================================================
// 构造/析构
// =============================================================================

D3D12Texture::D3D12Texture()
{
    RHI_LOG_DEBUG("D3D12Texture: 构造函数");
}

D3D12Texture::~D3D12Texture()
{
    RHI_LOG_DEBUG("D3D12Texture: 析构函数");
    Shutdown();
}

// =============================================================================
// 移动语义
// =============================================================================

D3D12Texture::D3D12Texture(D3D12Texture&& other) noexcept
    : m_resource(std::move(other.m_resource)),
      m_width(other.m_width),
      m_height(other.m_height),
      m_depth(other.m_depth),
      m_mipLevels(other.m_mipLevels),
      m_arraySize(other.m_arraySize),
      m_format(other.m_format),
      m_usage(other.m_usage),
      m_dxgiFormat(other.m_dxgiFormat),
      m_mappedData(other.m_mappedData),
      m_isMapped(other.m_isMapped),
      m_cpuAccessible(other.m_cpuAccessible),
      m_isCubeMap(other.m_isCubeMap),
      m_resourceState(other.m_resourceState),
      m_debugName(std::move(other.m_debugName)),
      m_uploadBuffer(std::move(other.m_uploadBuffer))
{
    // 重置源对象
    other.m_width         = 0;
    other.m_height        = 0;
    other.m_depth         = 1;
    other.m_mipLevels     = 1;
    other.m_arraySize     = 1;
    other.m_format        = Format::Unknown;
    other.m_usage         = TextureUsage::None;
    other.m_dxgiFormat    = DXGI_FORMAT_UNKNOWN;
    other.m_mappedData    = nullptr;
    other.m_isMapped      = false;
    other.m_cpuAccessible = false;
    other.m_isCubeMap     = false;
    other.m_resourceState = D3D12_RESOURCE_STATE_COMMON;
}

D3D12Texture& D3D12Texture::operator=(D3D12Texture&& other) noexcept
{
    if (this != &other)
    {
        // 释放当前资源
        Shutdown();

        // 移动数据
        m_resource      = std::move(other.m_resource);
        m_width         = other.m_width;
        m_height        = other.m_height;
        m_depth         = other.m_depth;
        m_mipLevels     = other.m_mipLevels;
        m_arraySize     = other.m_arraySize;
        m_format        = other.m_format;
        m_usage         = other.m_usage;
        m_dxgiFormat    = other.m_dxgiFormat;
        m_mappedData    = other.m_mappedData;
        m_isMapped      = other.m_isMapped;
        m_cpuAccessible = other.m_cpuAccessible;
        m_isCubeMap     = other.m_isCubeMap;
        m_resourceState = other.m_resourceState;
        m_debugName     = std::move(other.m_debugName);
        m_uploadBuffer  = std::move(other.m_uploadBuffer);

        // 重置源对象
        other.m_width         = 0;
        other.m_height        = 0;
        other.m_depth         = 1;
        other.m_mipLevels     = 1;
        other.m_arraySize     = 1;
        other.m_format        = Format::Unknown;
        other.m_usage         = TextureUsage::None;
        other.m_dxgiFormat    = DXGI_FORMAT_UNKNOWN;
        other.m_mappedData    = nullptr;
        other.m_isMapped      = false;
        other.m_cpuAccessible = false;
        other.m_isCubeMap     = false;
        other.m_resourceState = D3D12_RESOURCE_STATE_COMMON;
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12Texture::Initialize(D3D12Device* device, const TextureDesc& desc)
{
    // 参数验证
    if (!device)
    {
        RHI_LOG_ERROR("D3D12Texture: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    if (desc.width == 0 || desc.height == 0)
    {
        RHI_LOG_ERROR("D3D12Texture: 纹理尺寸无效 (%ux%u)", desc.width, desc.height);
        return RHIResult::InvalidParameter;
    }

    if (desc.format == Format::Unknown)
    {
        RHI_LOG_ERROR("D3D12Texture: 像素格式无效");
        return RHIResult::InvalidParameter;
    }

    if (desc.usage == TextureUsage::None)
    {
        RHI_LOG_ERROR("D3D12Texture: 纹理用途未指定");
        return RHIResult::InvalidParameter;
    }

    // 创建资源
    RHIResult result = CreateResource(device, desc);
    if (IsFailure(result))
    {
        RHI_LOG_ERROR("D3D12Texture: 创建资源失败");
        return result;
    }

    // 保存属性
    m_width      = desc.width;
    m_height     = desc.height;
    m_depth      = desc.depth;
    m_mipLevels  = desc.mipLevels;
    m_arraySize  = desc.arraySize;
    m_format     = desc.format;
    m_usage      = desc.usage;
    m_dxgiFormat = ToDXGIFormat(desc.format);
    m_debugName  = desc.debugName ? desc.debugName : "";

    // 设置调试名称
    if (!m_debugName.empty() && m_resource)
    {
        std::wstring wideName(m_debugName.begin(), m_debugName.end());
        m_resource->SetName(wideName.c_str());
    }

    RHI_LOG_INFO("D3D12Texture: 纹理创建成功 (%ux%u, 格式=%d, Mips=%u, 数组=%u)", m_width, m_height,
                 static_cast<int>(m_format), m_mipLevels, m_arraySize);

    return RHIResult::Success;
}

// =============================================================================
// 关闭
// =============================================================================

void D3D12Texture::Shutdown()
{
    // 如果已映射，先取消映射
    if (m_isMapped && m_resource)
    {
        m_resource->Unmap(0, nullptr);
        m_isMapped   = false;
        m_mappedData = nullptr;
    }

    // 释放上传缓冲区
    m_uploadBuffer.Reset();

    // 释放资源
    m_resource.Reset();

    // 重置属性
    m_width         = 0;
    m_height        = 0;
    m_depth         = 1;
    m_mipLevels     = 1;
    m_arraySize     = 1;
    m_format        = Format::Unknown;
    m_usage         = TextureUsage::None;
    m_dxgiFormat    = DXGI_FORMAT_UNKNOWN;
    m_cpuAccessible = false;
    m_isCubeMap     = false;
    m_resourceState = D3D12_RESOURCE_STATE_COMMON;
    m_debugName.clear();

    RHI_LOG_DEBUG("D3D12Texture: 资源已释放");
}

// =============================================================================
// 资源创建
// =============================================================================

RHIResult D3D12Texture::CreateResource(D3D12Device* device, const TextureDesc& desc)
{
    ID3D12Device* d3dDevice = device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12Texture: D3D12 设备无效");
        return RHIResult::InternalError;
    }

    // 转换格式
    DXGI_FORMAT dxgiFormat = ToDXGIFormat(desc.format);
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        RHI_LOG_ERROR("D3D12Texture: 不支持的像素格式");
        return RHIResult::NotSupported;
    }

    // 计算 mip 级别数（如果为 0 则自动计算）
    uint32_t mipLevels = desc.mipLevels;
    if (mipLevels == 0)
    {
        mipLevels = CalcMipLevels(desc.width, desc.height);
    }

    // 检测 CubeMap
    m_isCubeMap = (desc.arraySize == 6 && desc.depth == 1);

    // 创建资源描述
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Alignment           = 0;

    // 确定资源维度
    if (desc.depth > 1)
    {
        // 3D 纹理
        resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        resourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.depth);
    }
    else if (desc.arraySize > 1 || m_isCubeMap)
    {
        // 纹理数组或 CubeMap
        resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.arraySize);
    }
    else
    {
        // 2D 纹理
        resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.DepthOrArraySize = 1;
    }

    resourceDesc.Width              = desc.width;
    resourceDesc.Height             = desc.height;
    resourceDesc.MipLevels          = static_cast<UINT16>(mipLevels);
    resourceDesc.Format             = dxgiFormat;
    resourceDesc.SampleDesc.Count   = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Flags              = DetermineResourceFlags(desc.usage);
    resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    // 确定堆类型和初始状态
    D3D12_HEAP_TYPE       heapType;
    D3D12_RESOURCE_STATES initialState;

    // 检查是否需要 CPU 访问（上传堆）
    // 深度纹理和渲染目标不能使用上传堆
    m_cpuAccessible = false;
    if ((desc.usage & TextureUsage::DepthStencil) != TextureUsage::None)
    {
        heapType     = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    else if ((desc.usage & TextureUsage::RenderTarget) != TextureUsage::None)
    {
        heapType     = D3D12_HEAP_TYPE_DEFAULT;
        initialState = DetermineResourceState(desc.usage, desc.initialState);
    }
    else
    {
        // 默认使用默认堆
        heapType     = D3D12_HEAP_TYPE_DEFAULT;
        initialState = DetermineResourceState(desc.usage, desc.initialState);
    }

    // 创建堆属性
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type                  = heapType;
    heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask      = 1;
    heapProps.VisibleNodeMask       = 1;

    // 创建优化清除值（可选）
    D3D12_CLEAR_VALUE* pClearValue = nullptr;
    D3D12_CLEAR_VALUE  clearValue  = {};

    if ((desc.usage & TextureUsage::DepthStencil) != TextureUsage::None)
    {
        // 深度模板纹理的优化清除值
        clearValue.Format               = dxgiFormat;
        clearValue.DepthStencil.Depth   = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClearValue                     = &clearValue;
    }
    else if ((desc.usage & TextureUsage::RenderTarget) != TextureUsage::None)
    {
        // 渲染目标的优化清除值
        clearValue.Format   = dxgiFormat;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
        pClearValue         = &clearValue;
    }

    // 创建资源
    HRESULT hr = d3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
                                                    pClearValue, IID_PPV_ARGS(&m_resource));

    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12Texture: CreateCommittedResource 失败 (HRESULT: 0x%08X)", hr);
        return RHIResult::ResourceCreationFailed;
    }

    // 保存资源状态
    m_resourceState = initialState;

    return RHIResult::Success;
}

// =============================================================================
// 资源视图创建
// =============================================================================

RHIResult D3D12Texture::CreateSRV(D3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t mostDetailedMip,
                                  int32_t mipLevels)
{

    if (!device || !m_resource)
    {
        RHI_LOG_ERROR("D3D12Texture: CreateSRV 参数无效");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12Texture: D3D12 设备无效");
        return RHIResult::InternalError;
    }

    // 创建 SRV 描述
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = m_dxgiFormat;
    srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // 根据纹理类型设置视图维度
    if (m_isCubeMap)
    {
        srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip     = mostDetailedMip;
        srvDesc.TextureCube.MipLevels           = (mipLevels < 0) ? m_mipLevels : mipLevels;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else if (m_arraySize > 1)
    {
        srvDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip     = mostDetailedMip;
        srvDesc.Texture2DArray.MipLevels           = (mipLevels < 0) ? m_mipLevels : mipLevels;
        srvDesc.Texture2DArray.FirstArraySlice     = 0;
        srvDesc.Texture2DArray.ArraySize           = m_arraySize;
        srvDesc.Texture2DArray.PlaneSlice          = 0;
        srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip     = mostDetailedMip;
        srvDesc.Texture2D.MipLevels           = (mipLevels < 0) ? m_mipLevels : mipLevels;
        srvDesc.Texture2D.PlaneSlice          = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    }

    d3dDevice->CreateShaderResourceView(m_resource.Get(), &srvDesc, handle);

    RHI_LOG_DEBUG("D3D12Texture: SRV 创建成功");
    return RHIResult::Success;
}

RHIResult D3D12Texture::CreateRTV(D3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t mipSlice,
                                  uint32_t arraySlice)
{

    if (!device || !m_resource)
    {
        RHI_LOG_ERROR("D3D12Texture: CreateRTV 参数无效");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12Texture: D3D12 设备无效");
        return RHIResult::InternalError;
    }

    // 检查用途
    if ((m_usage & TextureUsage::RenderTarget) == TextureUsage::None)
    {
        RHI_LOG_WARNING("D3D12Texture: 纹理未设置 RenderTarget 用途");
    }

    // 创建 RTV 描述
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format                        = m_dxgiFormat;

    if (m_arraySize > 1)
    {
        rtvDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice        = mipSlice;
        rtvDesc.Texture2DArray.FirstArraySlice = arraySlice;
        rtvDesc.Texture2DArray.ArraySize       = 1;
        rtvDesc.Texture2DArray.PlaneSlice      = 0;
    }
    else
    {
        rtvDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice   = mipSlice;
        rtvDesc.Texture2D.PlaneSlice = 0;
    }

    d3dDevice->CreateRenderTargetView(m_resource.Get(), &rtvDesc, handle);

    RHI_LOG_DEBUG("D3D12Texture: RTV 创建成功");
    return RHIResult::Success;
}

RHIResult D3D12Texture::CreateDSV(D3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t mipSlice)
{

    if (!device || !m_resource)
    {
        RHI_LOG_ERROR("D3D12Texture: CreateDSV 参数无效");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12Texture: D3D12 设备无效");
        return RHIResult::InternalError;
    }

    // 检查用途
    if ((m_usage & TextureUsage::DepthStencil) == TextureUsage::None)
    {
        RHI_LOG_WARNING("D3D12Texture: 纹理未设置 DepthStencil 用途");
    }

    // 创建 DSV 描述
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format                        = m_dxgiFormat;
    dsvDesc.Flags                         = D3D12_DSV_FLAG_NONE;

    if (m_arraySize > 1)
    {
        dsvDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice        = mipSlice;
        dsvDesc.Texture2DArray.FirstArraySlice = 0;
        dsvDesc.Texture2DArray.ArraySize       = m_arraySize;
    }
    else
    {
        dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = mipSlice;
    }

    d3dDevice->CreateDepthStencilView(m_resource.Get(), &dsvDesc, handle);

    RHI_LOG_DEBUG("D3D12Texture: DSV 创建成功");
    return RHIResult::Success;
}

RHIResult D3D12Texture::CreateUAV(D3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t mipSlice)
{

    if (!device || !m_resource)
    {
        RHI_LOG_ERROR("D3D12Texture: CreateUAV 参数无效");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12Texture: D3D12 设备无效");
        return RHIResult::InternalError;
    }

    // 检查用途
    if ((m_usage & TextureUsage::UnorderedAccess) == TextureUsage::None)
    {
        RHI_LOG_WARNING("D3D12Texture: 纹理未设置 UnorderedAccess 用途");
    }

    // 创建 UAV 描述
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format                           = m_dxgiFormat;

    if (m_arraySize > 1)
    {
        uavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice        = mipSlice;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.ArraySize       = m_arraySize;
        uavDesc.Texture2DArray.PlaneSlice      = 0;
    }
    else
    {
        uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice   = mipSlice;
        uavDesc.Texture2D.PlaneSlice = 0;
    }

    d3dDevice->CreateUnorderedAccessView(m_resource.Get(), nullptr, &uavDesc, handle);

    RHI_LOG_DEBUG("D3D12Texture: UAV 创建成功");
    return RHIResult::Success;
}

// =============================================================================
// 数据上传
// =============================================================================

RHIResult D3D12Texture::UploadData(D3D12Device* device, ID3D12GraphicsCommandList* commandList, const void* data,
                                   size_t dataSize, uint32_t rowPitch, uint32_t arraySlice, uint32_t mipLevel)
{

    if (!device || !m_resource || !data)
    {
        RHI_LOG_ERROR("D3D12Texture: UploadData 参数无效");
        return RHIResult::InvalidParameter;
    }

    if (dataSize == 0 || rowPitch == 0)
    {
        RHI_LOG_ERROR("D3D12Texture: UploadData 数据大小或行间距为 0");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12Texture: D3D12 设备无效");
        return RHIResult::InternalError;
    }

    // 计算上传所需的缓冲区大小
    uint64_t uploadBufferSize = GetRequiredIntermediateSize(m_resource.Get(), CalcSubresource(mipLevel, arraySlice), 1);

    // 创建上传缓冲区（如果需要）
    if (!m_uploadBuffer || m_uploadBufferSize < uploadBufferSize)
    {
        // 创建堆属性
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type                  = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask      = 1;
        uploadHeapProps.VisibleNodeMask       = 1;

        // 创建缓冲区描述
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment           = 0;
        bufferDesc.Width               = uploadBufferSize;
        bufferDesc.Height              = 1;
        bufferDesc.DepthOrArraySize    = 1;
        bufferDesc.MipLevels           = 1;
        bufferDesc.Format              = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count    = 1;
        bufferDesc.SampleDesc.Quality  = 0;
        bufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = d3dDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&m_uploadBuffer));

        if (FAILED(hr))
        {
            RHI_LOG_ERROR("D3D12Texture: 创建上传缓冲区失败");
            return RHIResult::ResourceCreationFailed;
        }

        m_uploadBufferSize = uploadBufferSize;
        m_uploadBuffer->SetName(L"D3D12Texture_UploadBuffer");
    }

    // 计算子资源索引
    uint32_t subresourceIndex = CalcSubresource(mipLevel, arraySlice);

    // 获取 mip 级别的尺寸
    uint32_t mipWidth  = GetWidth(mipLevel);
    uint32_t mipHeight = GetHeight(mipLevel);

    // 填充上传数据
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData                  = data;
    subresourceData.RowPitch               = rowPitch;
    subresourceData.SlicePitch             = rowPitch * mipHeight;

    // 检查是否需要状态转换
    D3D12_RESOURCE_BARRIER barrier = {};
    if (m_resourceState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = m_resource.Get();
        barrier.Transition.StateBefore = m_resourceState;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = subresourceIndex;

        commandList->ResourceBarrier(1, &barrier);
        m_resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    // 执行上传
    UpdateSubresources(commandList, m_resource.Get(), m_uploadBuffer.Get(), 0, subresourceIndex, 1, &subresourceData);

    // 转换回着色器资源状态
    if (m_resourceState == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(1, &barrier);
        m_resourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    RHI_LOG_DEBUG("D3D12Texture: 数据上传成功 (大小=%zu, 行间距=%u)", dataSize, rowPitch);
    return RHIResult::Success;
}

void* D3D12Texture::Map(uint32_t subresource, const D3D12_RANGE* range)
{
    if (!m_resource)
    {
        RHI_LOG_ERROR("D3D12Texture: 资源未初始化");
        return nullptr;
    }

    // 默认堆纹理不能直接映射
    if (!m_cpuAccessible)
    {
        RHI_LOG_ERROR("D3D12Texture: 默认堆纹理不能直接映射");
        return nullptr;
    }

    if (m_isMapped)
    {
        // 已经映射，返回缓存的指针
        return m_mappedData;
    }

    D3D12_RANGE        readRange = {0, 0};
    const D3D12_RANGE* pRange    = range ? range : &readRange;

    HRESULT hr = m_resource->Map(subresource, pRange, &m_mappedData);
    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12Texture: Map 失败 (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    m_isMapped = true;
    return m_mappedData;
}

void D3D12Texture::Unmap(uint32_t subresource, const D3D12_RANGE* range)
{
    if (!m_resource || !m_isMapped)
    {
        return;
    }

    m_resource->Unmap(subresource, range);
    m_isMapped   = false;
    m_mappedData = nullptr;
}

// =============================================================================
// 辅助方法
// =============================================================================

uint32_t D3D12Texture::GetWidth(uint32_t mipLevel) const
{
    return std::max(1u, m_width >> mipLevel);
}

uint32_t D3D12Texture::GetHeight(uint32_t mipLevel) const
{
    return std::max(1u, m_height >> mipLevel);
}

uint32_t D3D12Texture::GetSubresourceCount() const
{
    return m_mipLevels * m_arraySize;
}

uint32_t D3D12Texture::CalcSubresource(uint32_t mipLevel, uint32_t arraySlice) const
{
    return D3D12CalcSubresource(mipLevel, arraySlice, 0, m_mipLevels, m_arraySize);
}

// =============================================================================
// 静态辅助方法
// =============================================================================

D3D12_RESOURCE_STATES D3D12Texture::DetermineResourceState(TextureUsage usage, ResourceState initialState)
{

    // 优先使用用户指定的初始状态
    switch (initialState)
    {
        case ResourceState::Common:
            return D3D12_RESOURCE_STATE_COMMON;
        case ResourceState::RenderTarget:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ResourceState::DepthWrite:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case ResourceState::DepthRead:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case ResourceState::ShaderResource:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case ResourceState::UnorderedAccess:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case ResourceState::CopyDest:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceState::CopySource:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceState::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        default:
            break;
    }

    // 根据用途推断状态
    if ((usage & TextureUsage::RenderTarget) != TextureUsage::None)
    {
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if ((usage & TextureUsage::DepthStencil) != TextureUsage::None)
    {
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if ((usage & TextureUsage::UnorderedAccess) != TextureUsage::None)
    {
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if ((usage & TextureUsage::ShaderResource) != TextureUsage::None)
    {
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_RESOURCE_FLAGS D3D12Texture::DetermineResourceFlags(TextureUsage usage)
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if ((usage & TextureUsage::RenderTarget) != TextureUsage::None)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if ((usage & TextureUsage::DepthStencil) != TextureUsage::None)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if ((usage & TextureUsage::UnorderedAccess) != TextureUsage::None)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return flags;
}

DXGI_FORMAT D3D12Texture::ToDXGIFormat(Format format)
{
    switch (format)
    {
        case Format::R32G32B32A32_Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R32G32B32_Float:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::R32G32_Float:
            return DXGI_FORMAT_R32G32_FLOAT;
        case Format::R32_Float:
            return DXGI_FORMAT_R32_FLOAT;
        case Format::R16G16B16A16_Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::R16G16_Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        case Format::R16_Float:
            return DXGI_FORMAT_R16_FLOAT;
        case Format::R8G8B8A8_UNorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_UNorm_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::R8G8_UNorm:
            return DXGI_FORMAT_R8G8_UNORM;
        case Format::R8_UNorm:
            return DXGI_FORMAT_R8_UNORM;
        case Format::D32_Float:
            return DXGI_FORMAT_D32_FLOAT;
        case Format::D24_UNorm_S8_UInt:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::D16_UNorm:
            return DXGI_FORMAT_D16_UNORM;
        case Format::BC1_UNorm:
            return DXGI_FORMAT_BC1_UNORM;
        case Format::BC3_UNorm:
            return DXGI_FORMAT_BC3_UNORM;
        case Format::BC7_UNorm:
            return DXGI_FORMAT_BC7_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

uint32_t D3D12Texture::GetBytesPerPixel(Format format)
{
    switch (format)
    {
        case Format::R32G32B32A32_Float:
            return 16;
        case Format::R32G32B32_Float:
            return 12;
        case Format::R32G32_Float:
            return 8;
        case Format::R32_Float:
            return 4;
        case Format::R16G16B16A16_Float:
            return 8;
        case Format::R16G16_Float:
            return 4;
        case Format::R16_Float:
            return 2;
        case Format::R8G8B8A8_UNorm:
        case Format::R8G8B8A8_UNorm_SRGB:
            return 4;
        case Format::R8G8_UNorm:
            return 2;
        case Format::R8_UNorm:
            return 1;
        case Format::D32_Float:
            return 4;
        case Format::D24_UNorm_S8_UInt:
            return 4;
        case Format::D16_UNorm:
            return 2;
        default:
            return 0;
    }
}

uint32_t D3D12Texture::CalcMipLevels(uint32_t width, uint32_t height)
{
    uint32_t maxDimension = std::max(width, height);
    uint32_t mipLevels    = 0;

    while (maxDimension > 0)
    {
        ++mipLevels;
        maxDimension >>= 1;
    }

    return mipLevels;
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12