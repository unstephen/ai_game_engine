// =============================================================================
// IDevice.h - 设备接口
// =============================================================================

#pragma once

#include "RHICore.h"

namespace Engine::RHI {

// 前向声明
class ICommandList;
class IBuffer;
class ITexture;
class IShader;
class IPipelineState;
class IRootSignature;
class ISwapChain;
class IFence;
class IDescriptorHeap;
class IFrameResourceManager;
class IUploadManager;

// =============================================================================
// 设备描述
// =============================================================================

struct DeviceDesc {
    Backend backend = Backend::D3D12;
    bool enableDebug = true;
    bool enableValidation = true;
    void* windowHandle = nullptr;  // 用于创建交换链
};

// =============================================================================
// 设备信息
// =============================================================================

struct DeviceInfo {
    std::string driverName;
    std::string driverVersion;
    uint64_t dedicatedVideoMemory;
    uint64_t sharedVideoMemory;
    bool isIntegratedGPU;
    bool supportsRayTracing;
    bool supportsMeshShaders;
};

// =============================================================================
// 设备丢失原因
// =============================================================================

enum class DeviceLostReason {
    Unknown,
    DriverCrash,
    GPUHang,
    DeviceRemoved,
    OutOfVideoMemory,
    InvalidCall,
};

// 设备丢失回调
using DeviceLostCallback = void(*)(DeviceLostReason reason, const char* message);

// =============================================================================
// IDevice - 设备接口
// =============================================================================

/**
 * @brief RHI 设备接口
 * 
 * 代表图形设备（GPU），是所有 RHI 资源的工厂
 */
class RHI_API IDevice {
public:
    virtual ~IDevice() = default;
    
    // ========== 基本信息 ==========
    
    /// 获取后端类型
    virtual Backend GetBackend() const = 0;
    
    /// 获取设备名称
    virtual std::string_view GetDeviceName() const = 0;
    
    /// 获取设备信息
    virtual DeviceInfo GetDeviceInfo() const = 0;
    
    // ========== 资源创建 ==========
    
    /// 创建缓冲区
    virtual std::unique_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;
    
    /// 创建纹理
    virtual std::unique_ptr<ITexture> CreateTexture(const TextureDesc& desc) = 0;
    
    /// 创建着色器
    virtual std::unique_ptr<IShader> CreateShader(const ShaderDesc& desc) = 0;
    
    // ========== 管线创建 ==========
    
    /// 创建根签名
    virtual std::unique_ptr<IRootSignature> CreateRootSignature(
        const RootSignatureDesc& desc) = 0;
    
    /// 创建图形管线
    virtual std::unique_ptr<IPipelineState> CreateGraphicsPipeline(
        const GraphicsPipelineDesc& desc) = 0;
    
    /// 创建计算管线
    virtual std::unique_ptr<IPipelineState> CreateComputePipeline(
        const ComputePipelineDesc& desc) = 0;
    
    // ========== 交换链 ==========
    
    /// 创建交换链
    virtual std::unique_ptr<ISwapChain> CreateSwapChain(
        void* windowHandle, 
        uint32_t width, 
        uint32_t height,
        Format format = Format::R8G8B8A8_UNorm) = 0;
    
    // ========== 描述符 ==========
    
    /// 创建描述符堆
    virtual std::unique_ptr<IDescriptorHeap> CreateDescriptorHeap(
        const DescriptorHeapDesc& desc) = 0;
    
    /// 获取描述符大小
    virtual uint32_t GetDescriptorHandleIncrementSize(DescriptorHeapType type) const = 0;
    
    // ========== 同步 ==========
    
    /// 创建围栏
    virtual std::unique_ptr<IFence> CreateFence(uint64_t initialValue = 0) = 0;
    
    /// 等待围栏
    virtual void WaitForFence(IFence* fence, uint64_t value) = 0;
    
    /// 信号围栏
    virtual void SignalFence(IFence* fence, uint64_t value) = 0;
    
    // ========== 命令列表 ==========
    
    /// 创建命令列表
    virtual std::unique_ptr<ICommandList> CreateCommandList() = 0;
    
    /// 提交命令列表
    virtual void SubmitCommandLists(std::span<ICommandList* const> commandLists) = 0;
    
    // ========== 帧资源管理 ==========
    
    /// 获取帧资源管理器
    virtual IFrameResourceManager* GetFrameResourceManager() = 0;
    
    /// 获取上传管理器
    virtual IUploadManager* GetUploadManager() = 0;
    
    // ========== 错误处理 ==========
    
    /// 设置设备丢失回调
    virtual void SetDeviceLostCallback(DeviceLostCallback callback) = 0;
    
    /// 检查设备是否丢失
    virtual bool IsDeviceLost() const = 0;
    
    /// 获取设备丢失原因
    virtual DeviceLostReason GetDeviceLostReason() const = 0;
    
    /// 尝试恢复设备
    virtual RHIResult TryRecover() = 0;
    
    /// 获取最后错误信息
    virtual const char* GetLastError() const = 0;
};

// =============================================================================
// 工厂函数
// =============================================================================

/**
 * @brief 创建 RHI 设备
 * @param desc 设备描述
 * @return std::unique_ptr<IDevice> 设备指针
 */
RHI_API std::unique_ptr<IDevice> CreateDevice(const DeviceDesc& desc);

} // namespace Engine::RHI
