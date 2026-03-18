// =============================================================================
// D3D12DescriptorAllocator.h - D3D12 描述符分配器
// =============================================================================

#pragma once

#if ENGINE_RHI_D3D12

#include "IDescriptorAllocator.h"

#include <d3d12.h>
#include <wrl/client.h>

namespace Engine::RHI::D3D12
{

// =============================================================================
// D3D12DescriptorHeap - D3D12 描述符堆实现
// =============================================================================

class D3D12DescriptorHeap : public IDescriptorHeap
{
  public:
    D3D12DescriptorHeap();
    ~D3D12DescriptorHeap() override;

    // 禁止拷贝
    D3D12DescriptorHeap(const D3D12DescriptorHeap&) = delete;
    D3D12DescriptorHeap& operator=(const D3D12DescriptorHeap&) = delete;

    // 允许移动
    D3D12DescriptorHeap(D3D12DescriptorHeap&& other) noexcept;
    D3D12DescriptorHeap& operator=(D3D12DescriptorHeap&& other) noexcept;

    // =========================================================================
    // 初始化/销毁
    // =========================================================================

    /// 初始化描述符堆
    /// @param device D3D12 设备
    /// @param desc 描述符堆描述
    /// @return 成功或错误码
    RHIResult Initialize(ID3D12Device* device, const DescriptorHeapDesc& desc);

    /// 销毁资源
    void Shutdown();

    // =========================================================================
    // IDescriptorHeap 接口实现
    // =========================================================================

    DescriptorHandle Allocate() override;
    void Free(DescriptorHandle handle) override;
    DescriptorHandle GetHeapStart() const override;
    uint32_t GetDescriptorCount() const override;
    uint32_t GetUsedDescriptorCount() const override;

    // =========================================================================
    // D3D12 特有方法
    // =========================================================================

    /// 获取 D3D12 描述符堆
    ID3D12DescriptorHeap* GetD3D12Heap() const { return m_heap.Get(); }

    /// 获取 CPU 句柄递增量
    uint32_t GetDescriptorSize() const { return m_descriptorSize; }

    /// 获取指定索引的 CPU 句柄
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index) const;

    /// 获取指定索引的 GPU 句柄
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index) const;

  private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart = {0};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart = {0};

    uint32_t m_descriptorSize = 0;
    uint32_t m_capacity = 0;
    uint32_t m_allocated = 0;

    bool m_shaderVisible = false;

    std::string m_debugName;

    // 空闲列表（简单实现，TODO: 更高效的管理）
    std::vector<uint32_t> m_freeList;
};

// =============================================================================
// D3D12DescriptorAllocator - D3D12 描述符分配器
// =============================================================================

class D3D12DescriptorAllocator
{
  public:
    D3D12DescriptorAllocator();
    ~D3D12DescriptorAllocator();

    // 禁止拷贝
    D3D12DescriptorAllocator(const D3D12DescriptorAllocator&) = delete;
    D3D12DescriptorAllocator& operator=(const D3D12DescriptorAllocator&) = delete;

    /// 初始化分配器
    /// @param device D3D12 设备
    /// @param shaderVisibleCount 着色器可见描述符数量
    /// @param rtvCount RTV 描述符数量
    /// @param dsvCount DSV 描述符数量
    /// @param samplerCount 采样器描述符数量
    RHIResult Initialize(ID3D12Device* device,
                         uint32_t shaderVisibleCount = 1024,
                         uint32_t rtvCount = 256,
                         uint32_t dsvCount = 64,
                         uint32_t samplerCount = 64);

    /// 销毁资源
    void Shutdown();

    // =========================================================================
    // 分配方法
    // =========================================================================

    /// 分配 CBV/SRV/UAV 描述符（着色器可见）
    DescriptorHandle AllocateShaderVisible();

    /// 分配 RTV 描述符
    DescriptorHandle AllocateRTV();

    /// 分配 DSV 描述符
    DescriptorHandle AllocateDSV();

    /// 分配采样器描述符
    DescriptorHandle AllocateSampler();

    /// 释放描述符
    void FreeShaderVisible(DescriptorHandle handle);
    void FreeRTV(DescriptorHandle handle);
    void FreeDSV(DescriptorHandle handle);
    void FreeSampler(DescriptorHandle handle);

    // =========================================================================
    // 访问器
    // =========================================================================

    D3D12DescriptorHeap* GetShaderVisibleHeap() { return &m_shaderVisibleHeap; }
    D3D12DescriptorHeap* GetRTVHeap() { return &m_rtvHeap; }
    D3D12DescriptorHeap* GetDSVHeap() { return &m_dsvHeap; }
    D3D12DescriptorHeap* GetSamplerHeap() { return &m_samplerHeap; }

  private:
    D3D12DescriptorHeap m_shaderVisibleHeap; // CBV_SRV_UAV, shader visible
    D3D12DescriptorHeap m_rtvHeap;           // Render Target View
    D3D12DescriptorHeap m_dsvHeap;           // Depth Stencil View
    D3D12DescriptorHeap m_samplerHeap;       // Sampler

    ID3D12Device* m_device = nullptr;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12