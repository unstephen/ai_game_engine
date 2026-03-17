// =============================================================================
// Device.cpp - 设备工厂实现
// =============================================================================

#include "IDevice.h"

#if ENGINE_RHI_D3D12
#include "D3D12Device.h"
#endif

namespace Engine::RHI {

std::unique_ptr<IDevice> CreateDevice(const DeviceDesc& desc) {
#if ENGINE_RHI_D3D12
    if (desc.backend == Backend::D3D12) {
        auto device = std::make_unique<D3D12Device>();
        RHIResult result = device->Initialize(desc);
        if (result == RHIResult::Success) {
            return device;
        }
        return nullptr;
    }
#endif
    
    LogMessage(LogLevel::Error, "CreateDevice: 不支持的后端类型");
    return nullptr;
}

} // namespace Engine::RHI