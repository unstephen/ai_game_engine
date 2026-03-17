// =============================================================================
// Memory.cpp - 内存管理系统实现
// =============================================================================

#include "Memory.h"

namespace Engine {

static HeapAllocator g_defaultAllocator;

IAllocator* GetDefaultAllocator() {
    return &g_defaultAllocator;
}

} // namespace Engine