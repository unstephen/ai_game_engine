// =============================================================================
// Core.h - 核心系统公共头文件
// =============================================================================

#pragma once

// 平台检测
#if defined(_WIN32) || defined(_WIN64)
#define ENGINE_PLATFORM_WINDOWS 1
#endif

// 标准库头文件 - 必须在 Windows.h 之前包含
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <math.h>

// Windows 头文件 (在标准库之后包含)
#if ENGINE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

// 其他标准库
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// MSVC 特定
#ifdef _MSC_VER
#include <float.h>
#include <intrin.h>
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