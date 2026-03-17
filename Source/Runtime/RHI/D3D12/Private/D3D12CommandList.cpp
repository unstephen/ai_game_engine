// =============================================================================
// D3D12CommandList.cpp - D3D12 命令列表实现
// =============================================================================

#include "D3D12CommandList.h"

#if ENGINE_RHI_D3D12

#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"

#include <sstream>

namespace Engine::RHI::D3D12 {

// =============================================================================
// 构造/析构
// =============================================================================

D3D12CommandList::D3D12CommandList()
    : m_type(D3D12_COMMAND_LIST_TYPE_DIRECT)
{
    // 初始化描述符堆数组
    ZeroMemory(m_descriptorHeaps, sizeof(m_descriptorHeaps));
}

D3D12CommandList::~D3D12CommandList() {
    Shutdown();
}

D3D12CommandList::D3D12CommandList(D3D12CommandList&& other) noexcept
    : m_commandList(std::move(other.m_commandList))
    , m_device(other.m_device)
    , m_commandAllocator(other.m_commandAllocator)
    , m_type(other.m_type)
    , m_isRecording(other.m_isRecording)
    , m_isClosed(other.m_isClosed)
    , m_isInitialized(other.m_isInitialized)
    , m_primitiveTopology(other.m_primitiveTopology)
    , m_currentPipelineState(other.m_currentPipelineState)
    , m_currentRootSignature(other.m_currentRootSignature)
    , m_indexBufferFormat(other.m_indexBufferFormat)
    , m_debugName(std::move(other.m_debugName))
{
    // 复制描述符堆数组
    for (int i = 0; i < 2; i++) {
        m_descriptorHeaps[i] = other.m_descriptorHeaps[i];
    }
    
    // 重置源对象
    other.m_device = nullptr;
    other.m_commandAllocator = nullptr;
    other.m_isRecording = false;
    other.m_isClosed = false;
    other.m_isInitialized = false;
    ZeroMemory(other.m_descriptorHeaps, sizeof(other.m_descriptorHeaps));
}

D3D12CommandList& D3D12CommandList::operator=(D3D12CommandList&& other) noexcept {
    if (this != &other) {
        // 先关闭当前资源
        Shutdown();
        
        // 移动资源
        m_commandList = std::move(other.m_commandList);
        m_device = other.m_device;
        m_commandAllocator = other.m_commandAllocator;
        m_type = other.m_type;
        m_isRecording = other.m_isRecording;
        m_isClosed = other.m_isClosed;
        m_isInitialized = other.m_isInitialized;
        m_primitiveTopology = other.m_primitiveTopology;
        m_currentPipelineState = other.m_currentPipelineState;
        m_currentRootSignature = other.m_currentRootSignature;
        m_indexBufferFormat = other.m_indexBufferFormat;
        m_debugName = std::move(other.m_debugName);
        
        // 复制描述符堆数组
        for (int i = 0; i < 2; i++) {
            m_descriptorHeaps[i] = other.m_descriptorHeaps[i];
        }
        
        // 重置源对象
        other.m_device = nullptr;
        other.m_commandAllocator = nullptr;
        other.m_isRecording = false;
        other.m_isClosed = false;
        other.m_isInitialized = false;
        ZeroMemory(other.m_descriptorHeaps, sizeof(other.m_descriptorHeaps));
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12CommandList::Initialize(
    D3D12Device* device,
    ID3D12CommandAllocator* allocator,
    D3D12_COMMAND_LIST_TYPE type)
{
    // 参数验证
    if (!device) {
        RHI_LOG_ERROR("D3D12CommandList::Initialize: 设备指针为空");
        return RHIResult::InvalidParameter;
    }
    
    if (!allocator) {
        RHI_LOG_ERROR("D3D12CommandList::Initialize: 命令分配器为空");
        return RHIResult::InvalidParameter;
    }
    
    // 如果已经初始化，先关闭
    if (m_isInitialized) {
        Shutdown();
    }
    
    // 保存设备和分配器引用
    m_device = device;
    m_commandAllocator = allocator;
    m_type = type;
    
    // 创建 D3D12 命令列表
    ID3D12Device* d3dDevice = device->GetD3D12Device();
    HRESULT hr = d3dDevice->CreateCommandList(
        0,                      // nodeMask（单 GPU 为 0）
        type,                   // 命令列表类型
        allocator,              // 命令分配器
        nullptr,                // 初始管线状态
        IID_PPV_ARGS(&m_commandList)
    );
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12CommandList::Initialize: 创建命令列表失败, HRESULT=0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }
    
    // 立即关闭命令列表（需要调用 Begin() 才能录制）
    hr = m_commandList->Close();
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12CommandList::Initialize: 关闭命令列表失败, HRESULT=0x%08X", hr);
        m_commandList.Reset();
        return RHIResult::InternalError;
    }
    
    m_isClosed = true;
    m_isRecording = false;
    m_isInitialized = true;
    
    // 重置状态
    m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_currentPipelineState = nullptr;
    m_currentRootSignature = nullptr;
    ZeroMemory(m_descriptorHeaps, sizeof(m_descriptorHeaps));
    m_indexBufferFormat = DXGI_FORMAT_R32_UINT;
    
    RHI_LOG_INFO("D3D12CommandList::Initialize: 命令列表创建成功");
    return RHIResult::Success;
}

void D3D12CommandList::Shutdown() {
    // 如果正在录制，先关闭
    if (m_isRecording) {
        End();
    }
    
    // 释放资源
    m_commandList.Reset();
    
    // 重置状态
    m_device = nullptr;
    m_commandAllocator = nullptr;
    m_isRecording = false;
    m_isClosed = false;
    m_isInitialized = false;
    m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_currentPipelineState = nullptr;
    m_currentRootSignature = nullptr;
    ZeroMemory(m_descriptorHeaps, sizeof(m_descriptorHeaps));
    m_debugName.clear();
}

// =============================================================================
// 生命周期管理
// =============================================================================

void D3D12CommandList::Begin() {
    if (!CheckRecordingState()) {
        return; // 已经在录制状态或未初始化
    }
    
    if (!m_commandList || !m_commandAllocator) {
        RHI_LOG_ERROR("D3D12CommandList::Begin: 命令列表或分配器无效");
        return;
    }
    
    // 重置命令列表
    HRESULT hr = m_commandList->Reset(m_commandAllocator, nullptr);
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12CommandList::Begin: 重置命令列表失败, HRESULT=0x%08X", hr);
        return;
    }
    
    m_isRecording = true;
    m_isClosed = false;
    
    // 重置绑定状态缓存
    m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_currentPipelineState = nullptr;
    m_currentRootSignature = nullptr;
    ZeroMemory(m_descriptorHeaps, sizeof(m_descriptorHeaps));
    
    RHI_LOG_DEBUG("D3D12CommandList::Begin: 命令列表开始录制");
}

void D3D12CommandList::End() {
    if (!m_commandList) {
        RHI_LOG_ERROR("D3D12CommandList::End: 命令列表无效");
        return;
    }
    
    if (!m_isRecording) {
        RHI_LOG_WARNING("D3D12CommandList::End: 命令列表未在录制状态");
        return;
    }
    
    // 关闭命令列表
    HRESULT hr = m_commandList->Close();
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12CommandList::End: 关闭命令列表失败, HRESULT=0x%08X", hr);
        return;
    }
    
    m_isRecording = false;
    m_isClosed = true;
    
    RHI_LOG_DEBUG("D3D12CommandList::End: 命令列表关闭成功");
}

void D3D12CommandList::Reset(ID3D12PipelineState* pso) {
    if (!m_commandList || !m_commandAllocator) {
        RHI_LOG_ERROR("D3D12CommandList::Reset: 命令列表或分配器无效");
        return;
    }
    
    // 如果正在录制，先关闭
    if (m_isRecording) {
        m_commandList->Close();
    }
    
    // 重置命令列表
    HRESULT hr = m_commandList->Reset(m_commandAllocator, pso);
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12CommandList::Reset: 重置命令列表失败, HRESULT=0x%08X", hr);
        return;
    }
    
    m_isRecording = true;
    m_isClosed = false;
    
    // 重置绑定状态缓存
    m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_currentPipelineState = pso;
    m_currentRootSignature = nullptr;
    ZeroMemory(m_descriptorHeaps, sizeof(m_descriptorHeaps));
}

// =============================================================================
// ICommandList 接口实现
// =============================================================================

void* D3D12CommandList::GetNativeCommandList() const {
    return m_commandList.Get();
}

uint32_t D3D12CommandList::GetType() const {
    return static_cast<uint32_t>(m_type);
}

// =============================================================================
// 资源屏障
// =============================================================================

void D3D12CommandList::ResourceBarrier(ITexture* texture, ResourceState newState) {
    if (!CheckRecordingState()) return;
    
    if (!texture) {
        RHI_LOG_ERROR("D3D12CommandList::ResourceBarrier: 纹理为空");
        return;
    }
    
    D3D12Texture* d3dTexture = static_cast<D3D12Texture*>(texture);
    ID3D12Resource* resource = d3dTexture->GetD3D12Resource();
    
    if (!resource) {
        RHI_LOG_ERROR("D3D12CommandList::ResourceBarrier: D3D12 资源无效");
        return;
    }
    
    D3D12_RESOURCE_STATES beforeState = d3dTexture->GetResourceState();
    D3D12_RESOURCE_STATES afterState = ToD3D12ResourceState(newState);
    
    // 如果状态相同，跳过
    if (beforeState == afterState) {
        return;
    }
    
    // 创建资源屏障
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = beforeState;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    m_commandList->ResourceBarrier(1, &barrier);
    
    // 更新纹理状态
    d3dTexture->SetResourceState(afterState);
}

void D3D12CommandList::ResourceBarrier(IBuffer* buffer, ResourceState newState) {
    if (!CheckRecordingState()) return;
    
    if (!buffer) {
        RHI_LOG_ERROR("D3D12CommandList::ResourceBarrier: 缓冲区为空");
        return;
    }
    
    D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);
    ID3D12Resource* resource = d3dBuffer->GetD3D12Resource();
    
    if (!resource) {
        RHI_LOG_ERROR("D3D12CommandList::ResourceBarrier: D3D12 资源无效");
        return;
    }
    
    D3D12_RESOURCE_STATES beforeState = d3dBuffer->GetResourceState();
    D3D12_RESOURCE_STATES afterState = ToD3D12ResourceState(newState);
    
    // 如果状态相同，跳过
    if (beforeState == afterState) {
        return;
    }
    
    // 创建资源屏障
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = beforeState;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    m_commandList->ResourceBarrier(1, &barrier);
    
    // 更新缓冲区状态
    d3dBuffer->SetResourceState(afterState);
}

// =============================================================================
// 清除操作
// =============================================================================

void D3D12CommandList::ClearRenderTarget(ITexture* renderTarget, const float clearColor[4]) {
    if (!CheckRecordingState()) return;
    
    if (!renderTarget) {
        RHI_LOG_ERROR("D3D12CommandList::ClearRenderTarget: 渲染目标为空");
        return;
    }
    
    D3D12Texture* d3dTexture = static_cast<D3D12Texture*>(renderTarget);
    ID3D12Resource* resource = d3dTexture->GetD3D12Resource();
    
    if (!resource) {
        RHI_LOG_ERROR("D3D12CommandList::ClearRenderTarget: D3D12 资源无效");
        return;
    }
    
    // 注意：这里需要 RTV 描述符句柄
    // 简化实现：使用 ClearRenderTargetView 需要调用者提供 RTV
    // 完整实现需要从 Framebuffer 或 RenderPass 获取 RTV
    RHI_LOG_WARNING("D3D12CommandList::ClearRenderTarget: 需要提供 RTV 描述符句柄");
}

void D3D12CommandList::ClearDepthStencil(
    ITexture* depthStencil,
    bool clearDepth,
    bool clearStencil,
    float depth,
    uint8_t stencil)
{
    if (!CheckRecordingState()) return;
    
    if (!depthStencil) {
        RHI_LOG_ERROR("D3D12CommandList::ClearDepthStencil: 深度模板为空");
        return;
    }
    
    // 注意：这里需要 DSV 描述符句柄
    // 简化实现：使用 ClearDepthStencilView 需要调用者提供 DSV
    // 完整实现需要从 Framebuffer 或 RenderPass 获取 DSV
    RHI_LOG_WARNING("D3D12CommandList::ClearDepthStencil: 需要提供 DSV 描述符句柄");
}

// =============================================================================
// 状态设置
// =============================================================================

void D3D12CommandList::SetPipelineState(IPipelineState* pipeline) {
    if (!CheckRecordingState()) return;
    
    if (!pipeline) {
        RHI_LOG_ERROR("D3D12CommandList::SetPipelineState: 管线状态为空");
        return;
    }
    
    // 获取原生管线状态
    ID3D12PipelineState* pso = static_cast<ID3D12PipelineState*>(pipeline->GetNativeHandle());
    
    if (!pso) {
        RHI_LOG_ERROR("D3D12CommandList::SetPipelineState: 原生管线状态无效");
        return;
    }
    
    // 状态缓存：避免冗余设置
    if (m_currentPipelineState == pso) {
        return;
    }
    
    m_commandList->SetPipelineState(pso);
    m_currentPipelineState = pso;
}

void D3D12CommandList::SetRootSignature(IRootSignature* rootSignature) {
    if (!CheckRecordingState()) return;
    
    if (!rootSignature) {
        RHI_LOG_ERROR("D3D12CommandList::SetRootSignature: 根签名为空");
        return;
    }
    
    // 获取原生根签名
    ID3D12RootSignature* rootSig = static_cast<ID3D12RootSignature*>(rootSignature->GetNativeHandle());
    
    if (!rootSig) {
        RHI_LOG_ERROR("D3D12CommandList::SetRootSignature: 原生根签名无效");
        return;
    }
    
    // 状态缓存：避免冗余设置
    if (m_currentRootSignature == rootSig) {
        return;
    }
    
    // 根据命令列表类型设置不同的根签名
    switch (m_type) {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
        case D3D12_COMMAND_LIST_TYPE_BUNDLE:
            m_commandList->SetGraphicsRootSignature(rootSig);
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            m_commandList->SetComputeRootSignature(rootSig);
            break;
        default:
            RHI_LOG_WARNING("D3D12CommandList::SetRootSignature: 未知的命令列表类型");
            m_commandList->SetGraphicsRootSignature(rootSig);
            break;
    }
    
    m_currentRootSignature = rootSig;
}

void D3D12CommandList::SetRenderTargets(
    ITexture* const* renderTargets,
    uint32_t count,
    ITexture* depthStencil)
{
    if (!CheckRecordingState()) return;
    
    if (count == 0 && !depthStencil) {
        RHI_LOG_WARNING("D3D12CommandList::SetRenderTargets: 没有渲染目标或深度模板");
        return;
    }
    
    // 注意：完整实现需要从 Framebuffer 获取 RTV/DSV 描述符
    // 这里简化实现，实际使用时需要 RenderPass 或 Framebuffer 提供
    RHI_LOG_WARNING("D3D12CommandList::SetRenderTargets: 需要 RTV/DSV 描述符句柄");
}

void D3D12CommandList::SetViewport(
    float x, float y,
    float width, float height,
    float minDepth,
    float maxDepth)
{
    if (!CheckRecordingState()) return;
    
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = minDepth;
    viewport.MaxDepth = maxDepth;
    
    m_commandList->RSSetViewports(1, &viewport);
}

void D3D12CommandList::SetScissorRect(
    int32_t left, int32_t top,
    int32_t right, int32_t bottom)
{
    if (!CheckRecordingState()) return;
    
    D3D12_RECT scissorRect = {};
    scissorRect.left = left;
    scissorRect.top = top;
    scissorRect.right = right;
    scissorRect.bottom = bottom;
    
    m_commandList->RSSetScissorRects(1, &scissorRect);
}

void D3D12CommandList::SetPrimitiveTopology(PrimitiveTopology topology) {
    if (!CheckRecordingState()) return;
    
    D3D12_PRIMITIVE_TOPOLOGY d3dTopology = ToD3D12PrimitiveTopology(topology);
    
    // 状态缓存：避免冗余设置
    if (m_primitiveTopology == d3dTopology) {
        return;
    }
    
    m_commandList->IASetPrimitiveTopology(d3dTopology);
    m_primitiveTopology = d3dTopology;
}

// =============================================================================
// 资源绑定
// =============================================================================

void D3D12CommandList::SetVertexBuffer(uint32_t slot, IBuffer* buffer, uint64_t offset) {
    if (!CheckRecordingState()) return;
    
    if (slot >= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) {
        RHI_LOG_ERROR("D3D12CommandList::SetVertexBuffer: 顶点缓冲槽索引越界, slot=%u", slot);
        return;
    }
    
    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    
    if (buffer) {
        D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);
        ID3D12Resource* resource = d3dBuffer->GetD3D12Resource();
        
        if (resource) {
            vbView.BufferLocation = resource->GetGPUVirtualAddress() + offset;
            vbView.SizeInBytes = static_cast<UINT>(d3dBuffer->GetSize() - offset);
            vbView.StrideInBytes = d3dBuffer->GetStride();
        }
    }
    
    m_commandList->IASetVertexBuffers(slot, 1, &vbView);
}

void D3D12CommandList::SetIndexBuffer(IBuffer* buffer, uint64_t offset) {
    if (!CheckRecordingState()) return;
    
    D3D12_INDEX_BUFFER_VIEW ibView = {};
    
    if (buffer) {
        D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);
        ID3D12Resource* resource = d3dBuffer->GetD3D12Resource();
        
        if (resource) {
            ibView.BufferLocation = resource->GetGPUVirtualAddress() + offset;
            ibView.SizeInBytes = static_cast<UINT>(d3dBuffer->GetSize() - offset);
            ibView.Format = m_indexBufferFormat;
        }
    }
    
    m_commandList->IASetIndexBuffer(&ibView);
}

void D3D12CommandList::SetIndexBuffer(IBuffer* buffer, uint64_t offset, DXGI_FORMAT format) {
    m_indexBufferFormat = format;
    SetIndexBuffer(buffer, offset);
}

// =============================================================================
// 绘制调用
// =============================================================================

void D3D12CommandList::Draw(uint32_t vertexCount, uint32_t startVertex) {
    if (!CheckRecordingState()) return;
    
    m_commandList->DrawInstanced(vertexCount, 1, startVertex, 0);
}

void D3D12CommandList::DrawIndexed(
    uint32_t indexCount,
    uint32_t startIndex,
    int32_t baseVertex)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->DrawIndexedInstanced(indexCount, 1, startIndex, baseVertex, 0);
}

void D3D12CommandList::DrawInstanced(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t startVertex,
    uint32_t startInstance)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void D3D12CommandList::DrawIndexedInstanced(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t startIndex,
    int32_t baseVertex,
    uint32_t startInstance)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

// =============================================================================
// 高级资源绑定
// =============================================================================

void D3D12CommandList::SetGraphicsRootConstantBufferView(
    uint32_t rootParameterIndex,
    uint64_t address)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, address);
}

void D3D12CommandList::SetGraphicsRootShaderResourceView(
    uint32_t rootParameterIndex,
    uint64_t address)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, address);
}

void D3D12CommandList::SetGraphicsRootUnorderedAccessView(
    uint32_t rootParameterIndex,
    uint64_t address)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, address);
}

void D3D12CommandList::SetComputeRootConstantBufferView(
    uint32_t rootParameterIndex,
    uint64_t address)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->SetComputeRootConstantBufferView(rootParameterIndex, address);
}

// =============================================================================
// 描述符堆绑定
// =============================================================================

void D3D12CommandList::SetDescriptorHeaps(ID3D12DescriptorHeap* heap) {
    if (!CheckRecordingState()) return;
    
    if (heap) {
        m_commandList->SetDescriptorHeaps(1, &heap);
        
        // 缓存描述符堆
        D3D12_DESCRIPTOR_HEAP_TYPE type = heap->GetDesc().Type;
        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
            m_descriptorHeaps[0] = heap;
        } else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
            m_descriptorHeaps[1] = heap;
        }
    }
}

void D3D12CommandList::SetDescriptorHeaps(
    ID3D12DescriptorHeap* const* heaps,
    uint32_t count)
{
    if (!CheckRecordingState()) return;
    
    if (count > 0 && heaps) {
        m_commandList->SetDescriptorHeaps(count, heaps);
        
        // 缓存描述符堆
        for (uint32_t i = 0; i < count && i < 2; i++) {
            if (heaps[i]) {
                D3D12_DESCRIPTOR_HEAP_TYPE type = heaps[i]->GetDesc().Type;
                if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
                    m_descriptorHeaps[0] = heaps[i];
                } else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
                    m_descriptorHeaps[1] = heaps[i];
                }
            }
        }
    }
}

// =============================================================================
// 计算调度
// =============================================================================

void D3D12CommandList::Dispatch(
    uint32_t groupCountX,
    uint32_t groupCountY,
    uint32_t groupCountZ)
{
    if (!CheckRecordingState()) return;
    
    m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

// =============================================================================
// 复制命令
// =============================================================================

void D3D12CommandList::CopyBufferRegion(
    ID3D12Resource* dest,
    uint64_t destOffset,
    ID3D12Resource* src,
    uint64_t srcOffset,
    uint64_t size)
{
    if (!CheckRecordingState()) return;
    
    if (!dest || !src) {
        RHI_LOG_ERROR("D3D12CommandList::CopyBufferRegion: 源或目标资源为空");
        return;
    }
    
    m_commandList->CopyBufferRegion(dest, destOffset, src, srcOffset, size);
}

void D3D12CommandList::CopyTextureRegion(
    ID3D12Resource* dest, uint32_t destX, uint32_t destY, uint32_t destZ,
    ID3D12Resource* src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
    uint32_t width, uint32_t height, uint32_t depth)
{
    if (!CheckRecordingState()) return;
    
    if (!dest || !src) {
        RHI_LOG_ERROR("D3D12CommandList::CopyTextureRegion: 源或目标资源为空");
        return;
    }
    
    // 创建目标位置
    D3D12_TEXTURE_COPY_LOCATION destLocation = {};
    destLocation.pResource = dest;
    destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destLocation.SubresourceIndex = 0;
    
    // 创建源位置
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = src;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;
    
    // 创建源盒子
    D3D12_BOX srcBox = {};
    srcBox.left = srcX;
    srcBox.top = srcY;
    srcBox.front = srcZ;
    srcBox.right = srcX + width;
    srcBox.bottom = srcY + height;
    srcBox.back = srcZ + depth;
    
    m_commandList->CopyTextureRegion(
        &destLocation, destX, destY, destZ,
        &srcLocation, &srcBox
    );
}

void D3D12CommandList::CopyResource(ID3D12Resource* dest, ID3D12Resource* src) {
    if (!CheckRecordingState()) return;
    
    if (!dest || !src) {
        RHI_LOG_ERROR("D3D12CommandList::CopyResource: 源或目标资源为空");
        return;
    }
    
    m_commandList->CopyResource(dest, src);
}

// =============================================================================
// 资源屏障扩展
// =============================================================================

void D3D12CommandList::TransitionBarrier(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after,
    UINT subresource)
{
    if (!CheckRecordingState()) return;
    
    if (!resource) {
        RHI_LOG_ERROR("D3D12CommandList::TransitionBarrier: 资源为空");
        return;
    }
    
    // 如果状态相同，跳过
    if (before == after) {
        return;
    }
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = subresource;
    
    m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12CommandList::UAVBarrier(ID3D12Resource* resource) {
    if (!CheckRecordingState()) return;
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource;
    
    m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12CommandList::AliasBarrier(ID3D12Resource* before, ID3D12Resource* after) {
    if (!CheckRecordingState()) return;
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Aliasing.pResourceBefore = before;
    barrier.Aliasing.pResourceAfter = after;
    
    m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12CommandList::ResourceBarriers(
    const D3D12_RESOURCE_BARRIER* barriers,
    uint32_t count)
{
    if (!CheckRecordingState()) return;
    
    if (!barriers || count == 0) {
        return;
    }
    
    m_commandList->ResourceBarrier(count, barriers);
}

// =============================================================================
// 调试支持
// =============================================================================

void D3D12CommandList::SetMarker(const char* name) {
    if (!m_commandList || !name) {
        return;
    }
    
    // 转换为宽字符串
    wchar_t wideName[256];
    int result = MultiByteToWideChar(CP_UTF8, 0, name, -1, wideName, 256);
    
    if (result > 0) {
        // 使用 PIX 事件
        // PIX 事件颜色：0 表示默认颜色
        m_commandList->SetMarker(0, wideName);
    }
}

void D3D12CommandList::BeginEvent(const char* name) {
    if (!m_commandList || !name) {
        return;
    }
    
    // 转换为宽字符串
    wchar_t wideName[256];
    int result = MultiByteToWideChar(CP_UTF8, 0, name, -1, wideName, 256);
    
    if (result > 0) {
        // 使用 PIX 事件
        // PIX 事件颜色：0 表示默认颜色
        m_commandList->BeginEvent(0, wideName);
    }
}

void D3D12CommandList::EndEvent() {
    if (!m_commandList) {
        return;
    }
    
    m_commandList->EndEvent();
}

// =============================================================================
// 内部方法
// =============================================================================

bool D3D12CommandList::CheckRecordingState() const {
    if (!m_isInitialized) {
        RHI_LOG_ERROR("D3D12CommandList: 命令列表未初始化");
        return false;
    }
    
    if (!m_commandList) {
        RHI_LOG_ERROR("D3D12CommandList: D3D12 命令列表无效");
        return false;
    }
    
    if (!m_isRecording) {
        RHI_LOG_ERROR("D3D12CommandList: 命令列表未在录制状态");
        return false;
    }
    
    return true;
}

D3D12_PRIMITIVE_TOPOLOGY D3D12CommandList::ToD3D12PrimitiveTopology(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
            RHI_LOG_WARNING("D3D12CommandList: 未知的图元拓扑类型，使用 TriangleList");
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

D3D12_RESOURCE_STATES D3D12CommandList::ToD3D12ResourceState(ResourceState state) {
    switch (state) {
        case ResourceState::Undefined:
            return D3D12_RESOURCE_STATE_COMMON;
        case ResourceState::Common:
            return D3D12_RESOURCE_STATE_COMMON;
        case ResourceState::VertexBuffer:
            return D3D12_RESOURCE_STATE_VERTEX_BUFFER;
        case ResourceState::IndexBuffer:
            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case ResourceState::ConstantBuffer:
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case ResourceState::ShaderResource:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case ResourceState::UnorderedAccess:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case ResourceState::RenderTarget:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ResourceState::DepthWrite:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case ResourceState::DepthRead:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case ResourceState::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        case ResourceState::CopySource:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceState::CopyDest:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceState::GenericRead:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        default:
            RHI_LOG_WARNING("D3D12CommandList: 未知的资源状态，使用 Common");
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

DXGI_FORMAT D3D12CommandList::ToDXGIFormat(Format format) {
    switch (format) {
        case Format::Unknown:
            return DXGI_FORMAT_UNKNOWN;
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
            RHI_LOG_WARNING("D3D12CommandList: 未知的格式，使用 UNKNOWN");
            return DXGI_FORMAT_UNKNOWN;
    }
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12