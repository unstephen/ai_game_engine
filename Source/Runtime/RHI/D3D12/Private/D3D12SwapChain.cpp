// =============================================================================
// D3D12SwapChain.cpp - D3D12 交换链实现
// =============================================================================

#include "D3D12SwapChain.h"

#if ENGINE_RHI_D3D12

#include "D3D12Device.h"
#include "D3D12Texture.h"

// Windows/DX12 头文件
#include <dxgi1_6.h>

namespace Engine::RHI::D3D12
{

// =============================================================================
// 构造/析构
// =============================================================================

D3D12SwapChain::D3D12SwapChain()
    : m_width(0),
      m_height(0),
      m_bufferCount(2),
      m_format(Format::R8G8B8A8_UNorm),
      m_vsync(true),
      m_allowTearing(false),
      m_currentFrameIndex(0),
      m_device(nullptr),
      m_windowHandle(nullptr),
      m_rtvDescriptorSize(0)
{
}

D3D12SwapChain::~D3D12SwapChain()
{
    Shutdown();
}

// =============================================================================
// 移动语义
// =============================================================================

D3D12SwapChain::D3D12SwapChain(D3D12SwapChain&& other) noexcept
    : m_swapChain(std::move(other.m_swapChain)),
      m_backBuffers(std::move(other.m_backBuffers)),
      m_backBufferTextures(std::move(other.m_backBufferTextures)),
      m_rtvHeap(std::move(other.m_rtvHeap)),
      m_rtvHandles(std::move(other.m_rtvHandles)),
      m_width(other.m_width),
      m_height(other.m_height),
      m_bufferCount(other.m_bufferCount),
      m_format(other.m_format),
      m_vsync(other.m_vsync),
      m_allowTearing(other.m_allowTearing),
      m_currentFrameIndex(other.m_currentFrameIndex),
      m_device(other.m_device),
      m_windowHandle(other.m_windowHandle),
      m_debugName(std::move(other.m_debugName)),
      m_rtvDescriptorSize(other.m_rtvDescriptorSize)
{
    // 重置源对象
    other.m_width             = 0;
    other.m_height            = 0;
    other.m_currentFrameIndex = 0;
    other.m_device            = nullptr;
    other.m_windowHandle      = nullptr;
}

D3D12SwapChain& D3D12SwapChain::operator=(D3D12SwapChain&& other) noexcept
{
    if (this != &other)
    {
        // 释放当前资源
        Shutdown();

        // 移动资源
        m_swapChain          = std::move(other.m_swapChain);
        m_backBuffers        = std::move(other.m_backBuffers);
        m_backBufferTextures = std::move(other.m_backBufferTextures);
        m_rtvHeap            = std::move(other.m_rtvHeap);
        m_rtvHandles         = std::move(other.m_rtvHandles);
        m_width              = other.m_width;
        m_height             = other.m_height;
        m_bufferCount        = other.m_bufferCount;
        m_format             = other.m_format;
        m_vsync              = other.m_vsync;
        m_allowTearing       = other.m_allowTearing;
        m_currentFrameIndex  = other.m_currentFrameIndex;
        m_device             = other.m_device;
        m_windowHandle       = other.m_windowHandle;
        m_debugName          = std::move(other.m_debugName);
        m_rtvDescriptorSize  = other.m_rtvDescriptorSize;

        // 重置源对象
        other.m_width             = 0;
        other.m_height            = 0;
        other.m_currentFrameIndex = 0;
        other.m_device            = nullptr;
        other.m_windowHandle      = nullptr;
    }
    return *this;
}

// =============================================================================
// 初始化
// =============================================================================

RHIResult D3D12SwapChain::Initialize(D3D12Device* device, const SwapChainDesc& desc)
{
    // 参数验证
    if (!device)
    {
        RHI_LOG_ERROR("D3D12SwapChain::Initialize: 设备指针为空");
        return RHIResult::InvalidParameter;
    }

    if (!desc.windowHandle)
    {
        RHI_LOG_ERROR("D3D12SwapChain::Initialize: 窗口句柄为空");
        return RHIResult::InvalidParameter;
    }

    if (desc.width == 0 || desc.height == 0)
    {
        RHI_LOG_ERROR("D3D12SwapChain::Initialize: 无效的尺寸 (%ux%u)", desc.width, desc.height);
        return RHIResult::InvalidParameter;
    }

    if (desc.bufferCount < 2 || desc.bufferCount > 4)
    {
        RHI_LOG_ERROR("D3D12SwapChain::Initialize: 无效的缓冲区数量 %u (应为 2-4)", desc.bufferCount);
        return RHIResult::InvalidParameter;
    }

    // 保存参数
    m_device       = device;
    m_windowHandle = desc.windowHandle;
    m_width        = desc.width;
    m_height       = desc.height;
    m_format       = desc.format;
    m_bufferCount  = desc.bufferCount;
    m_vsync        = desc.vsync;
    m_debugName    = desc.debugName ? desc.debugName : "SwapChain";

    // 创建交换链
    RHIResult result = CreateSwapChain(device, desc.windowHandle);
    if (IsFailure(result))
    {
        RHI_LOG_ERROR("D3D12SwapChain::Initialize: 创建交换链失败");
        return result;
    }

    // 创建渲染目标视图
    result = CreateRenderTargetViews(device);
    if (IsFailure(result))
    {
        RHI_LOG_ERROR("D3D12SwapChain::Initialize: 创建 RTV 失败");
        return result;
    }

    // 获取当前帧索引
    m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

    RHI_LOG_INFO("D3D12SwapChain 初始化成功: %ux%u, %u 缓冲区, 格式 %d", m_width, m_height, m_bufferCount,
                 static_cast<int>(m_format));

    return RHIResult::Success;
}

void D3D12SwapChain::Shutdown()
{
    // 释放后备缓冲区纹理
    m_backBufferTextures.clear();

    // 释放后备缓冲区资源
    ReleaseBackBuffers();

    // 释放 RTV 描述符堆
    m_rtvHeap.Reset();
    m_rtvHandles.clear();

    // 释放交换链
    m_swapChain.Reset();

    // 重置状态
    m_width             = 0;
    m_height            = 0;
    m_currentFrameIndex = 0;
    m_device            = nullptr;
    m_windowHandle      = nullptr;

    RHI_LOG_DEBUG("D3D12SwapChain 已关闭");
}

// =============================================================================
// ISwapChain 接口实现
// =============================================================================

uint32_t D3D12SwapChain::GetCurrentBackBufferIndex() const
{
    if (!m_swapChain)
    {
        return 0;
    }
    return m_swapChain->GetCurrentBackBufferIndex();
}

void D3D12SwapChain::Resize(uint32_t width, uint32_t height)
{
    if (!m_swapChain || !m_device)
    {
        RHI_LOG_WARNING("D3D12SwapChain::Resize: 交换链未初始化");
        return;
    }

    if (width == 0 || height == 0)
    {
        RHI_LOG_ERROR("D3D12SwapChain::Resize: 无效的尺寸 (%ux%u)", width, height);
        return;
    }

    // 如果尺寸未变化，跳过
    if (width == m_width && height == m_height)
    {
        RHI_LOG_DEBUG("D3D12SwapChain::Resize: 尺寸未变化，跳过");
        return;
    }

    RHI_LOG_INFO("D3D12SwapChain::Resize: %ux%u -> %ux%u", m_width, m_height, width, height);

    // 释放后备缓冲区资源
    ReleaseBackBuffers();
    m_rtvHeap.Reset();
    m_rtvHandles.clear();
    m_backBufferTextures.clear();

    // 更新尺寸
    m_width  = width;
    m_height = height;

    // 调整缓冲区大小
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    HRESULT               hr   = m_swapChain->GetDesc1(&desc);
    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12SwapChain::Resize: 获取交换链描述失败: 0x%08X", hr);
        return;
    }

    // 计算 SwapChain 标志
    UINT swapChainFlags = 0;
    if (m_allowTearing)
    {
        swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    hr = m_swapChain->ResizeBuffers(m_bufferCount, width, height, desc.Format, swapChainFlags);

    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12SwapChain::Resize: ResizeBuffers 失败: 0x%08X", hr);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            m_device->SetDeviceLostCallback(nullptr); // 清除回调
            // 设备丢失需要重建
            RHI_LOG_ERROR("D3D12SwapChain::Resize: 设备丢失");
        }
        return;
    }

    // 重新创建 RTV
    RHIResult result = CreateRenderTargetViews(m_device);
    if (IsFailure(result))
    {
        RHI_LOG_ERROR("D3D12SwapChain::Resize: 创建 RTV 失败");
        return;
    }

    // 更新帧索引
    m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

    RHI_LOG_INFO("D3D12SwapChain 调整大小成功: %ux%u", m_width, m_height);
}

void D3D12SwapChain::Present()
{
    if (!m_swapChain)
    {
        RHI_LOG_ERROR("D3D12SwapChain::Present: 交换链未初始化");
        return;
    }

    // 计算 SyncInterval
    // VSync 启用时: SyncInterval = 1 (每帧同步一次)
    // VSync 禁用时: SyncInterval = 0 (不同步，可能产生撕裂)
    UINT syncInterval = m_vsync ? 1 : 0;

    // 计算 Present 标志
    UINT presentFlags = 0;
    if (!m_vsync && m_allowTearing)
    {
        presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    // 呈现
    HRESULT hr = m_swapChain->Present(syncInterval, presentFlags);

    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            RHI_LOG_ERROR("D3D12SwapChain::Present: 设备丢失 (DEVICE_REMOVED)");
            // 设备丢失处理
            if (m_device)
            {
                // 通知设备丢失
            }
        }
        else if (hr == DXGI_ERROR_DEVICE_RESET)
        {
            RHI_LOG_ERROR("D3D12SwapChain::Present: 设备重置 (DEVICE_RESET)");
        }
        else if (hr == DXGI_ERROR_INVALID_CALL)
        {
            RHI_LOG_ERROR("D3D12SwapChain::Present: 无效调用");
        }
        else
        {
            RHI_LOG_ERROR("D3D12SwapChain::Present: 呈现失败: 0x%08X", hr);
        }
        return;
    }

    // 更新当前帧索引
    m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

ITexture* D3D12SwapChain::GetBackBuffer(uint32_t index)
{
    if (index >= m_bufferCount)
    {
        RHI_LOG_ERROR("D3D12SwapChain::GetBackBuffer: 索引越界 %u (缓冲区数量: %u)", index, m_bufferCount);
        return nullptr;
    }

    // 如果纹理包装对象不存在，创建它
    if (m_backBufferTextures.size() != m_bufferCount)
    {
        m_backBufferTextures.resize(m_bufferCount);
    }

    if (!m_backBufferTextures[index] && m_backBuffers[index])
    {
        // 创建纹理包装对象
        m_backBufferTextures[index] = std::make_unique<D3D12Texture>();

        // 注意：这里我们创建的是一个包装对象，不拥有资源
        // D3D12Texture 需要支持包装外部资源的功能
        // 暂时返回 nullptr，等待 D3D12Texture 实现包装功能
        // TODO: 实现 D3D12Texture 的资源包装功能
    }

    return m_backBufferTextures[index].get();
}

// =============================================================================
// 扩展接口
// =============================================================================

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetCurrentRTV() const
{
    return GetRTV(GetCurrentBackBufferIndex());
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetRTV(uint32_t index) const
{
    if (index >= m_rtvHandles.size())
    {
        RHI_LOG_ERROR("D3D12SwapChain::GetRTV: 索引越界 %u", index);
        return {0};
    }
    return m_rtvHandles[index];
}

ID3D12Resource* D3D12SwapChain::GetCurrentBackBufferResource() const
{
    return GetBackBufferResource(GetCurrentBackBufferIndex());
}

ID3D12Resource* D3D12SwapChain::GetBackBufferResource(uint32_t index) const
{
    if (index >= m_backBuffers.size())
    {
        RHI_LOG_ERROR("D3D12SwapChain::GetBackBufferResource: 索引越界 %u", index);
        return nullptr;
    }
    return m_backBuffers[index].Get();
}

// =============================================================================
// 内部方法
// =============================================================================

RHIResult D3D12SwapChain::CreateSwapChain(D3D12Device* device, void* windowHandle)
{
    // 获取 DXGI 工厂
    IDXGIFactory4* factory = device->GetDXGIFactory();
    if (!factory)
    {
        RHI_LOG_ERROR("D3D12SwapChain::CreateSwapChain: DXGI 工厂为空");
        return RHIResult::InternalError;
    }

    // 检查撕裂支持
    m_allowTearing = CheckTearingSupport(factory);

    // 设置交换链描述
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width                 = m_width;
    swapChainDesc.Height                = m_height;
    swapChainDesc.Format                = ToDXGIFormat(m_format);
    swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount           = m_bufferCount;
    swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD; // FLIP_DISCARD 模式
    swapChainDesc.SampleDesc.Count      = 1;                             // 不使用 MSAA
    swapChainDesc.SampleDesc.Quality    = 0;
    swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Scaling               = DXGI_SCALING_STRETCH;

    // 如果支持撕裂，添加标志
    if (m_allowTearing)
    {
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    // 创建交换链
    ComPtr<IDXGISwapChain1> tempSwapChain;
    HRESULT                 hr = factory->CreateSwapChainForHwnd(device->GetCommandQueue(),       // 使用命令队列
                                                                 static_cast<HWND>(windowHandle), // 窗口句柄
                                                                 &swapChainDesc,                  // 交换链描述
                                                                 nullptr,                         // 全屏描述
                                                                 nullptr,                         // 限制输出
                                                                 &tempSwapChain                   // 输出交换链
                    );

    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12SwapChain::CreateSwapChain: 创建交换链失败: 0x%08X", hr);
        return RHIResult::SwapChainError;
    }

    // 转换为 IDXGISwapChain3（支持 GetCurrentBackBufferIndex）
    hr = tempSwapChain.As(&m_swapChain);
    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12SwapChain::CreateSwapChain: 获取 SwapChain3 失败: 0x%08X", hr);
        return RHIResult::SwapChainError;
    }

    // 禁用 Alt+Enter 全屏切换
    // 这由应用程序自行处理
    hr = factory->MakeWindowAssociation(static_cast<HWND>(windowHandle), DXGI_MWA_NO_ALT_ENTER);

    if (FAILED(hr))
    {
        RHI_LOG_WARNING("D3D12SwapChain::CreateSwapChain: 禁用 Alt+Enter 失败: 0x%08X", hr);
        // 这不是致命错误
    }

    RHI_LOG_DEBUG("D3D12SwapChain: DXGI 交换链创建成功");
    return RHIResult::Success;
}

RHIResult D3D12SwapChain::CreateRenderTargetViews(D3D12Device* device)
{
    if (!m_swapChain)
    {
        RHI_LOG_ERROR("D3D12SwapChain::CreateRenderTargetViews: 交换链为空");
        return RHIResult::InvalidParameter;
    }

    ID3D12Device* d3dDevice = device->GetD3D12Device();
    if (!d3dDevice)
    {
        RHI_LOG_ERROR("D3D12SwapChain::CreateRenderTargetViews: D3D12 设备为空");
        return RHIResult::InvalidParameter;
    }

    // 创建 RTV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors             = m_bufferCount;
    rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask                   = 0;

    HRESULT hr = d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));

    if (FAILED(hr))
    {
        RHI_LOG_ERROR("D3D12SwapChain::CreateRenderTargetViews: 创建 RTV 描述符堆失败: 0x%08X", hr);
        return RHIResult::ResourceCreationFailed;
    }

    // 设置调试名称
    if (!m_debugName.empty())
    {
        std::wstring wname(m_debugName.begin(), m_debugName.end());
        m_rtvHeap->SetName((wname + L"_RTVHeap").c_str());
    }

    // 获取描述符增量大小
    m_rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 初始化后备缓冲区容器
    m_backBuffers.resize(m_bufferCount);
    m_rtvHandles.resize(m_bufferCount);

    // 获取堆起始句柄
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    // 为每个后备缓冲区创建 RTV
    for (UINT i = 0; i < m_bufferCount; i++)
    {
        // 获取后备缓冲区
        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        if (FAILED(hr))
        {
            RHI_LOG_ERROR("D3D12SwapChain::CreateRenderTargetViews: 获取后备缓冲区 %u 失败: 0x%08X", i, hr);
            return RHIResult::ResourceCreationFailed;
        }

        // 设置调试名称
        if (!m_debugName.empty())
        {
            wchar_t name[64];
            swprintf_s(name, L"%s_BackBuffer%u", std::wstring(m_debugName.begin(), m_debugName.end()).c_str(), i);
            m_backBuffers[i]->SetName(name);
        }

        // 创建 RTV
        d3dDevice->CreateRenderTargetView(m_backBuffers[i].Get(),
                                          nullptr, // 默认 RTV 描述
                                          rtvHandle);

        // 保存句柄
        m_rtvHandles[i] = rtvHandle;

        // 移动到下一个描述符
        rtvHandle.ptr += m_rtvDescriptorSize;

        RHI_LOG_DEBUG("D3D12SwapChain: 后备缓冲区 %u RTV 创建成功", i);
    }

    RHI_LOG_DEBUG("D3D12SwapChain: RTV 创建完成，共 %u 个", m_bufferCount);
    return RHIResult::Success;
}

void D3D12SwapChain::ReleaseBackBuffers()
{
    for (auto& buffer : m_backBuffers)
    {
        buffer.Reset();
    }
    m_backBuffers.clear();
    m_rtvHandles.clear();
}

// =============================================================================
// 静态方法
// =============================================================================

DXGI_FORMAT D3D12SwapChain::ToDXGIFormat(Format format)
{
    switch (format)
    {
        case Format::R8G8B8A8_UNorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_UNorm_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::R16G16B16A16_Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::R32G32B32A32_Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R10G10B10A2_UNorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        default:
            RHI_LOG_WARNING("D3D12SwapChain::ToDXGIFormat: 未知的格式 %d, 使用默认 R8G8B8A8_UNORM",
                            static_cast<int>(format));
            return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

bool D3D12SwapChain::CheckTearingSupport(IDXGIFactory4* factory)
{
    if (!factory)
    {
        return false;
    }

    // 尝试获取 IDXGIFactory5
    ComPtr<IDXGIFactory5> factory5;
    HRESULT               hr = factory->QueryInterface(IID_PPV_ARGS(&factory5));

    if (FAILED(hr))
    {
        RHI_LOG_DEBUG("D3D12SwapChain: 不支持 IDXGIFactory5，禁用撕裂");
        return false;
    }

    // 检查撕裂支持
    BOOL allowTearing = FALSE;
    hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

    if (FAILED(hr))
    {
        RHI_LOG_DEBUG("D3D12SwapChain: 检查撕裂支持失败: 0x%08X", hr);
        return false;
    }

    RHI_LOG_DEBUG("D3D12SwapChain: 撕裂支持: %s", allowTearing ? "是" : "否");
    return allowTearing == TRUE;
}

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12