#pragma once
#include <windows.h>
#include <cstdint>
#include <atomic>

#ifndef VK_MWHEELUP
#define VK_MWHEELUP   0x0E
#endif
#ifndef VK_MWHEELDOWN
#define VK_MWHEELDOWN 0x0F
#endif

namespace Hook {

class InputHook {
public:
    static bool Initialize(HWND window);
    static void Shutdown();

    static LRESULT CALLBACK WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static bool IsKeyHeld(int vkCode);
    static bool ConsumeWheelUp();
    static bool ConsumeWheelDown();

    static inline std::atomic<HWND> s_Window{nullptr};
    static inline WNDPROC s_OriginalWndProc = nullptr;

    static inline std::atomic<ULONGLONG> s_LastWheelUpTime{0};
    static inline std::atomic<ULONGLONG> s_LastWheelDownTime{0};
    static inline bool s_WheelUpConsumed = true;
    static inline bool s_WheelDownConsumed = true;
};

}
