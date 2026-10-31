#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace Core {

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

    fopen_s(&s_LogFile, "repo_cheat.log", "w");
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

    if (s_PrevExceptionFilter) {
        SetUnhandledExceptionFilter(s_PrevExceptionFilter);
        s_PrevExceptionFilter = nullptr;
    }

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

LONG WINAPI Logger::UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo) {
    if (!pExceptionInfo || !pExceptionInfo->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    std::lock_guard<std::mutex> lock(s_LogMutex);

    printf("\n\n\033[91m============================================================\033[0m\n");
    printf("\033[91m[!] CRITICAL FATAL EXCEPTION DETECTED!\033[0m\n");
    printf("[!] Exception Code    : 0x%08lX\n", (unsigned long)pExceptionInfo->ExceptionRecord->ExceptionCode);
    printf("[!] Exception Address : 0x%p\n", pExceptionInfo->ExceptionRecord->ExceptionAddress);

    HMODULE hModule = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(pExceptionInfo->ExceptionRecord->ExceptionAddress), &hModule)) {
        char modName[MAX_PATH] = { 0 };
        GetModuleFileNameA(hModule, modName, MAX_PATH);
        printf("[!] In Module         : %s\n", modName);
        uintptr_t rva = reinterpret_cast<uintptr_t>(pExceptionInfo->ExceptionRecord->ExceptionAddress) - reinterpret_cast<uintptr_t>(hModule);
        printf("[!] Module Base       : 0x%p (RVA: +0x%llX)\n", static_cast<void*>(hModule), static_cast<unsigned long long>(rva));
    } else {
        printf("[!] In Module         : Unknown / JIT-generated memory\n");
    }

    if (pExceptionInfo->ContextRecord) {
#if defined(_M_X64) || defined(__x86_64__)
        printf("[!] RIP: 0x%p  RAX: 0x%p  RBX: 0x%p\n",
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Rip),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Rax),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Rbx));
        printf("[!] RCX: 0x%p  RDX: 0x%p  RSP: 0x%p\n",
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Rcx),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Rdx),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Rsp));
#elif defined(_M_IX86) || defined(__i386__)
        printf("[!] EIP: 0x%p  EAX: 0x%p  EBX: 0x%p\n",
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Eip),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Eax),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Ebx));
        printf("[!] ECX: 0x%p  EDX: 0x%p  ESP: 0x%p\n",
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Ecx),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Edx),
               reinterpret_cast<void*>(pExceptionInfo->ContextRecord->Esp));
#endif
    }
    printf("\033[91m============================================================\033[0m\n");
    fflush(stdout);

    FILE* crashFile = nullptr;
    fopen_s(&crashFile, "repo_cheat_crash.log", "w");
    if (crashFile) {
        fprintf(crashFile, "Fatal Crash Detected!\nCode: 0x%08lX\nAddress: 0x%p\n",
                (unsigned long)pExceptionInfo->ExceptionRecord->ExceptionCode,
                pExceptionInfo->ExceptionRecord->ExceptionAddress);
        fclose(crashFile);
    }

    Sleep(5000);
    return EXCEPTION_CONTINUE_SEARCH;
}

}
