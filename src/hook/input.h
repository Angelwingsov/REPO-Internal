#pragma once
#include <windows.h>

namespace Hook {

class InputHook {
public:
    static bool Initialize(HWND window);
    static void Shutdown();

    static LRESULT CALLBACK WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static inline HWND s_Window = nullptr;
    static inline WNDPROC s_OriginalWndProc = nullptr;
};

}
