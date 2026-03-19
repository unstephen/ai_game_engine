// =============================================================================
// Log.h - 日志系统
// =============================================================================

#pragma once

#include "Core.h"

// MSVC 下使用 C 标准库头文件
#ifdef _MSC_VER
#include <stdarg.h>
#include <stdio.h>
#else
#include <cstdarg>
#include <cstdio>
#endif

namespace Engine
{

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

ENGINE_CORE_API void Log(LogLevel level, const char* format, ...);
ENGINE_CORE_API void LogV(LogLevel level, const char* format, va_list args);

// 便捷宏
#define LOG_TRACE(...) Engine::Log(Engine::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) Engine::Log(Engine::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) Engine::Log(Engine::LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) Engine::Log(Engine::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) Engine::Log(Engine::LogLevel::Error, __VA_ARGS__)
#define LOG_FATAL(...) Engine::Log(Engine::LogLevel::Fatal, __VA_ARGS__)

} // namespace Engine