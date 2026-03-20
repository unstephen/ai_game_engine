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
// MSVC: 必须在 C++ 标准库头文件之前包含 C 标准库头文件
#include <math.h>
#include <stdlib.h>
#include <cmath>

// MSVC 的 C++ 标准库不提供 sinf/cosf/sqrtf 等全局函数
// DirectXMath 和其他库使用这些 POSIX 命名，需要手动提供
inline float sinf(float _X) { return std::sin(_X); }
inline float cosf(float _X) { return std::cos(_X); }
inline float tanf(float _X) { return std::tan(_X); }
inline float sqrtf(float _X) { return std::sqrt(_X); }
inline float asinf(float _X) { return std::asin(_X); }
inline float acosf(float _X) { return std::acos(_X); }
inline float atanf(float _X) { return std::atan(_X); }
inline float atan2f(float _Y, float _X) { return std::atan2(_Y, _X); }
inline float powf(float _X, float _Y) { return std::pow(_X, _Y); }
inline float expf(float _X) { return std::exp(_X); }
inline float logf(float _X) { return std::log(_X); }
inline float log10f(float _X) { return std::log10(_X); }
inline float fabsf(float _X) { return std::fabs(_X); }
inline float floorf(float _X) { return std::floor(_X); }
inline float ceilf(float _X) { return std::ceil(_X); }
inline float roundf(float _X) { return std::round(_X); }
inline float fmodf(float _X, float _Y) { return std::fmod(_X, _Y); }
inline float modff(float _X, float* _Ip) { return std::modf(_X, _Ip); }
#endif
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