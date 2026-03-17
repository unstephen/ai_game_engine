// =============================================================================
// IShader.h - 着色器接口
// =============================================================================

#pragma once

#include "RHICore.h"

#include <string>
#include <vector>

namespace Engine::RHI
{

// =============================================================================
// 着色器描述
// =============================================================================

struct ShaderDesc
{
    ShaderStage stage      = ShaderStage::Vertex;
    const char* source     = nullptr; // 源代码
    size_t      sourceSize = 0;       // 源代码大小
    const char* entryPoint = "main";  // 入口点
    const char* debugName  = nullptr; // 调试名称

    // 编译选项
    const char*  includeDir  = nullptr; // 包含目录
    const char** defines     = nullptr; // 宏定义
    uint32_t     defineCount = 0;       // 宏定义数量
};

// =============================================================================
// IShader - 着色器接口
// =============================================================================

class RHI_API IShader
{
  public:
    virtual ~IShader() = default;

    /// 获取着色器阶段
    virtual ShaderStage GetStage() const = 0;

    /// 获取编译后的字节码
    virtual const void* GetBytecode() const = 0;

    /// 获取字节码大小
    virtual size_t GetBytecodeSize() const = 0;

    /// 获取入口点名称
    virtual const char* GetEntryPoint() const = 0;
};

} // namespace Engine::RHI