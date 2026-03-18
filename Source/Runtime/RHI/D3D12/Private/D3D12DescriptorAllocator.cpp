// =============================================================================
// D3D12DescriptorAllocator.cpp - D3D12 描述符分配器实现
// =============================================================================

#include "D3D12DescriptorAllocator.h"
#include "D3D12Device.h"

#if ENGINE_RHI_D3D12

namespace Engine::RHI::D3D12
{

// =============================================================================
// D3D12DescriptorHeap 实现
// =============================================================================

D3D12DescriptorHeap::D3D12DescriptorHeap()
{
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
    Shutdown();
}

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12DescriptorHeap&& other) noexcept
    : m_heap(std::move(other.m_heap)),
      m_cpuStart(other.m_cpuStart),
      m_gpuStart(other.m_gpuStart),
      m_descriptorSize(other.m_descriptorSize),
      m_capacity(other.m_capacity),
      m_allocated(other.m_allocated),
      m_shaderVisible(other.m_shaderVisible),
      m_debugName(std::move(other.m_debugName)),
      m_freeList(std::move(other.m_freeList))
{
    other.m_cpuStart = {0};
    other.m_gpuStart = {0};
    other.m_descriptorSize = 0;
    other.m_capacity = 0;
    other.m_allocated = 0;
    other.m_shaderVisible = false;
}

D3D12DescriptorHeap& D3D12DescriptorHeap::operator=(D3D12DescriptorHeap&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();

        m_heap = std::move(other.m_heap);
        m_cpuStart = other.m_cpuStart;
        m_gpuStart = other.m_gpuStart;
        m_descriptorSize = other.m_descriptorSize;
        m_capacity = other.m_capacity;
        m_allocated = other.m_allocated;
        m_shaderVisible = other.m_shaderVisible;
        m_debugName = std::move(other.m_debugName);
        m_freeList = std::move(other.m_freeList);

        other.m_cpuStart = {0};
        other.m_gpuStart = {0};
        other.m_descriptorSize = 0;
        other.m_capacity = 0;
        other.m_allocated = 0;
        other.m_shaderVisible = false;
    }
    return *this;
}

RHIResult D3D12DescriptorHeap::Initialize(ID3D12Device* device, const DescriptorHeapDesc& desc)
{
    if (!device)
    {
        RHI_LOG_ERROR("D3D12DescriptorHeap::Initialize: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    if (desc.numDescriptors == 0)
    {
        RHI_LOG_ERROR("D3D12DescriptorHeap::Initialize: 描述符数量不能为 0");
        return RHIResult::InvalidParameter;
    }

    // 转换堆类型
    D3D12_DESCRIPTOR_HEAP_TYPE d3dType;
    switch (desc.type)
    {
    case DescriptorHeapType::CBV_SRV_UAV:
        d3dType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        break;
    case DescriptorHeapType::Sampler:
        d3dType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        break;
    case DescriptorHeapType::RTV:
        d3dType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        break;
    case DescriptorHeapType::DSV:
        d3dType = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        break;
    default:
        RHI_LOG_ERROR("D3D12DescriptorHeap::Initialize: 未知的堆类型");
        return RHIResult::InvalidParameter;
    }

    // 创建堆描述
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = d3dType;
    heapDesc.NumDescriptors = desc.numDescriptors;
    heapDesc.Flags = desc.shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE 
                                        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    // 创建堆
    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap));
    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12DescriptorHeap::Initialize: 创建描述符堆失败, HRESULT=0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }

    // 获取起始句柄
    m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();
    if (desc.shaderVisible)
    {
        m_gpuStart = m_heap->GetGPUDescriptorHandleForHeapStart();
    }
    else
    {
        m_gpuStart = {0};
    }

    // 获取描述符大小
    m_descriptorSize = device->GetDescriptorHandleIncrementSize(d3dType);
    m_capacity = desc.numDescriptors;
    m_allocated = 0;
    m_shaderVisible = desc.shaderVisible;

    // 设置调试名称
    if (desc.debugName)
    {
        m_debugName = desc.debugName;
        m_heap->SetName(StringToWString(desc.debugName).c_str());
    }

    // 初始化空闲列表
    m_freeList.clear();
    m_freeList.reserve(desc.numDescriptors);

    RHI_LOG_INFO("D3D12DescriptorHeap::Initialize: 成功创建描述符堆, 类型=%d, 容量=%u, 着色器可见=%s",
                 static_cast<int>(desc.type), desc.numDescriptors, 
                 desc.shaderVisible ? "是" : "否");

    return RHIResult::Success;
}

void D3D12DescriptorHeap::Shutdown()
{
    m_heap.Reset();
    m_cpuStart = {0};
    m_gpuStart = {0};
    m_descriptorSize = 0;
    m_capacity = 0;
    m_allocated = 0;
    m_shaderVisible = false;
    m_debugName.clear();
    m_freeList.clear();
}

DescriptorHandle D3D12DescriptorHeap::Allocate()
{
    if (m_allocated >= m_capacity && m_freeList.empty())
    {
        RHI_LOG_ERROR("D3D12DescriptorHeap::Allocate: 描述符堆已满, 容量=%u", m_capacity);
        return DescriptorHandle{};
    }

    uint32_t index;

    // 优先从空闲列表分配
    if (!m_freeList.empty())
    {
        index = m_freeList.back();
        m_freeList.pop_back();
    }
    else
    {
        index = m_allocated++;
    }

    DescriptorHandle handle;
    handle.cpuHandle = GetCPUDescriptorHandle(index).ptr;
    if (m_shaderVisible)
    {
        handle.gpuHandle = GetGPUDescriptorHandle(index).ptr;
    }
    handle.isValid = true;

    return handle;
}

void D3D12DescriptorHeap::Free(DescriptorHandle handle)
{
    if (!handle.isValid)
    {
        return;
    }

    // 计算索引
    uint32_t index = static_cast<uint32_t>((handle.cpuHandle - m_cpuStart.ptr) / m_descriptorSize);
    
    if (index >= m_capacity)
    {
        RHI_LOG_WARNING("D3D12DescriptorHeap::Free: 无效的描述符句柄");
        return;
    }

    // 添加到空闲列表
    m_freeList.push_back(index);
}

DescriptorHandle D3D12DescriptorHeap::GetHeapStart() const
{
    DescriptorHandle handle;
    handle.cpuHandle = m_cpuStart.ptr;
    handle.gpuHandle = m_gpuStart.ptr;
    handle.isValid = (m_heap != nullptr);
    return handle;
}

uint32_t D3D12DescriptorHeap::GetDescriptorCount() const
{
    return m_capacity;
}

uint32_t D3D12DescriptorHeap::GetUsedDescriptorCount() const
{
    return m_allocated - static_cast<uint32_t>(m_freeList.size());
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUDescriptorHandle(uint32_t index) const
{
    if (index >= m_capacity)
    {
        RHI_LOG_ERROR("D3D12DescriptorHeap::GetCPUDescriptorHandle: 索引越界, index=%u, capacity=%u",
                      index, m_capacity);
        return {0};
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = m_cpuStart.ptr + static_cast<UINT64>(index) * m_descriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUDescriptorHandle(uint32_t index) const
{
    if (!m_shaderVisible)
    {
        RHI_LOG_ERROR("D3D12DescriptorHeap::GetGPUDescriptorHandle: 堆不是着色器可见的");
        return {0};
    }

    if (index >= m_capacity)
    {
        RHI_LOG_ERROR("D3D12DescriptorHeap::GetGPUDescriptorHandle: 索引越界");
        return {0};
    }

    D3D12_GPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = m_gpuStart.ptr + static_cast<UINT64>(index) * m_descriptorSize;
    return handle;
}

// =============================================================================
// D3D12DescriptorAllocator 实现
// =============================================================================

D3D12DescriptorAllocator::D3D12DescriptorAllocator()
{
}

D3D12DescriptorAllocator::~D3D12DescriptorAllocator()
{
    Shutdown();
}

RHIResult D3D12DescriptorAllocator::Initialize(ID3D12Device* device,
                                                uint32_t shaderVisibleCount,
                                                uint32_t rtvCount,
                                                uint32_t dsvCount,
                                                uint32_t samplerCount)
{
    if (!device)
    {
        RHI_LOG_ERROR("D3D12DescriptorAllocator::Initialize: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    m_device = device;

    RHIResult result;

    // 创建着色器可见堆 (CBV/SRV/UAV)
    {
        DescriptorHeapDesc desc;
        desc.type = DescriptorHeapType::CBV_SRV_UAV;
        desc.numDescriptors = shaderVisibleCount;
        desc.shaderVisible = true;
        desc.debugName = "ShaderVisibleHeap";

        result = m_shaderVisibleHeap.Initialize(device, desc);
        if (result != RHIResult::Success)
        {
            RHI_LOG_ERROR("D3D12DescriptorAllocator::Initialize: 创建着色器可见堆失败");
            return result;
        }
    }

    // 创建 RTV 堆
    {
        DescriptorHeapDesc desc;
        desc.type = DescriptorHeapType::RTV;
        desc.numDescriptors = rtvCount;
        desc.shaderVisible = false;
        desc.debugName = "RTVHeap";

        result = m_rtvHeap.Initialize(device, desc);
        if (result != RHIResult::Success)
        {
            RHI_LOG_ERROR("D3D12DescriptorAllocator::Initialize: 创建 RTV 堆失败");
            return result;
        }
    }

    // 创建 DSV 堆
    {
        DescriptorHeapDesc desc;
        desc.type = DescriptorHeapType::DSV;
        desc.numDescriptors = dsvCount;
        desc.shaderVisible = false;
        desc.debugName = "DSVHeap";

        result = m_dsvHeap.Initialize(device, desc);
        if (result != RHIResult::Success)
        {
            RHI_LOG_ERROR("D3D12DescriptorAllocator::Initialize: 创建 DSV 堆失败");
            return result;
        }
    }

    // 创建采样器堆
    {
        DescriptorHeapDesc desc;
        desc.type = DescriptorHeapType::Sampler;
        desc.numDescriptors = samplerCount;
        desc.shaderVisible = true;
        desc.debugName = "SamplerHeap";

        result = m_samplerHeap.Initialize(device, desc);
        if (result != RHIResult::Success)
        {
            RHI_LOG_ERROR("D3D12DescriptorAllocator::Initialize: 创建采样器堆失败");
            return result;
        }
    }

    RHI_LOG_INFO("D3D12DescriptorAllocator::Initialize: 描述符分配器初始化成功");
    RHI_LOG_INFO("  - 着色器可见: %u", shaderVisibleCount);
    RHI_LOG_INFO("  - RTV: %u", rtvCount);
    RHI_LOG_INFO("  - DSV: %u", dsvCount);
    RHI_LOG_INFO("  - 采样器: %u", samplerCount);

    return RHIResult::Success;
}

void D3D12DescriptorAllocator::Shutdown()
{
    m_shaderVisibleHeap.Shutdown();
    m_rtvHeap.Shutdown();
    m_dsvHeap.Shutdown();
    m_samplerHeap.Shutdown();
    m_device = nullptr;
}

DescriptorHandle D3D12DescriptorAllocator::AllocateShaderVisible()
{
    return m_shaderVisibleHeap.Allocate();
}

DescriptorHandle D3D12DescriptorAllocator::AllocateRTV()
{
    return m_rtvHeap.Allocate();
}

DescriptorHandle D3D12DescriptorAllocator::AllocateDSV()
{
    return m_dsvHeap.Allocate();
}

DescriptorHandle D3D12DescriptorAllocator::AllocateSampler()
{
    return m_samplerHeap.Allocate();
}

void D3D12DescriptorAllocator::FreeShaderVisible(DescriptorHandle handle)
{
    m_shaderVisibleHeap.Free(handle);
}

void D3D12DescriptorAllocator::FreeRTV(DescriptorHandle handle)
{
    m_rtvHeap.Free(handle);
}

void D3D12DescriptorAllocator::FreeDSV(DescriptorHandle handle)
{
    m_dsvHeap.Free(handle);
}

void D3D12DescriptorAllocator::FreeSampler(DescriptorHandle handle)
{
    m_samplerHeap.Free(handle);
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12