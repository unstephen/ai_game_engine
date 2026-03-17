// =============================================================================
// D3D12Device.cpp - D3D12 设备实现
// =============================================================================

#include "D3D12Device.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12SwapChain.h"
#include <dxgi1_6.h>
#include <sstream>
#include <iomanip>

// 调试层 GUID（需要在 Windows SDK 18082 以上版本）
#ifndef DXGI_CREATE_FACTORY_DEBUG
#define DXGI_CREATE_FACTORY_DEBUG 0x02
#endif

namespace Engine::RHI::D3D12 {

// =============================================================================
// 辅助函数
// =============================================================================

namespace {

/**
 * @brief 将 HRESULT 转换为 RHIResult
 */
RHIResult HRESULTToRHIResult(HRESULT hr) {
    if (SUCCEEDED(hr)) {
        return RHIResult::Success;
    }
    
    switch (hr) {
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
            return RHIResult::DeviceLost;
        case E_OUTOFMEMORY:
            return RHIResult::OutOfMemory;
        case E_INVALIDARG:
            return RHIResult::InvalidParameter;
        default:
            return RHIResult::InternalError;
    }
}

/**
 * @brief 获取 HRESULT 错误描述
 */
std::string GetHRESULTErrorString(HRESULT hr) {
    switch (hr) {
        case DXGI_ERROR_DEVICE_REMOVED:
            return "DXGI_ERROR_DEVICE_REMOVED";
        case DXGI_ERROR_DEVICE_RESET:
            return "DXGI_ERROR_DEVICE_RESET";
        case DXGI_ERROR_DEVICE_HUNG:
            return "DXGI_ERROR_DEVICE_HUNG";
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
            return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
        case E_OUTOFMEMORY:
            return "E_OUTOFMEMORY";
        case E_INVALIDARG:
            return "E_INVALIDARG";
        default:
            return "Unknown HRESULT: " + std::to_string(hr);
    }
}

/**
 * @brief 宽字符转 UTF-8
 */
std::string WideStringToUTF8(const wchar_t* wideStr) {
    if (!wideStr) return "";
    
    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    
    if (sizeNeeded <= 0) return "";
    
    std::string result(sizeNeeded - 1, '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, wideStr, -1, result.data(), sizeNeeded, nullptr, nullptr);
    
    return result;
}

} // anonymous namespace

// =============================================================================
// 构造/析构
// =============================================================================

D3D12Device::D3D12Device() {
    RHI_LOG_DEBUG("D3D12Device: 构造函数");
}

D3D12Device::~D3D12Device() {
    RHI_LOG_DEBUG("D3D12Device: 析构函数");
    Shutdown();
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12Device::Initialize(const DeviceDesc& desc) {
    RHI_LOG_INFO("D3D12Device: 开始初始化...");
    
    // 存储配置
    m_enableDebug = desc.enableDebug;
    m_enableValidation = desc.enableValidation;
    
    HRESULT hr = S_OK;
    
    // ========== 1. 启用调试层 ==========
    
    if (m_enableDebug) {
        ComPtr<ID3D12Debug> debugController;
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
        
        if (SUCCEEDED(hr)) {
            debugController->EnableDebugLayer();
            RHI_LOG_INFO("D3D12Device: D3D12 调试层已启用");
        } else {
            RHI_LOG_WARNING("D3D12Device: 无法启用 D3D12 调试层 (HRESULT: 0x%08X)", hr);
        }
        
        // 尝试启用 GPU 验证（仅当同时启用验证时）
        if (m_enableValidation) {
            ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
                debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                RHI_LOG_INFO("D3D12Device: 同步命令队列验证已启用");
            }
        }
    }
    
    // ========== 2. 创建 DXGI 工厂 ==========
    
    UINT dxgiFactoryFlags = 0;
    if (m_enableDebug) {
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
    
    hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr)) {
        m_lastError = "CreateDXGIFactory2 失败: " + GetHRESULTErrorString(hr);
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return HRESULTToRHIResult(hr);
    }
    
    RHI_LOG_INFO("D3D12Device: DXGI 工厂创建成功");
    
    // ========== 3. 枚举适配器并选择最佳 GPU ==========
    
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIAdapter1> selectedAdapter;
    SIZE_T maxDedicatedVideoMemory = 0;
    
    for (UINT adapterIndex = 0; 
         m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
         ++adapterIndex) {
        
        DXGI_ADAPTER_DESC1 adapterDesc;
        hr = adapter->GetDesc1(&adapterDesc);
        
        if (FAILED(hr)) {
            RHI_LOG_WARNING("D3D12Device: 无法获取适配器 %u 描述", adapterIndex);
            continue;
        }
        
        // 跳过软件适配器（除非是唯一选择）
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            RHI_LOG_DEBUG("D3D12Device: 跳过软件适配器 %u", adapterIndex);
            continue;
        }
        
        // 检查 D3D12 支持
        hr = D3D12CreateDevice(
            adapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            _uuidof(ID3D12Device),
            nullptr);
        
        if (FAILED(hr)) {
            RHI_LOG_DEBUG("D3D12Device: 适配器 %u 不支持 D3D12", adapterIndex);
            continue;
        }
        
        // 选择显存最大的 GPU
        if (adapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory) {
            maxDedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
            selectedAdapter = adapter;
        }
    }
    
    // 如果没有找到硬件适配器，尝试 WARP（软件渲染器）
    if (!selectedAdapter && m_dxgiFactory) {
        RHI_LOG_WARNING("D3D12Device: 未找到硬件适配器，尝试 WARP");
        m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&selectedAdapter));
    }
    
    if (!selectedAdapter) {
        m_lastError = "未找到支持 D3D12 的适配器";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return RHIResult::NotSupported;
    }
    
    // 保存适配器信息
    DXGI_ADAPTER_DESC1 selectedDesc;
    selectedAdapter->GetDesc1(&selectedDesc);
    
    m_deviceName = WideStringToUTF8(selectedDesc.Description);
    
    RHI_LOG_INFO("D3D12Device: 选择适配器: %s", m_deviceName.c_str());
    
    // ========== 4. 创建 D3D12 设备 ==========
    
    hr = D3D12CreateDevice(
        selectedAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device));
    
    if (FAILED(hr)) {
        m_lastError = "D3D12CreateDevice 失败: " + GetHRESULTErrorString(hr);
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return HRESULTToRHIResult(hr);
    }
    
    RHI_LOG_INFO("D3D12Device: D3D12 设备创建成功");
    
    // ========== 5. 设置调试信息队列 ==========
    
    if (m_enableDebug) {
        hr = m_device->QueryInterface(IID_PPV_ARGS(&m_infoQueue));
        
        if (SUCCEEDED(hr)) {
            // 设置严重性级别
            m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            
            // 抑制某些调试消息
            D3D12_MESSAGE_ID denyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
            };
            
            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(denyIds);
            filter.DenyList.pIDList = denyIds;
            
            m_infoQueue->PushStorageFilter(&filter);
            
            RHI_LOG_INFO("D3D12Device: 调试信息队列配置完成");
        }
    }
    
    // ========== 6. 创建命令队列 ==========
    
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;
    
    hr = m_device->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(&m_commandQueue));
    
    if (FAILED(hr)) {
        m_lastError = "CreateCommandQueue 失败: " + GetHRESULTErrorString(hr);
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return HRESULTToRHIResult(hr);
    }
    
    // 设置命令队列名称（用于调试）
    m_commandQueue->SetName(L"D3D12Device::m_commandQueue");
    
    RHI_LOG_INFO("D3D12Device: 命令队列创建成功");
    
    // ========== 7. 查询并填充设备信息 ==========
    
    m_deviceInfo.driverName = m_deviceName;
    m_deviceInfo.dedicatedVideoMemory = selectedDesc.DedicatedVideoMemory;
    m_deviceInfo.sharedVideoMemory = selectedDesc.SharedSystemMemory;
    m_deviceInfo.isIntegratedGPU = 
        (selectedDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
    
    // 查询可选特性支持
    D3D12_FEATURE_DATA_RAYTRACING_TIER raytracingData = {};
    if (SUCCEEDED(m_device->CheckFeatureSupport(
        D3D12_FEATURE_RAYTRACING_TIER, &raytracingData, sizeof(raytracingData)))) {
        m_deviceInfo.supportsRayTracing = 
            raytracingData.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
    } else {
        m_deviceInfo.supportsRayTracing = false;
    }
    
    D3D12_FEATURE_DATA_MESH_SHADER meshShaderData = {};
    if (SUCCEEDED(m_device->CheckFeatureSupport(
        D3D12_FEATURE_MESH_SHADER, &meshShaderData, sizeof(meshShaderData)))) {
        m_deviceInfo.supportsMeshShaders = meshShaderData.MeshShaderTier > D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    } else {
        m_deviceInfo.supportsMeshShaders = false;
    }
    
    // 尝试获取驱动版本
    ComPtr<IDXGIAdapter3> adapter3;
    if (SUCCEEDED(selectedAdapter->QueryInterface(IID_PPV_ARGS(&adapter3)))) {
        DXGI_QUERY_VIDEO_MEMORY_INFO localMemoryInfo;
        if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(
            0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localMemoryInfo))) {
            RHI_LOG_DEBUG("D3D12Device: 当前显存使用: %llu MB",
                localMemoryInfo.CurrentUsage / (1024 * 1024));
        }
    }
    
    // 格式化驱动版本
    std::ostringstream oss;
    oss << std::hex << selectedDesc.Revision << "." << selectedDesc.BuildNum;
    m_deviceInfo.driverVersion = oss.str();
    
    RHI_LOG_INFO("D3D12Device: 设备初始化完成");
    RHI_LOG_INFO("  - 设备名称: %s", m_deviceName.c_str());
    RHI_LOG_INFO("  - 专用显存: %llu MB", m_deviceInfo.dedicatedVideoMemory / (1024 * 1024));
    RHI_LOG_INFO("  - 共享显存: %llu MB", m_deviceInfo.sharedVideoMemory / (1024 * 1024));
    RHI_LOG_INFO("  - 集成 GPU: %s", m_deviceInfo.isIntegratedGPU ? "是" : "否");
    RHI_LOG_INFO("  - 光线追踪: %s", m_deviceInfo.supportsRayTracing ? "支持" : "不支持");
    RHI_LOG_INFO("  - 网格着色器: %s", m_deviceInfo.supportsMeshShaders ? "支持" : "不支持");
    
    return RHIResult::Success;
}

// =============================================================================
// 关闭
// =============================================================================

void D3D12Device::Shutdown() {
    RHI_LOG_INFO("D3D12Device: 开始关闭...");
    
    // 清理管理器（按照逆初始化顺序）
    m_uploadManager.reset();
    m_frameResourceManager.reset();
    
    // 清理命令队列
    m_commandQueue.Reset();
    
    // 清理调试信息队列
    if (m_infoQueue) {
        // 清除所有存储过滤器
        while (m_infoQueue->GetStorageFilterStackSize() > 0) {
            m_infoQueue->PopStorageFilter();
        }
        m_infoQueue.Reset();
    }
    
    // 清理设备
    m_device.Reset();
    
    // 清理 DXGI 工厂
    m_dxgiFactory.Reset();
    
    // 重置状态
    m_isDeviceLost = false;
    m_deviceLostReason = DeviceLostReason::Unknown;
    m_lastError.clear();
    m_deviceName.clear();
    
    RHI_LOG_INFO("D3D12Device: 关闭完成");
}

// =============================================================================
// IDevice 接口实现
// =============================================================================

std::string_view D3D12Device::GetDeviceName() const {
    return m_deviceName;
}

DeviceInfo D3D12Device::GetDeviceInfo() const {
    return m_deviceInfo;
}

// =============================================================================
// 资源创建（占位实现）
// =============================================================================

std::unique_ptr<IBuffer> D3D12Device::CreateBuffer(const BufferDesc& desc) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // 创建 D3D12Buffer
    auto buffer = std::make_unique<D3D12Buffer>();
    RHIResult result = buffer->Initialize(this, desc);
    
    if (IsFailure(result)) {
        m_lastError = "缓冲区创建失败: ";
        m_lastError += GetErrorName(result);
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    RHI_LOG_INFO("D3D12Device: 缓冲区创建成功 (大小=%llu)", desc.size);
    return buffer;
}

std::unique_ptr<ITexture> D3D12Device::CreateTexture(const TextureDesc& desc) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // 创建 D3D12Texture
    auto texture = std::make_unique<D3D12Texture>();
    RHIResult result = texture->Initialize(this, desc);
    
    if (IsFailure(result)) {
        m_lastError = "纹理创建失败: ";
        m_lastError += GetErrorName(result);
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    RHI_LOG_INFO("D3D12Device: 纹理创建成功 (%ux%u, 格式=%d)", 
        desc.width, desc.height, static_cast<int>(desc.format));
    return texture;
}

std::unique_ptr<IShader> D3D12Device::CreateShader(const ShaderDesc& desc) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // TODO: Task 004 - 实现 D3D12Shader
    m_lastError = "CreateShader 尚未实现";
    RHI_LOG_WARNING("D3D12Device: %s", m_lastError.c_str());
    return nullptr;
}

std::unique_ptr<IRootSignature> D3D12Device::CreateRootSignature(
    const RootSignatureDesc& desc) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // TODO: Task 005 - 实现 D3D12RootSignature
    m_lastError = "CreateRootSignature 尚未实现";
    RHI_LOG_WARNING("D3D12Device: %s", m_lastError.c_str());
    return nullptr;
}

std::unique_ptr<IPipelineState> D3D12Device::CreateGraphicsPipeline(
    const GraphicsPipelineDesc& desc) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // TODO: Task 006 - 实现 D3D12PipelineState
    m_lastError = "CreateGraphicsPipeline 尚未实现";
    RHI_LOG_WARNING("D3D12Device: %s", m_lastError.c_str());
    return nullptr;
}

std::unique_ptr<IPipelineState> D3D12Device::CreateComputePipeline(
    const ComputePipelineDesc& desc) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // TODO: Task 006 - 实现 D3D12PipelineState
    m_lastError = "CreateComputePipeline 尚未实现";
    RHI_LOG_WARNING("D3D12Device: %s", m_lastError.c_str());
    return nullptr;
}

std::unique_ptr<ISwapChain> D3D12Device::CreateSwapChain(
    void* windowHandle,
    uint32_t width,
    uint32_t height,
    Format format) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    if (!windowHandle) {
        m_lastError = "窗口句柄为空";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // 创建交换链描述
    SwapChainDesc desc = {};
    desc.windowHandle = windowHandle;
    desc.width = width;
    desc.height = height;
    desc.format = format;
    desc.bufferCount = 2;  // 双缓冲
    desc.vsync = true;
    desc.debugName = "MainSwapChain";
    
    // 创建 D3D12SwapChain
    auto swapChain = std::make_unique<D3D12SwapChain>();
    RHIResult result = swapChain->Initialize(this, desc);
    
    if (IsFailure(result)) {
        m_lastError = "交换链创建失败: ";
        m_lastError += GetErrorName(result);
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    RHI_LOG_INFO("D3D12Device: 交换链创建成功 (%ux%u, 格式=%d)", 
        width, height, static_cast<int>(format));
    return swapChain;
}

std::unique_ptr<IDescriptorHeap> D3D12Device::CreateDescriptorHeap(
    const DescriptorHeapDesc& desc) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // TODO: Task 008 - 实现 D3D12DescriptorHeap
    m_lastError = "CreateDescriptorHeap 尚未实现";
    RHI_LOG_WARNING("D3D12Device: %s", m_lastError.c_str());
    return nullptr;
}

uint32_t D3D12Device::GetDescriptorHandleIncrementSize(DescriptorHeapType type) const {
    if (!m_device) {
        RHI_LOG_ERROR("D3D12Device: 设备未初始化");
        return 0;
    }
    
    D3D12_DESCRIPTOR_HEAP_TYPE d3dType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    switch (type) {
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
            RHI_LOG_WARNING("D3D12Device: 未知的描述符堆类型");
            break;
    }
    
    return m_device->GetDescriptorHandleIncrementSize(d3dType);
}

std::unique_ptr<IFence> D3D12Device::CreateFence(uint64_t initialValue) {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // TODO: Task 009 - 实现 D3D12Fence
    m_lastError = "CreateFence 尚未实现";
    RHI_LOG_WARNING("D3D12Device: %s", m_lastError.c_str());
    return nullptr;
}

void D3D12Device::WaitForFence(IFence* fence, uint64_t value) {
    if (!fence) {
        RHI_LOG_ERROR("D3D12Device: 围栏指针为空");
        return;
    }
    
    // TODO: Task 009 - 实现 D3D12Fence
    RHI_LOG_WARNING("D3D12Device: WaitForFence 尚未实现");
}

void D3D12Device::SignalFence(IFence* fence, uint64_t value) {
    if (!fence) {
        RHI_LOG_ERROR("D3D12Device: 围栏指针为空");
        return;
    }
    
    // TODO: Task 009 - 实现 D3D12Fence
    RHI_LOG_WARNING("D3D12Device: SignalFence 尚未实现");
}

std::unique_ptr<ICommandList> D3D12Device::CreateCommandList() {
    if (!m_device) {
        m_lastError = "设备未初始化";
        RHI_LOG_ERROR("D3D12Device: %s", m_lastError.c_str());
        return nullptr;
    }
    
    // TODO: Task 010 - 实现 D3D12CommandList
    m_lastError = "CreateCommandList 尚未实现";
    RHI_LOG_WARNING("D3D12Device: %s", m_lastError.c_str());
    return nullptr;
}

void D3D12Device::SubmitCommandLists(std::span<ICommandList* const> commandLists) {
    if (commandLists.empty()) {
        return;
    }
    
    if (!m_commandQueue) {
        RHI_LOG_ERROR("D3D12Device: 命令队列未初始化");
        return;
    }
    
    // TODO: Task 010 - 实现 D3D12CommandList 提交
    RHI_LOG_WARNING("D3D12Device: SubmitCommandLists 尚未实现");
}

// =============================================================================
// 错误处理
// =============================================================================

void D3D12Device::SetDeviceLostCallback(DeviceLostCallback callback) {
    m_deviceLostCallback = callback;
}

RHIResult D3D12Device::TryRecover() {
    if (!m_isDeviceLost) {
        return RHIResult::Success;
    }
    
    RHI_LOG_WARNING("D3D12Device: 尝试恢复设备...");
    
    // 检查设备状态
    if (m_device) {
        HRESULT hr = m_device->GetDeviceRemovedReason();
        if (hr == S_OK) {
            // 设备已恢复正常
            m_isDeviceLost = false;
            m_deviceLostReason = DeviceLostReason::Unknown;
            m_lastError.clear();
            RHI_LOG_INFO("D3D12Device: 设备已恢复");
            return RHIResult::Success;
        }
    }
    
    // 需要完全重建设备
    RHI_LOG_ERROR("D3D12Device: 设备无法恢复，需要重建");
    return RHIResult::DeviceLost;
}

// =============================================================================
// 设备丢失处理
// =============================================================================

void D3D12Device::OnDeviceLost(DeviceLostReason reason, const char* message) {
    m_isDeviceLost = true;
    m_deviceLostReason = reason;
    m_lastError = message ? message : "设备丢失";
    
    RHI_LOG_ERROR("D3D12Device: 设备丢失 - %s", m_lastError.c_str());
    
    // 调试模式下触发断点
    if (m_enableDebug) {
        RHI_DEBUG_BREAK();
    }
    
    // 调用用户回调
    if (m_deviceLostCallback) {
        m_deviceLostCallback(reason, m_lastError.c_str());
    }
}

void D3D12Device::CheckDeviceStatus() {
    if (!m_device || m_isDeviceLost) {
        return;
    }
    
    HRESULT hr = m_device->GetDeviceRemovedReason();
    if (hr != S_OK) {
        DeviceLostReason reason = DeviceLostReason::Unknown;
        
        switch (hr) {
            case DXGI_ERROR_DEVICE_REMOVED:
                reason = DeviceLostReason::DeviceRemoved;
                break;
            case DXGI_ERROR_DEVICE_RESET:
                reason = DeviceLostReason::DriverCrash;
                break;
            case DXGI_ERROR_DEVICE_HUNG:
                reason = DeviceLostReason::GPUHang;
                break;
            case E_OUTOFMEMORY:
                reason = DeviceLostReason::OutOfVideoMemory;
                break;
            default:
                reason = DeviceLostReason::Unknown;
                break;
        }
        
        OnDeviceLost(reason, GetHRESULTErrorString(hr).c_str());
    }
}

} // namespace Engine::RHI::D3D12