#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <share.h>
#include <dbghelp.h>

namespace Core {
namespace {
    HMODULE crashLibrary = nullptr;
    decltype(&MiniDumpWriteDump) writeCrashDump = nullptr;
    volatile LONG handlingCrash = 0;
}


void Logger::Initialize(const char* consoleTitle) {
    std::lock_guard<std::mutex> lock(s_LogMutex);
    if (s_Initialized) return;

    if (AllocConsole()) {
        freopen_s(&s_ConsoleOut, "CONOUT$", "w", stdout);
        freopen_s(&s_ConsoleErr, "CONOUT$", "w", stderr);
        freopen_s(&s_ConsoleIn, "CONIN$", "r", stdin);

        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        if (consoleTitle) {
            SetConsoleTitleA(consoleTitle);
        }

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
    }

    // Allow diagnostics to be read while the game is running.
    s_LogFile = _fsopen("repo_cheat.log", "w", _SH_DENYNO);
    crashLibrary = LoadLibraryA("dbghelp.dll");
    if (crashLibrary) writeCrashDump = reinterpret_cast<decltype(writeCrashDump)>(GetProcAddress(crashLibrary, "MiniDumpWriteDump"));
    s_PrevExceptionFilter = SetUnhandledExceptionFilter(UnhandledExceptionFilter);

    s_Initialized = true;

    printf("\033[96m============================================================\033[0m\n");
    printf("\033[96m                        Console                             \033[0m\n");
    printf("\033[96m============================================================\033[0m\n\n");
    fflush(stdout);
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(s_LogMutex);
    if (!s_Initialized) return;

    // A null previous filter still has to be restored before this DLL is unloaded.
    SetUnhandledExceptionFilter(s_PrevExceptionFilter);
    s_PrevExceptionFilter = nullptr;
    writeCrashDump = nullptr;
    if (crashLibrary) { FreeLibrary(crashLibrary); crashLibrary = nullptr; }

    if (s_LogFile) {
        fflush(s_LogFile);
        fclose(s_LogFile);
        s_LogFile = nullptr;
    }

    if (s_ConsoleOut) { fclose(s_ConsoleOut); s_ConsoleOut = nullptr; }
    if (s_ConsoleErr) { fclose(s_ConsoleErr); s_ConsoleErr = nullptr; }
    if (s_ConsoleIn)  { fclose(s_ConsoleIn);  s_ConsoleIn = nullptr; }

    FreeConsole();
    s_Initialized = false;
}

std::string Logger::GetTimestamp() {
    time_t now = time(nullptr);
    tm t{};
    localtime_s(&t, &now);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    return std::string(buf);
}

void Logger::Log(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(level, format, args);
    va_end(args);
}

void Logger::LogInternal(LogLevel level, const char* format, va_list args) {
    std::lock_guard<std::mutex> lock(s_LogMutex);

    const char* prefix = "[*]";
    const char* color = "\033[94m";
    const char* filePrefix = "[INFO]";

    switch (level) {
    case LogLevel::Debug:
        prefix = "[?]";
        color = "\033[90m";
        filePrefix = "[DEBUG]";
        break;
    case LogLevel::Info:
        prefix = "[*]";
        color = "\033[94m";
        filePrefix = "[INFO]";
        break;
    case LogLevel::Success:
        prefix = "[+]";
        color = "\033[92m";
        filePrefix = "[OK]";
        break;
    case LogLevel::Warning:
        prefix = "[!]";
        color = "\033[93m";
        filePrefix = "[WARN]";
        break;
    case LogLevel::Error:
        prefix = "[-]";
        color = "\033[91m";
        filePrefix = "[ERROR]";
        break;
    }

    std::string timestamp = GetTimestamp();

    printf("\033[90m[%s]\033[0m %s%s\033[0m ", timestamp.c_str(), color, prefix);

    va_list argsCopy;
    va_copy(argsCopy, args);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);

    if (s_LogFile) {
        fprintf(s_LogFile, "[%s] %s ", timestamp.c_str(), filePrefix);
        vfprintf(s_LogFile, format, argsCopy);
        fprintf(s_LogFile, "\n");
        fflush(s_LogFile);
    }
    va_end(argsCopy);
}

LONG WINAPI Logger::UnhandledExceptionFilter(EXCEPTION_POINTERS* exception) {
    if (!exception || !exception->ExceptionRecord || InterlockedCompareExchange(&handlingCrash, 1, 0))
        return EXCEPTION_CONTINUE_SEARCH;
    // Never acquire the logger mutex or use its FILE streams on the faulting thread:
    // the exception may have interrupted a log write while that mutex was held.
    HMODULE module = nullptr;
    char modulePath[MAX_PATH] = "Unknown / JIT memory";
    auto* record = exception->ExceptionRecord;
    uintptr_t rva = 0;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(record->ExceptionAddress), &module)) {
        GetModuleFileNameA(module, modulePath, MAX_PATH);
        rva = reinterpret_cast<uintptr_t>(record->ExceptionAddress) - reinterpret_cast<uintptr_t>(module);
    }
    char report[2048]{};
    int length = _snprintf_s(report, sizeof(report), _TRUNCATE,
        "Build: %s %s\r\nCode: 0x%08lX\r\nThread: %lu\r\nAddress: %p\r\nModule: %s\r\nRVA: 0x%llX\r\n",
        __DATE__, __TIME__, record->ExceptionCode, GetCurrentThreadId(), record->ExceptionAddress,
        modulePath, static_cast<unsigned long long>(rva));
    HANDLE file = CreateFileA("repo_cheat_crash.log", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        if (length > 0) WriteFile(file, report, static_cast<DWORD>(length), &written, nullptr);
        if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
            length = _snprintf_s(report, sizeof(report), _TRUNCATE, "Operation: %llu; target: %p\r\n",
                static_cast<unsigned long long>(record->ExceptionInformation[0]), reinterpret_cast<void*>(record->ExceptionInformation[1]));
            if (length > 0) WriteFile(file, report, static_cast<DWORD>(length), &written, nullptr);
        }
        FlushFileBuffers(file);
        CloseHandle(file);
    }
    if (writeCrashDump) {
        HANDLE dump = CreateFileA("repo_cheat_crash.dmp", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (dump != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), exception, FALSE};
            writeCrashDump(GetCurrentProcess(), GetCurrentProcessId(), dump, MiniDumpNormal, &info, nullptr, nullptr);
            CloseHandle(dump);
        }
    }
    return s_PrevExceptionFilter ? s_PrevExceptionFilter(exception) : EXCEPTION_CONTINUE_SEARCH;
}

}
