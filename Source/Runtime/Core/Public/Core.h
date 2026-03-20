// =============================================================================
// Core.h - 核心系统公共头文件
// =============================================================================

#pragma once

// 平台检测
#if defined(_WIN32) || defined(_WIN64)
#define ENGINE_PLATFORM_WINDOWS 1
// MSVC 兼容性定义
#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

// MSVC 关键修复：使用 _CRT_DECLARE_NONSTDC_NAMES 启用 POSIX 函数名
// 这会让 MSVC 在全局命名空间提供 sinf/cosf/sqrtf 等函数
#define _CRT_DECLARE_NONSTDC_NAMES 1
#include <math.h>
#include <stdlib.h>
#include <cmath>
#endif

// 标准库头文件
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// Windows 头文件 (在标准库之后包含)
#if ENGINE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// mingw 下使用小写 windows.h（Linux 大小写敏感）
#ifdef __MINGW32__
#include <windows.h>
#else
#include <Windows.h>
#endif

// =============================================================================
// 立即 #undef Windows API 宏 - 必须在命名空间内使用函数名之前
// =============================================================================
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateWindowA
#undef CreateWindowA
#endif
#ifdef CreateWindowW
#undef CreateWindowW
#endif
#ifdef DestroyWindow
#undef DestroyWindow
#endif
#ifdef Reset
#undef Reset
#endif
#ifdef Submit
#undef Submit
#endif
#ifdef Allocate
#undef Allocate
#endif
#ifdef CopyFile
#undef CopyFile
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef MoveFile
#undef MoveFile
#endif
#ifdef GetVersion
#undef GetVersion
#endif
#ifdef Yield
#undef Yield
#endif
#endif  // ENGINE_PLATFORM_WINDOWS

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