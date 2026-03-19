// =============================================================================
// Memory.h - 内存管理系统
// =============================================================================

#pragma once

#include "Core.h"

// MSVC 下使用 C 标准库头文件
#ifdef _MSC_VER
#include <stdlib.h>
#else
#include <cstdlib>
#endif

#include <new>

namespace Engine
{

// 内存分配器接口
class ENGINE_CORE_API IAllocator
{
  public:
    virtual ~IAllocator() = default;

    virtual void*  Allocate(size_t size, size_t alignment = 16) = 0;
    virtual void   Deallocate(void* ptr)                        = 0;
    virtual size_t GetAllocatedSize() const                     = 0;
};

// 默认堆分配器
class ENGINE_CORE_API HeapAllocator : public IAllocator
{
  public:
    void* Allocate(size_t size, size_t alignment = 16) override
    {
        return ::operator new(size, std::align_val_t(alignment));
    }

    void Deallocate(void* ptr) override { ::operator delete(ptr); }

    size_t GetAllocatedSize() const override { return 0; }
};

// 全局分配器
ENGINE_CORE_API IAllocator* GetDefaultAllocator();

} // namespace Engine