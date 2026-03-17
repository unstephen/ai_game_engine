// =============================================================================
// D3D12Buffer.cpp - D3D12 缓冲区实现
// =============================================================================

#include "D3D12Buffer.h"

#if ENGINE_RHI_D3D12

namespace Engine::RHI::D3D12 {

// =============================================================================
// 辅助结构（替代 d3dx12.h）
// =============================================================================

namespace {

/**
 * @brief 堆属性辅助结构
 */
struct HeapProperties {
    D3D12_HEAP_PROPERTIES props;
    
    explicit HeapProperties(D3D12_HEAP_TYPE type) {
        props.Type = type;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.CreationNodeMask = 1;
        props.VisibleNodeMask = 1;
    }
    
    operator const D3D12_HEAP_PROPERTIES&() const { return props; }
};

/**
 * @brief 创建缓冲区资源描述
 */
D3D12_RESOURCE_DESC CreateBufferDesc(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;
    return desc;
}

} // anonymous namespace

// =============================================================================
// 构造/析构
// =============================================================================

D3D12Buffer::D3D12Buffer() {
    RHI_LOG_DEBUG("D3D12Buffer: 构造函数");
}

D3D12Buffer::~D3D12Buffer() {
    RHI_LOG_DEBUG("D3D12Buffer: 析构函数");
    Shutdown();
}

// =============================================================================
// 移动构造/赋值
// =============================================================================

D3D12Buffer::D3D12Buffer(D3D12Buffer&& other) noexcept
    : m_resource(std::move(other.m_resource))
    , m_size(other.m_size)
    , m_stride(other.m_stride)
    , m_usage(other.m_usage)
    , m_mappedData(other.m_mappedData)
    , m_isMapped(other.m_isMapped)
    , m_cpuAccessible(other.m_cpuAccessible)
    , m_resourceState(other.m_resourceState)
    , m_debugName(std::move(other.m_debugName)) {
    
    // 重置 other
    other.m_size = 0;
    other.m_stride = 0;
    other.m_usage = BufferUsage::None;
    other.m_mappedData = nullptr;
    other.m_isMapped = false;
    other.m_cpuAccessible = false;
    other.m_resourceState = D3D12_RESOURCE_STATE_COMMON;
    
    RHI_LOG_DEBUG("D3D12Buffer: 移动构造");
}

D3D12Buffer& D3D12Buffer::operator=(D3D12Buffer&& other) noexcept {
    if (this != &other) {
        // 先清理当前资源
        Shutdown();
        
        // 移动资源
        m_resource = std::move(other.m_resource);
        m_size = other.m_size;
        m_stride = other.m_stride;
        m_usage = other.m_usage;
        m_mappedData = other.m_mappedData;
        m_isMapped = other.m_isMapped;
        m_cpuAccessible = other.m_cpuAccessible;
        m_resourceState = other.m_resourceState;
        m_debugName = std::move(other.m_debugName);
        
        // 重置 other
        other.m_size = 0;
        other.m_stride = 0;
        other.m_usage = BufferUsage::None;
        other.m_mappedData = nullptr;
        other.m_isMapped = false;
        other.m_cpuAccessible = false;
        other.m_resourceState = D3D12_RESOURCE_STATE_COMMON;
        
        RHI_LOG_DEBUG("D3D12Buffer: 移动赋值");
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12Buffer::Initialize(D3D12Device* device, const BufferDesc& desc) {
    RHI_LOG_INFO("D3D12Buffer: 开始初始化 (大小=%llu, 用途=0x%X)",
        desc.size, static_cast<uint32_t>(desc.usage));
    
    // ========== 1. 参数验证 ==========
    
    if (!device) {
        RHI_LOG_ERROR("D3D12Buffer: 设备指针为空");
        return RHIResult::InvalidParameter;
    }
    
    if (!device->GetD3D12Device()) {
        RHI_LOG_ERROR("D3D12Buffer: D3D12 设备未初始化");
        return RHIResult::InvalidParameter;
    }
    
    if (desc.size == 0) {
        RHI_LOG_ERROR("D3D12Buffer: 缓冲区大小为零");
        return RHIResult::InvalidParameter;
    }
    
    if (desc.usage == BufferUsage::None) {
        RHI_LOG_ERROR("D3D12Buffer: 缓冲区用途未指定");
        return RHIResult::InvalidParameter;
    }
    
    // ========== 2. 保存属性 ==========
    
    m_size = desc.size;
    m_stride = desc.stride;
    m_usage = desc.usage;
    
    // 保存调试名称
    if (desc.debugName) {
        m_debugName = desc.debugName;
    }
    
    // ========== 3. 创建资源 ==========
    
    RHIResult result = CreateResource(device, desc);
    if (IsFailure(result)) {
        RHI_LOG_ERROR("D3D12Buffer: 创建资源失败 (%s)", GetErrorName(result));
        return result;
    }
    
    // ========== 4. 设置调试名称 ==========
    
    if (!m_debugName.empty() && m_resource) {
        std::wstring wideName(m_debugName.begin(), m_debugName.end());
        m_resource->SetName(wideName.c_str());
    }
    
    // ========== 5. 如果有初始数据，上传数据 ==========
    
    if (desc.initialData && m_cpuAccessible && m_mappedData) {
        memcpy(m_mappedData, desc.initialData, static_cast<size_t>(desc.size));
        RHI_LOG_DEBUG("D3D12Buffer: 已上传初始数据 (%llu 字节)", desc.size);
    }
    
    RHI_LOG_INFO("D3D12Buffer: 初始化完成");
    return RHIResult::Success;
}

// =============================================================================
// 关闭
// =============================================================================

void D3D12Buffer::Shutdown() {
    // 取消映射（如果已映射）
    if (m_isMapped && m_resource) {
        m_resource->Unmap(0, nullptr);
        m_mappedData = nullptr;
        m_isMapped = false;
    }
    
    // 释放资源
    m_resource.Reset();
    
    // 重置状态
    m_size = 0;
    m_stride = 0;
    m_usage = BufferUsage::None;
    m_cpuAccessible = false;
    m_resourceState = D3D12_RESOURCE_STATE_COMMON;
    m_debugName.clear();
    
    RHI_LOG_DEBUG("D3D12Buffer: 已关闭");
}

// =============================================================================
// IBuffer 接口实现
// =============================================================================

uint64_t D3D12Buffer::GetGPUVirtualAddress() const {
    if (!m_resource) {
        RHI_LOG_WARNING("D3D12Buffer: 获取 GPU 地址失败，资源为空");
        return 0;
    }
    
    return m_resource->GetGPUVirtualAddress();
}

void* D3D12Buffer::Map() {
    // 检查资源是否存在
    if (!m_resource) {
        RHI_LOG_ERROR("D3D12Buffer: 映射失败，资源为空");
        return nullptr;
    }
    
    // 检查是否为 CPU 可访问缓冲区
    if (!m_cpuAccessible) {
        RHI_LOG_ERROR("D3D12Buffer: 无法映射 GPU 专用缓冲区（默认堆）");
        return nullptr;
    }
    
    // 如果已经映射，直接返回指针
    if (m_isMapped && m_mappedData) {
        RHI_LOG_DEBUG("D3D12Buffer: 返回已映射的指针");
        return m_mappedData;
    }
    
    // 执行映射
    HRESULT hr = m_resource->Map(0, nullptr, &m_mappedData);
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12Buffer: Map 失败 (HRESULT=0x%08X)", hr);
        m_mappedData = nullptr;
        m_isMapped = false;
        return nullptr;
    }
    
    m_isMapped = true;
    RHI_LOG_DEBUG("D3D12Buffer: 映射成功，地址=%p", m_mappedData);
    return m_mappedData;
}

void D3D12Buffer::Unmap() {
    // 检查是否已映射
    if (!m_isMapped || !m_resource) {
        return;
    }
    
    // 对于上传堆，通常保持映射状态以提高性能
    // 这里可以选择是否真正取消映射
    // 我们选择保持映射，但标记为未映射
    m_isMapped = false;
    
    // 注意：不调用 Unmap 以提高性能
    // 如果需要真正取消映射，取消下面的注释：
    // m_resource->Unmap(0, nullptr);
    // m_mappedData = nullptr;
    
    RHI_LOG_DEBUG("D3D12Buffer: Unmap 完成");
}

void D3D12Buffer::UpdateData(const void* data, uint64_t size, uint64_t offset) {
    // 参数检查
    if (!data) {
        RHI_LOG_ERROR("D3D12Buffer: 更新数据失败，数据指针为空");
        return;
    }
    
    if (size == 0) {
        RHI_LOG_WARNING("D3D12Buffer: 更新数据大小为零");
        return;
    }
    
    if (offset + size > m_size) {
        RHI_LOG_ERROR("D3D12Buffer: 更新数据超出缓冲区范围 (offset=%llu, size=%llu, bufferSize=%llu)",
            offset, size, m_size);
        return;
    }
    
    // 检查是否为 CPU 可访问
    if (!m_cpuAccessible) {
        RHI_LOG_ERROR("D3D12Buffer: 无法直接更新 GPU 专用缓冲区，请使用上传缓冲区或复制命令");
        return;
    }
    
    // 映射缓冲区（如果尚未映射）
    void* mappedPtr = Map();
    if (!mappedPtr) {
        RHI_LOG_ERROR("D3D12Buffer: 映射失败，无法更新数据");
        return;
    }
    
    // 复制数据
    memcpy(static_cast<uint8_t*>(mappedPtr) + offset, data, static_cast<size_t>(size));
    
    RHI_LOG_DEBUG("D3D12Buffer: 已更新数据 (%llu 字节, 偏移=%llu)", size, offset);
}

// =============================================================================
// D3D12 特定接口
// =============================================================================

D3D12_VERTEX_BUFFER_VIEW D3D12Buffer::GetVertexBufferView() const {
    D3D12_VERTEX_BUFFER_VIEW view = {};
    
    if (!m_resource) {
        RHI_LOG_WARNING("D3D12Buffer: 获取顶点缓冲区视图失败，资源为空");
        return view;
    }
    
    // 检查用途
    if ((m_usage & BufferUsage::Vertex) == BufferUsage::None) {
        RHI_LOG_WARNING("D3D12Buffer: 缓冲区不是顶点缓冲区");
        return view;
    }
    
    view.BufferLocation = m_resource->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(m_size);
    view.StrideInBytes = m_stride;
    
    return view;
}

D3D12_INDEX_BUFFER_VIEW D3D12Buffer::GetIndexBufferView() const {
    D3D12_INDEX_BUFFER_VIEW view = {};
    
    if (!m_resource) {
        RHI_LOG_WARNING("D3D12Buffer: 获取索引缓冲区视图失败，资源为空");
        return view;
    }
    
    // 检查用途
    if ((m_usage & BufferUsage::Index) == BufferUsage::None) {
        RHI_LOG_WARNING("D3D12Buffer: 缓冲区不是索引缓冲区");
        return view;
    }
    
    view.BufferLocation = m_resource->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(m_size);
    
    // 根据步长确定索引格式
    if (m_stride == 4) {
        view.Format = DXGI_FORMAT_R32_UINT;
    } else if (m_stride == 2) {
        view.Format = DXGI_FORMAT_R16_UINT;
    } else {
        RHI_LOG_WARNING("D3D12Buffer: 未知的索引格式，步长=%u，默认使用 R32_UINT", m_stride);
        view.Format = DXGI_FORMAT_R32_UINT;
    }
    
    return view;
}

// =============================================================================
// 内部方法
// =============================================================================

RHIResult D3D12Buffer::CreateResource(D3D12Device* device, const BufferDesc& desc) {
    ID3D12Device* d3dDevice = device->GetD3D12Device();
    
    // ========== 1. 确定堆类型 ==========
    
    // 常量缓冲区和需要频繁更新的缓冲区使用上传堆
    // 静态数据使用默认堆
    bool needCPUAccess = (desc.usage & BufferUsage::Constant) != BufferUsage::None;
    
    // 如果用途包含 CopySource 或是动态数据，也使用上传堆
    // 这里简化处理：常量缓冲区使用上传堆，其他使用默认堆
    m_cpuAccessible = needCPUAccess;
    
    D3D12_HEAP_TYPE heapType = m_cpuAccessible 
        ? D3D12_HEAP_TYPE_UPLOAD 
        : D3D12_HEAP_TYPE_DEFAULT;
    
    RHI_LOG_DEBUG("D3D12Buffer: 堆类型=%s", 
        m_cpuAccessible ? "UPLOAD" : "DEFAULT");
    
    // ========== 2. 计算缓冲区大小（对齐） ==========
    
    uint64_t bufferSize = desc.size;
    
    // 常量缓冲区需要 256 字节对齐
    if ((desc.usage & BufferUsage::Constant) != BufferUsage::None) {
        bufferSize = (bufferSize + 255) & ~255ULL;
        RHI_LOG_DEBUG("D3D12Buffer: 常量缓冲区对齐到 %llu 字节", bufferSize);
    }
    
    // 更新实际大小
    m_size = bufferSize;
    
    // ========== 3. 创建资源描述 ==========
    
    D3D12_RESOURCE_FLAGS resourceFlags = DetermineResourceFlags(desc.usage);
    
    D3D12_RESOURCE_DESC resourceDesc = CreateBufferDesc(
        bufferSize,
        resourceFlags
    );
    
    // 结构化缓冲区
    if (desc.stride > 0 && 
        (desc.usage & BufferUsage::ShaderResource) != BufferUsage::None) {
        resourceDesc.StructureByteStride = desc.stride;
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    
    // ========== 4. 确定初始资源状态 ==========
    
    m_resourceState = DetermineResourceState(desc.usage, desc.initialState);
    
    // 上传堆必须使用 GENERIC_READ 状态
    if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
        m_resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    
    // ========== 5. 创建堆属性 ==========
    
    HeapProperties heapProps(heapType);
    
    // ========== 6. 创建资源 ==========
    
    HRESULT hr = d3dDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        m_resourceState,
        nullptr,  // 清除值（缓冲区不需要）
        IID_PPV_ARGS(&m_resource)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12Buffer: CreateCommittedResource 失败 (HRESULT=0x%08X)", hr);
        
        if (hr == E_OUTOFMEMORY) {
            return RHIResult::OutOfMemory;
        }
        return RHIResult::ResourceCreationFailed;
    }
    
    RHI_LOG_INFO("D3D12Buffer: D3D12 资源创建成功");
    
    // ========== 7. 如果是上传堆，立即映射 ==========
    
    if (m_cpuAccessible) {
        hr = m_resource->Map(0, nullptr, &m_mappedData);
        
        if (FAILED(hr)) {
            RHI_LOG_ERROR("D3D12Buffer: 初始映射失败 (HRESULT=0x%08X)", hr);
            m_resource.Reset();
            return RHIResult::ResourceCreationFailed;
        }
        
        m_isMapped = true;
        RHI_LOG_DEBUG("D3D12Buffer: 上传堆已映射，地址=%p", m_mappedData);
    }
    
    return RHIResult::Success;
}

D3D12_RESOURCE_STATES D3D12Buffer::DetermineResourceState(
    BufferUsage usage,
    ResourceState initialState) {
    
    // 如果指定了初始状态，转换为 D3D12 状态
    if (initialState != ResourceState::Undefined && 
        initialState != ResourceState::Common) {
        
        // 简单的映射
        if ((initialState & ResourceState::VertexBuffer) != ResourceState::Undefined) {
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        if ((initialState & ResourceState::IndexBuffer) != ResourceState::IndexBuffer) {
            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        }
        if ((initialState & ResourceState::ConstantBuffer) != ResourceState::Undefined) {
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        if ((initialState & ResourceState::UnorderedAccess) != ResourceState::Undefined) {
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        if ((initialState & ResourceState::CopyDest) != ResourceState::Undefined) {
            return D3D12_RESOURCE_STATE_COPY_DEST;
        }
        if ((initialState & ResourceState::CopySource) != ResourceState::Undefined) {
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        }
    }
    
    // 根据用途确定默认状态
    if ((usage & BufferUsage::Constant) != BufferUsage::None) {
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    if ((usage & BufferUsage::Vertex) != BufferUsage::None) {
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    if ((usage & BufferUsage::Index) != BufferUsage::None) {
        return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    if ((usage & BufferUsage::UnorderedAccess) != BufferUsage::None) {
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if ((usage & BufferUsage::Indirect) != BufferUsage::None) {
        return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }
    
    // 默认状态
    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_RESOURCE_FLAGS D3D12Buffer::DetermineResourceFlags(BufferUsage usage) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    
    // UAV 访问
    if ((usage & BufferUsage::UnorderedAccess) != BufferUsage::None) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    
    return flags;
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12