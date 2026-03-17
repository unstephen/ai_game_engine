// =============================================================================
// ICommandList.h - 命令列表接口
// =============================================================================

#pragma once

#include "RHICore.h"

namespace Engine::RHI
{

// =============================================================================
// ICommandList - 命令列表接口
// =============================================================================

class RHI_API ICommandList
{
  public:
    virtual ~ICommandList() = default;

    /// 开始命令列表
    virtual void Begin() = 0;

    /// 结束命令列表
    virtual void End() = 0;

    /// 获取原生命令列表
    virtual void* GetNativeCommandList() const = 0;

    /// 获取命令列表类型
    virtual uint32_t GetType() const = 0;

    // ========== 资源屏障 ==========

    /// 转换资源状态
    virtual void ResourceBarrier(ITexture* texture, ResourceState newState) = 0;
    virtual void ResourceBarrier(IBuffer* buffer, ResourceState newState)   = 0;

    // ========== 清除操作 ==========

    /// 清除渲染目标
    virtual void ClearRenderTarget(ITexture* renderTarget, const float clearColor[4]) = 0;

    /// 清除深度模板
    virtual void ClearDepthStencil(ITexture* depthStencil, bool clearDepth, bool clearStencil, float depth,
                                   uint8_t stencil) = 0;

    // ========== 绘制调用 ==========

    /// 设置管线状态
    virtual void SetPipelineState(IPipelineState* pipeline) = 0;

    /// 设置根签名
    virtual void SetRootSignature(IRootSignature* rootSignature) = 0;

    /// 设置渲染目标
    virtual void SetRenderTargets(ITexture* const* renderTargets, uint32_t count, ITexture* depthStencil = nullptr) = 0;

    /// 设置视口
    virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f,
                             float maxDepth = 1.0f) = 0;

    /// 设置剪裁矩形
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) = 0;

    /// 设置图元拓扑
    virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;

    /// 设置顶点缓冲区
    virtual void SetVertexBuffer(uint32_t slot, IBuffer* buffer, uint64_t offset = 0) = 0;

    /// 设置索引缓冲区
    virtual void SetIndexBuffer(IBuffer* buffer, uint64_t offset = 0) = 0;

    /// 绘制
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;

    /// 绘制索引
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;

    /// 绘制实例化
    virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex = 0,
                               uint32_t startInstance = 0) = 0;

    /// 绘制索引实例化
    virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex = 0,
                                      int32_t baseVertex = 0, uint32_t startInstance = 0) = 0;
};

} // namespace Engine::RHI