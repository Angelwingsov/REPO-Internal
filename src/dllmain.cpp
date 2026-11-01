#include <windows.h>
#include "core/logger.h"
#include "hook/dx11hook.h"

DWORD WINAPI MainThread(LPVOID lpParam) {
    Core::Logger::Initialize();
    LOG_OK("R.E.P.O build %s %s loaded into PID: %lu", __DATE__, __TIME__, GetCurrentProcessId());
    LOG_INFO("MainThread started (Thread ID: %lu)...", GetCurrentThreadId());
    LOG_INFO("Waiting for d3d11.dll and mono runtime...");

    while (GetModuleHandleA("d3d11.dll") == nullptr || 
          (GetModuleHandleA("mono-2.0-bdwgc.dll") == nullptr && GetModuleHandleA("mono.dll") == nullptr)) {
        Sleep(100);
    }

    LOG_OK("Core modules loaded: d3d11.dll=%p, mono=%p", 
           GetModuleHandleA("d3d11.dll"),
           GetModuleHandleA("mono-2.0-bdwgc.dll") ? GetModuleHandleA("mono-2.0-bdwgc.dll") : GetModuleHandleA("mono.dll"));

    Sleep(500);

    if (!Hook::DX11Hook::Initialize()) {
        LOG_ERR("Failed to initialize DX11 hooks!");
        return 0;
    }

    LOG_OK("DX11 Hooks successfully installed! Waiting for rendering loop...");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        Hook::DX11Hook::s_ModuleHandle = hModule;

        HANDLE hThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
