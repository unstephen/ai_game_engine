// =============================================================================
// D3D12Device.h - D3D12 设备接口
// =============================================================================

#pragma once

#include "RHI.h"

#if ENGINE_RHI_D3D12

// Windows/DX12 头文件
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

namespace Engine::RHI::D3D12 {

using Microsoft::WRL::ComPtr;

// =============================================================================
// D3D12Device - D3D12 后端设备实现
// =============================================================================

class D3D12Device : public IDevice {
public:
    D3D12Device();
    virtual ~D3D12Device() override;
    
    // ========== 初始化 ==========
    
    /// 初始化设备
    RHIResult Initialize(const DeviceDesc& desc);
    
    /// 关闭设备
    void Shutdown();
    
    // ========== IDevice 接口实现 ==========
    
    virtual Backend GetBackend() const override { return Backend::D3D12; }
    virtual std::string_view GetDeviceName() const override;
    virtual DeviceInfo GetDeviceInfo() const override;
    
    // 资源创建
    virtual std::unique_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) override;
    virtual std::unique_ptr<ITexture> CreateTexture(const TextureDesc& desc) override;
    virtual std::unique_ptr<IShader> CreateShader(const ShaderDesc& desc) override;
    
    // 管线创建
    virtual std::unique_ptr<IRootSignature> CreateRootSignature(
        const RootSignatureDesc& desc) override;
    virtual std::unique_ptr<IPipelineState> CreateGraphicsPipeline(
        const GraphicsPipelineDesc& desc) override;
    virtual std::unique_ptr<IPipelineState> CreateComputePipeline(
        const ComputePipelineDesc& desc) override;
    
    // 交换链
    virtual std::unique_ptr<ISwapChain> CreateSwapChain(
        void* windowHandle,
        uint32_t width,
        uint32_t height,
        Format format = Format::R8G8B8A8_UNorm) override;
    
    // 描述符
    virtual std::unique_ptr<IDescriptorHeap> CreateDescriptorHeap(
        const DescriptorHeapDesc& desc) override;
    virtual uint32_t GetDescriptorHandleIncrementSize(DescriptorHeapType type) const override;
    
    // 同步
    virtual std::unique_ptr<IFence> CreateFence(uint64_t initialValue = 0) override;
    virtual void WaitForFence(IFence* fence, uint64_t value) override;
    virtual void SignalFence(IFence* fence, uint64_t value) override;
    
    // 命令列表
    virtual std::unique_ptr<ICommandList> CreateCommandList() override;
    virtual void SubmitCommandLists(std::span<ICommandList* const> commandLists) override;
    
    // 帧资源管理
    virtual IFrameResourceManager* GetFrameResourceManager() override { return m_frameResourceManager.get(); }
    virtual IUploadManager* GetUploadManager() override { return m_uploadManager.get(); }
    
    // 错误处理
    virtual void SetDeviceLostCallback(DeviceLostCallback callback) override;
    virtual bool IsDeviceLost() const override { return m_isDeviceLost; }
    virtual DeviceLostReason GetDeviceLostReason() const override { return m_deviceLostReason; }
    virtual RHIResult TryRecover() override;
    virtual const char* GetLastError() const override { return m_lastError.c_str(); }
    
    // ========== D3D12 特定接口 ==========
    
    /// 获取 D3D12 设备
    ID3D12Device* GetD3D12Device() const { return m_device.Get(); }
    
    /// 获取命令队列
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
    
    /// 获取 DXGI 工厂
    IDXGIFactory4* GetDXGIFactory() const { return m_dxgiFactory.Get(); }
    
private:
    // D3D12 核心对象
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12InfoQueue> m_infoQueue;  // 调试信息队列
    
    // 管理器
    std::unique_ptr<IFrameResourceManager> m_frameResourceManager;
    std::unique_ptr<IUploadManager> m_uploadManager;
    
    // 设备状态
    bool m_isDeviceLost = false;
    DeviceLostReason m_deviceLostReason = DeviceLostReason::Unknown;
    std::string m_lastError;
    std::string m_deviceName;
    DeviceInfo m_deviceInfo;
    
    // 回调
    DeviceLostCallback m_deviceLostCallback = nullptr;
    
    // 调试
    bool m_enableDebug = false;
    bool m_enableValidation = false;
};

} // namespace Engine::RHI::D3D12

#endif // ENGINE_RHI_D3D12
