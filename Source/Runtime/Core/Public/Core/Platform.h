/**
 * @file Platform.h
 * @brief 平台兼容性层 - 解决 MSVC 与 POSIX 函数名兼容问题
 * 
 * MSVC 默认不支持 sinf/cosf/sqrtf 等 POSIX 函数名，
 * 需要定义 _CRT_NONSTDC_NO_WARNINGS 或手动映射到标准库版本。
 */

#pragma once

// ============================================================================
// MSVC 兼容层
// ============================================================================
#if defined(_MSC_VER)

    // 禁用 POSIX 函数名警告（sinf, cosf, sqrtf 等）
    #ifndef _CRT_NONSTDC_NO_WARNINGS
        #define _CRT_NONSTDC_NO_WARNINGS
    #endif
    
    // 禁用安全函数警告（sprintf, strcpy 等）
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
    
    // 启用 M_PI 等数学常量
    #ifndef _USE_MATH_DEFINES
        #define _USE_MATH_DEFINES
    #endif

    // 提供全局命名空间的 float 版本数学函数
    // DirectXMath 和其他库使用 sinf/cosf/sqrtf 等 POSIX 命名
    #include <cmath>

    // 使用 inline 函数而非宏，更安全且支持重载
    inline float sinf(float x) { return std::sin(x); }
    inline float cosf(float x) { return std::cos(x); }
    inline float tanf(float x) { return std::tan(x); }
    inline float sqrtf(float x) { return std::sqrt(x); }
    inline float asinf(float x) { return std::asin(x); }
    inline float acosf(float x) { return std::acos(x); }
    inline float atanf(float x) { return std::atan(x); }
    inline float atan2f(float y, float x) { return std::atan2(y, x); }
    inline float powf(float base, float exp) { return std::pow(base, exp); }
    inline float expf(float x) { return std::exp(x); }
    inline float logf(float x) { return std::log(x); }
    inline float log10f(float x) { return std::log10(x); }
    inline float fabsf(float x) { return std::fabs(x); }
    inline float floorf(float x) { return std::floor(x); }
    inline float ceilf(float x) { return std::ceil(x); }
    inline float roundf(float x) { return std::round(x); }
    inline float fmodf(float x, float y) { return std::fmod(x, y); }

#endif // _MSC_VER

// ============================================================================
// 通用平台定义
// ============================================================================

// Windows 平台
#if defined(_WIN32) || defined(_WIN64)
    #define ENGINE_PLATFORM_WINDOWS 1
    
    // 减少 Windows.h 包含的内容
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX  // 禁用 min/max 宏
    #endif

// Linux 平台
#elif defined(__linux__)
    #define ENGINE_PLATFORM_LINUX 1

// macOS 平台
#elif defined(__APPLE__)
    #define ENGINE_PLATFORM_MACOS 1

// 未知平台
#else
    #error "Unsupported platform"
#endif

// ============================================================================
// 编译器检测
// ============================================================================

#if defined(_MSC_VER)
    #define ENGINE_COMPILER_MSVC 1
    #define ENGINE_COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
    #define ENGINE_COMPILER_CLANG 1
    #define ENGINE_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
    #define ENGINE_COMPILER_GCC 1
    #define ENGINE_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
    #error "Unsupported compiler"
#endif

// ============================================================================
// 导出/导入宏
// ============================================================================

#if defined(_MSC_VER)
    #define ENGINE_EXPORT __declspec(dllexport)
    #define ENGINE_IMPORT __declspec(dllimport)
#else
    #define ENGINE_EXPORT __attribute__((visibility("default")))
    #define ENGINE_IMPORT __attribute__((visibility("default")))
#endif

// 核心库导出
#ifdef ENGINE_CORE_EXPORTS
    #define ENGINE_CORE_API ENGINE_EXPORT
#else
    #define ENGINE_CORE_API ENGINE_IMPORT
#endif

// RHI 导出
#ifdef ENGINE_RHI_EXPORTS
    #define ENGINE_RHI_API ENGINE_EXPORT
#else
    #define ENGINE_RHI_API ENGINE_IMPORT
#endif

// D3D12 后端导出
#if defined(ENGINE_RHI_D3D12_EXPORTS)
    #define ENGINE_RHI_D3D12_API ENGINE_EXPORT
#else
    #define ENGINE_RHI_D3D12_API ENGINE_IMPORT
#endif