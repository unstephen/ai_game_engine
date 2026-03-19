// =============================================================================
// Log.cpp - 日志系统实现
// ============================================================================

#include "Log.h"

#include <cstring>
#include <ctime>

namespace Engine
{

void Log(LogLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogV(level, format, args);
    va_end(args);
}

void LogV(LogLevel level, const char* format, va_list args)
{
    const char* levelStr = "";
    switch (level)
    {
        case LogLevel::Trace:
            levelStr = "[TRACE]";
            break;
        case LogLevel::Debug:
            levelStr = "[DEBUG]";
            break;
        case LogLevel::Info:
            levelStr = "[INFO] ";
            break;
        case LogLevel::Warning:
            levelStr = "[WARN] ";
            break;
        case LogLevel::Error:
            levelStr = "[ERROR]";
            break;
        case LogLevel::Fatal:
            levelStr = "[FATAL]";
            break;
    }

    // 时间戳
    time_t     now = time(nullptr);
    struct tm* t   = localtime(&now);
    char       timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", t);

    printf("%s %s ", timeStr, levelStr);
    vprintf(format, args);
    printf("\n");

    // Fatal 级别直接退出
    if (level == LogLevel::Fatal)
    {
        abort();
    }
}

} // namespace Engine