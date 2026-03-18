// =============================================================================
// Core.h - 核心系统公共头文件
// =============================================================================

#pragma once

// 平台检测 - 必须在任何标准库头文件之前定义
#if defined(_WIN32) || defined(_WIN64)
#define ENGINE_PLATFORM_WINDOWS 1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// 先包含标准库头文件，再包含 Windows.h，避免数学函数冲突
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <math.h>
#include <Windows.h>
#else
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#endif

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// 确保数学函数可用 (MSVC 兼容性)
#ifdef _MSC_VER
#include <float.h>
#endif

// 导出宏
#ifdef ENGINE_CORE_EXPORTS
#define ENGINE_CORE_API __declspec(dllexport)
#else
#define ENGINE_CORE_API __declspec(dllimport)
#endif

namespace Engine
{

// 版本信息
constexpr uint32_t ENGINE_VERSION_MAJOR = 0;
constexpr uint32_t ENGINE_VERSION_MINOR = 1;
constexpr uint32_t ENGINE_VERSION_PATCH = 0;

} // namespace Engine