#pragma once
#include <windows.h>
#include <string>
#include <mutex>

namespace Core {

enum class LogLevel {
    Debug,
    Info,
    Success,
    Warning,
    Error
};

class Logger {
public:
    static void Initialize(const char* consoleTitle = "Console");
    static void Shutdown();

    static void Log(LogLevel level, const char* format, ...);
    static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo);

private:
    static void LogInternal(LogLevel level, const char* format, va_list args);
    static std::string GetTimestamp();

    static inline bool s_Initialized = false;
    static inline FILE* s_ConsoleOut = nullptr;
    static inline FILE* s_ConsoleIn = nullptr;
    static inline FILE* s_ConsoleErr = nullptr;
    static inline FILE* s_LogFile = nullptr;
    static inline std::mutex s_LogMutex;
    static inline LPTOP_LEVEL_EXCEPTION_FILTER s_PrevExceptionFilter = nullptr;
};

}

#define LOG_DEBUG(...) ::Core::Logger::Log(::Core::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  ::Core::Logger::Log(::Core::LogLevel::Info, __VA_ARGS__)
#define LOG_OK(...)    ::Core::Logger::Log(::Core::LogLevel::Success, __VA_ARGS__)
#define LOG_WARN(...)  ::Core::Logger::Log(::Core::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERR(...)   ::Core::Logger::Log(::Core::LogLevel::Error, __VA_ARGS__)
