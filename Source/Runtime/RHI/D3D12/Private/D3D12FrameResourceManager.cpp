// =============================================================================
// D3D12FrameResourceManager.cpp - D3D12 帧资源管理器实现
// =============================================================================

#include "D3D12FrameResourceManager.h"

#if ENGINE_RHI_D3D12

#include <Windows.h>
#include <sstream>

namespace Engine::RHI::D3D12 {

// =============================================================================
// 构造/析构
// =============================================================================

D3D12FrameResourceManager::D3D12FrameResourceManager(D3D12Device* device, uint32_t numFrames)
    : m_device(device)
    , m_numFrames(numFrames)
    , m_currentFrameIndex(0)
    , m_nextFenceValue(1)
{
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 构造函数 (帧数: %u)", numFrames);
    
    // 预分配帧上下文数组
    m_frameContexts.reserve(numFrames);
}

D3D12FrameResourceManager::~D3D12FrameResourceManager() {
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 析构函数");
    Shutdown();
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12FrameResourceManager::Initialize() {
    RHI_LOG_INFO("D3D12FrameResourceManager: 开始初始化...");
    
    // 参数验证
    if (!m_device) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 设备指针为空");
        return RHIResult::InvalidParameter;
    }
    
    ID3D12Device* d3dDevice = m_device->GetD3D12Device();
    if (!d3dDevice) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: D3D12 设备未初始化");
        return RHIResult::InvalidParameter;
    }
    
    if (m_numFrames == 0 || m_numFrames > 16) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 无效的帧数 %u (范围: 1-16)", m_numFrames);
        return RHIResult::InvalidParameter;
    }
    
    // 如果已经初始化，先关闭
    if (m_isInitialized) {
        RHI_LOG_WARNING("D3D12FrameResourceManager: 已初始化，重新初始化");
        Shutdown();
    }
    
    HRESULT hr = S_OK;
    
    // 创建帧上下文
    m_frameContexts.clear();
    m_frameContexts.resize(m_numFrames);
    
    for (uint32_t i = 0; i < m_numFrames; ++i) {
        D3D12FrameContext& ctx = m_frameContexts[i];
        ctx.frameIndex = i;
        ctx.fenceValue = 0;
        
        // ========== 创建围栏 ==========
        
        hr = d3dDevice->CreateFence(
            0,                          // 初始值
            D3D12_FENCE_FLAG_NONE,      // 标志
            IID_PPV_ARGS(&ctx.fence)
        );
        
        if (FAILED(hr)) {
            RHI_LOG_ERROR("D3D12FrameResourceManager: 创建围栏失败 (帧 %u, HRESULT: 0x%08X)", i, hr);
            Shutdown();
            return RHIResult::InternalError;
        }
        
        // 设置围栏名称（用于调试）
        std::wostringstream fenceName;
        fenceName << L"FrameContext_Fence_" << i;
        ctx.fence->SetName(fenceName.str().c_str());
        
        // ========== 创建围栏事件 ==========
        
        ctx.fenceEvent = CreateEvent(
            nullptr,    // 默认安全属性
            FALSE,      // 自动重置
            FALSE,      // 初始状态：无信号
            nullptr     // 无名称
        );
        
        if (!ctx.fenceEvent) {
            RHI_LOG_ERROR("D3D12FrameResourceManager: 创建围栏事件失败 (帧 %u, 错误: %u)", 
                i, GetLastError());
            Shutdown();
            return RHIResult::OutOfMemory;
        }
        
        // ========== 创建命令分配器 ==========
        
        hr = d3dDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,  // 直接命令列表类型
            IID_PPV_ARGS(&ctx.commandAllocator)
        );
        
        if (FAILED(hr)) {
            RHI_LOG_ERROR("D3D12FrameResourceManager: 创建命令分配器失败 (帧 %u, HRESULT: 0x%08X)", 
                i, hr);
            Shutdown();
            return RHIResult::InternalError;
        }
        
        // 设置命令分配器名称（用于调试）
        std::wostringstream allocatorName;
        allocatorName << L"FrameContext_CommandAllocator_" << i;
        ctx.commandAllocator->SetName(allocatorName.str().c_str());
        
        // 初始化空容器
        ctx.uploadBuffers.clear();
        ctx.temporaryResources.clear();
        ctx.pendingDeletions.clear();
        
        ctx.isInitialized = true;
        
        RHI_LOG_DEBUG("D3D12FrameResourceManager: 帧 %u 初始化完成", i);
    }
    
    // 重置状态
    m_currentFrameIndex = 0;
    m_nextFenceValue = 1;
    m_isInitialized = true;
    
    RHI_LOG_INFO("D3D12FrameResourceManager: 初始化完成");
    RHI_LOG_INFO("  - 帧缓冲数量: %u", m_numFrames);
    RHI_LOG_INFO("  - 当前帧索引: %u", m_currentFrameIndex);
    
    return RHIResult::Success;
}

// =============================================================================
// 关闭
// =============================================================================

void D3D12FrameResourceManager::Shutdown() {
    RHI_LOG_INFO("D3D12FrameResourceManager: 开始关闭...");
    
    if (!m_isInitialized) {
        RHI_LOG_DEBUG("D3D12FrameResourceManager: 未初始化，无需关闭");
        return;
    }
    
    // 等待所有帧完成
    WaitForAllFrames();
    
    // 清理每个帧上下文
    for (uint32_t i = 0; i < m_frameContexts.size(); ++i) {
        D3D12FrameContext& ctx = m_frameContexts[i];
        
        // 清理临时资源
        ctx.uploadBuffers.clear();
        ctx.temporaryResources.clear();
        
        // 执行延迟删除
        for (ID3D12Resource* resource : ctx.pendingDeletions) {
            if (resource) {
                resource->Release();
            }
        }
        ctx.pendingDeletions.clear();
        
        // 关闭围栏事件
        if (ctx.fenceEvent) {
            CloseHandle(ctx.fenceEvent);
            ctx.fenceEvent = nullptr;
        }
        
        // 释放 D3D12 对象（ComPtr 自动管理）
        ctx.fence.Reset();
        ctx.commandAllocator.Reset();
        
        ctx.isInitialized = false;
        
        RHI_LOG_DEBUG("D3D12FrameResourceManager: 帧 %u 已清理", i);
    }
    
    m_frameContexts.clear();
    m_isInitialized = false;
    
    RHI_LOG_INFO("D3D12FrameResourceManager: 关闭完成");
}

// =============================================================================
// BeginFrame - 开始新帧
// =============================================================================

void D3D12FrameResourceManager::BeginFrame() {
    RHI_LOG_DEBUG("D3D12FrameResourceManager: BeginFrame (帧索引: %u)", m_currentFrameIndex);
    
    if (!m_isInitialized) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 未初始化，无法开始帧");
        return;
    }
    
    D3D12FrameContext& ctx = m_frameContexts[m_currentFrameIndex];
    
    // ========== 1. 等待 GPU 完成该帧索引的上一轮渲染 ==========
    
    if (ctx.fenceValue > 0) {
        RHI_LOG_DEBUG("D3D12FrameResourceManager: 等待 GPU 完成帧 %u (围栏值: %llu)", 
            m_currentFrameIndex, ctx.fenceValue);
        
        WaitForFence(ctx.fence.Get(), ctx.fenceValue, ctx.fenceEvent);
    }
    
    // ========== 2. 重置命令分配器 ==========
    
    // 重置前必须确保 GPU 已完成使用该分配器
    HRESULT hr = ctx.commandAllocator->Reset();
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 重置命令分配器失败 (HRESULT: 0x%08X)", hr);
        // 继续执行，可能设备已丢失
    }
    
    // ========== 3. 清理临时资源 ==========
    
    CleanupFrameResources(ctx);
    
    // ========== 4. 帧开始日志 ==========
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 帧 %u 已准备就绪", m_currentFrameIndex);
}

// =============================================================================
// EndFrame - 结束当前帧
// =============================================================================

void D3D12FrameResourceManager::EndFrame() {
    RHI_LOG_DEBUG("D3D12FrameResourceManager: EndFrame (帧索引: %u)", m_currentFrameIndex);
    
    if (!m_isInitialized) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 未初始化，无法结束帧");
        return;
    }
    
    D3D12FrameContext& ctx = m_frameContexts[m_currentFrameIndex];
    
    // ========== 1. 更新围栏值 ==========
    
    ctx.fenceValue = m_nextFenceValue++;
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 帧 %u 围栏值更新为 %llu", 
        m_currentFrameIndex, ctx.fenceValue);
    
    // 注意：围栏信号由调用者在提交命令队列后执行
    // m_device->GetCommandQueue()->Signal(ctx.fence.Get(), ctx.fenceValue);
    
    // ========== 2. 切换到下一帧 ==========
    
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_numFrames;
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 切换到帧 %u", m_currentFrameIndex);
}

// =============================================================================
// WaitForFrame - 等待指定帧完成
// =============================================================================

void D3D12FrameResourceManager::WaitForFrame(uint32_t frameIndex) {
    RHI_LOG_DEBUG("D3D12FrameResourceManager: WaitForFrame(%u)", frameIndex);
    
    if (!m_isInitialized) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 未初始化");
        return;
    }
    
    if (!IsValidFrameIndex(frameIndex)) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 无效的帧索引 %u (范围: 0-%u)", 
            frameIndex, m_numFrames - 1);
        return;
    }
    
    const D3D12FrameContext& ctx = m_frameContexts[frameIndex];
    
    if (ctx.fenceValue > 0) {
        WaitForFence(ctx.fence.Get(), ctx.fenceValue, ctx.fenceEvent);
    }
}

// =============================================================================
// GetFrameFenceValue - 获取帧围栏值
// =============================================================================

uint64_t D3D12FrameResourceManager::GetFrameFenceValue(uint32_t frameIndex) const {
    if (!IsValidFrameIndex(frameIndex)) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 无效的帧索引 %u", frameIndex);
        return 0;
    }
    
    return m_frameContexts[frameIndex].fenceValue;
}

// =============================================================================
// D3D12 特定接口
// =============================================================================

ID3D12CommandAllocator* D3D12FrameResourceManager::GetCurrentCommandAllocator() const {
    if (!m_isInitialized) {
        return nullptr;
    }
    
    return m_frameContexts[m_currentFrameIndex].commandAllocator.Get();
}

ID3D12Fence* D3D12FrameResourceManager::GetCurrentFence() const {
    if (!m_isInitialized) {
        return nullptr;
    }
    
    return m_frameContexts[m_currentFrameIndex].fence.Get();
}

uint64_t D3D12FrameResourceManager::GetCurrentFenceValue() const {
    if (!m_isInitialized) {
        return 0;
    }
    
    return m_frameContexts[m_currentFrameIndex].fenceValue;
}

HANDLE D3D12FrameResourceManager::GetCurrentFenceEvent() const {
    if (!m_isInitialized) {
        return nullptr;
    }
    
    return m_frameContexts[m_currentFrameIndex].fenceEvent;
}

// =============================================================================
// 资源注册
// =============================================================================

void D3D12FrameResourceManager::RegisterUploadBuffer(ID3D12Resource* buffer) {
    if (!m_isInitialized) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 未初始化，无法注册资源");
        return;
    }
    
    if (!buffer) {
        RHI_LOG_WARNING("D3D12FrameResourceManager: 空缓冲区指针，忽略");
        return;
    }
    
    D3D12FrameContext& ctx = m_frameContexts[m_currentFrameIndex];
    
    // 增加引用计数
    buffer->AddRef();
    
    // 包装到 ComPtr 中自动管理
    ctx.uploadBuffers.emplace_back(buffer);
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 上传缓冲区已注册到帧 %u", m_currentFrameIndex);
}

void D3D12FrameResourceManager::RegisterTemporaryResource(ID3D12Resource* resource) {
    if (!m_isInitialized) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 未初始化，无法注册资源");
        return;
    }
    
    if (!resource) {
        RHI_LOG_WARNING("D3D12FrameResourceManager: 空资源指针，忽略");
        return;
    }
    
    D3D12FrameContext& ctx = m_frameContexts[m_currentFrameIndex];
    
    // 增加引用计数
    resource->AddRef();
    
    // 包装到 ComPtr 中自动管理
    ctx.temporaryResources.emplace_back(resource);
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 临时资源已注册到帧 %u", m_currentFrameIndex);
}

void D3D12FrameResourceManager::DelayedDeleteResource(ID3D12Resource* resource) {
    if (!m_isInitialized) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 未初始化，无法延迟删除");
        return;
    }
    
    if (!resource) {
        RHI_LOG_WARNING("D3D12FrameResourceManager: 空资源指针，忽略");
        return;
    }
    
    D3D12FrameContext& ctx = m_frameContexts[m_currentFrameIndex];
    ctx.pendingDeletions.push_back(resource);
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 资源已加入延迟删除列表 (帧 %u)", m_currentFrameIndex);
}

// =============================================================================
// 状态查询
// =============================================================================

bool D3D12FrameResourceManager::IsFrameComplete(uint32_t frameIndex) const {
    if (!m_isInitialized || !IsValidFrameIndex(frameIndex)) {
        return false;
    }
    
    const D3D12FrameContext& ctx = m_frameContexts[frameIndex];
    
    // 如果围栏值为 0，说明该帧从未被使用
    if (ctx.fenceValue == 0) {
        return true;
    }
    
    // 检查 GPU 是否已完成
    UINT64 completedValue = ctx.fence->GetCompletedValue();
    return completedValue >= ctx.fenceValue;
}

void D3D12FrameResourceManager::WaitForAllFrames() {
    RHI_LOG_DEBUG("D3D12FrameResourceManager: WaitForAllFrames");
    
    if (!m_isInitialized) {
        return;
    }
    
    for (uint32_t i = 0; i < m_numFrames; ++i) {
        D3D12FrameContext& ctx = m_frameContexts[i];
        
        if (ctx.fenceValue > 0) {
            WaitForFence(ctx.fence.Get(), ctx.fenceValue, ctx.fenceEvent);
        }
    }
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 所有帧已完成");
}

// =============================================================================
// 内部辅助方法
// =============================================================================

void D3D12FrameResourceManager::WaitForFence(
    ID3D12Fence* fence,
    uint64_t fenceValue,
    HANDLE fenceEvent
) {
    if (!fence || !fenceEvent) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: 无效的围栏或事件句柄");
        return;
    }
    
    // 检查是否已完成（快速路径）
    UINT64 completedValue = fence->GetCompletedValue();
    
    if (completedValue >= fenceValue) {
        RHI_LOG_DEBUG("D3D12FrameResourceManager: 围栏已完成 (值: %llu >= %llu)", 
            completedValue, fenceValue);
        return;
    }
    
    RHI_LOG_DEBUG("D3D12FrameResourceManager: 等待围栏 (当前: %llu, 目标: %llu)", 
        completedValue, fenceValue);
    
    // 设置事件通知
    HRESULT hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
    
    if (FAILED(hr)) {
        RHI_LOG_ERROR("D3D12FrameResourceManager: SetEventOnCompletion 失败 (HRESULT: 0x%08X)", hr);
        return;
    }
    
    // 等待事件
    DWORD waitResult = WaitForSingleObject(fenceEvent, INFINITE);
    
    switch (waitResult) {
        case WAIT_OBJECT_0:
            // 成功
            RHI_LOG_DEBUG("D3D12FrameResourceManager: 围栏等待完成");
            break;
            
        case WAIT_FAILED:
            RHI_LOG_ERROR("D3D12FrameResourceManager: WaitForSingleObject 失败 (错误: %u)", 
                GetLastError());
            break;
            
        default:
            RHI_LOG_WARNING("D3D12FrameResourceManager: WaitForSingleObject 意外返回值: %u", 
                waitResult);
            break;
    }
}

void D3D12FrameResourceManager::CleanupFrameResources(D3D12FrameContext& ctx) {
    // 统计清理数量
    size_t uploadCount = ctx.uploadBuffers.size();
    size_t tempCount = ctx.temporaryResources.size();
    size_t deleteCount = ctx.pendingDeletions.size();
    
    // 清理上传缓冲区（ComPtr 自动释放引用）
    ctx.uploadBuffers.clear();
    
    // 清理临时资源（ComPtr 自动释放引用）
    ctx.temporaryResources.clear();
    
    // 执行延迟删除
    for (ID3D12Resource* resource : ctx.pendingDeletions) {
        if (resource) {
            // 获取资源名称（用于调试）
#ifdef RHI_DEBUG
            wchar_t name[256] = {0};
            UINT size = sizeof(name);
            resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
            if (name[0] != 0) {
                RHI_LOG_DEBUG("D3D12FrameResourceManager: 延迟删除资源: %ws", name);
            }
#endif
            
            resource->Release();
        }
    }
    ctx.pendingDeletions.clear();
    
    if (uploadCount > 0 || tempCount > 0 || deleteCount > 0) {
        RHI_LOG_DEBUG("D3D12FrameResourceManager: 帧 %u 清理完成 (上传: %zu, 临时: %zu, 删除: %zu)",
            ctx.frameIndex, uploadCount, tempCount, deleteCount);
    }
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12