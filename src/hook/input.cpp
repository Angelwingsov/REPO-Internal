#include "input.h"
#include "core/config.h"
#include "core/logger.h"
#include "imgui/imgui.h"
#include "core/hook_activity.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Hook {

bool InputHook::Initialize(HWND window) {
    if (!window || s_OriginalWndProc) return false;

    s_Window = window;
    s_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHandler)));

    if (!s_OriginalWndProc) {
        LOG_ERR("Failed to hook WndProc!");
        return false;
    }

    LOG_OK("WndProc hooked successfully on window: %p", window);
    return true;
}

void InputHook::Shutdown() {
    if (s_Window && s_OriginalWndProc) {
        SetWindowLongPtr(s_Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(s_OriginalWndProc));
        s_OriginalWndProc = nullptr;
        s_Window = nullptr;
        LOG_OK("WndProc restored successfully!");
    }
}

bool InputHook::ConsumeWheelUp() {
    if (!s_WheelUpConsumed && (GetTickCount64() - s_LastWheelUpTime < 250)) {
        s_WheelUpConsumed = true;
        return true;
    }
    return false;
}

bool InputHook::ConsumeWheelDown() {
    if (!s_WheelDownConsumed && (GetTickCount64() - s_LastWheelDownTime < 250)) {
        s_WheelDownConsumed = true;
        return true;
    }
    return false;
}

bool InputHook::IsKeyHeld(int vkCode) {
    if (vkCode <= 0) return false;

    if (s_Window && GetForegroundWindow() != s_Window) {
        return false;
    }

    if (vkCode == VK_MWHEELUP) {
        return (GetTickCount64() - s_LastWheelUpTime) < 200;
    }
    if (vkCode == VK_MWHEELDOWN) {
        return (GetTickCount64() - s_LastWheelDownTime) < 200;
    }
    return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
}

void InputHook::ReleaseAllKeys(HWND hWnd, WNDPROC originalProc) {
    if (!hWnd || !originalProc) return;

    for (int vk = 1; vk < 256; ++vk) {
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
            vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
            continue;
        }

        if (((GetAsyncKeyState(vk) & 0x8000) != 0) || ((GetKeyState(vk) & 0x8000) != 0)) {
            UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
            LPARAM lParamUp = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);
            CallWindowProc(originalProc, hWnd, WM_KEYUP, static_cast<WPARAM>(vk), lParamUp);
        }
    }

    static const int s_CommonKeys[] = {
        'W', 'A', 'S', 'D',
        VK_SPACE, VK_SHIFT, VK_LSHIFT, VK_RSHIFT,
        VK_CONTROL, VK_LCONTROL, VK_RCONTROL,
        VK_MENU, VK_LMENU, VK_RMENU,
        VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT,
        'E', 'F', 'Q', 'C', 'R', 'Z', 'X', VK_TAB
    };
    for (int vk : s_CommonKeys) {
        UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        LPARAM lParamUp = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);
        CallWindowProc(originalProc, hWnd, WM_KEYUP, static_cast<WPARAM>(vk), lParamUp);
    }

    CallWindowProc(originalProc, hWnd, WM_LBUTTONUP, 0, 0);
    CallWindowProc(originalProc, hWnd, WM_RBUTTONUP, 0, 0);
    CallWindowProc(originalProc, hWnd, WM_MBUTTONUP, 0, 0);
    CallWindowProc(originalProc, hWnd, WM_XBUTTONUP, MAKEWPARAM(0, XBUTTON1), 0);
    CallWindowProc(originalProc, hWnd, WM_XBUTTONUP, MAKEWPARAM(0, XBUTTON2), 0);
}

LRESULT CALLBACK InputHook::WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Core::HookActivity activity;
    std::unique_lock<std::recursive_mutex> lock(Core::Config::Mutex());
    auto& cfg = Core::Config::Global();

    if (uMsg == WM_MOUSEWHEEL) {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) {
            s_LastWheelUpTime = GetTickCount64();
            s_WheelUpConsumed = false;
        } else if (delta < 0) {
            s_LastWheelDownTime = GetTickCount64();
            s_WheelDownConsumed = false;
        }
    }

    bool toggleMenu = false;

    if (uMsg == WM_KEYDOWN && !(lParam & (1LL << 30))) {
        if (wParam == cfg.menu.toggleKey || wParam == VK_DELETE || wParam == VK_INSERT) {
            toggleMenu = true;
        }
    } else if (uMsg == WM_MBUTTONDOWN) {
        if (cfg.menu.toggleKey == VK_MBUTTON) {
            toggleMenu = true;
        }
    } else if (uMsg == WM_XBUTTONDOWN) {
        WORD xbutton = GET_XBUTTON_WPARAM(wParam);
        DWORD vk = (xbutton == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        if (cfg.menu.toggleKey == vk) {
            toggleMenu = true;
        }
    } else if (uMsg == WM_MOUSEWHEEL) {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0 && cfg.menu.toggleKey == VK_MWHEELUP) {
            bool canToggle = !cfg.menu.isVisible;
            if (cfg.menu.isVisible && ImGui::GetCurrentContext()) {
                canToggle = !ImGui::GetIO().WantCaptureMouse;
            }
            static ULONGLONG s_LastMenuWheelUp = 0;
            ULONGLONG now = GetTickCount64();
            if (canToggle && (now - s_LastMenuWheelUp > 300)) {
                s_LastMenuWheelUp = now;
                toggleMenu = true;
            }
        } else if (delta < 0 && cfg.menu.toggleKey == VK_MWHEELDOWN) {
            bool canToggle = !cfg.menu.isVisible;
            if (cfg.menu.isVisible && ImGui::GetCurrentContext()) {
                canToggle = !ImGui::GetIO().WantCaptureMouse;
            }
            static ULONGLONG s_LastMenuWheelDown = 0;
            ULONGLONG now = GetTickCount64();
            if (canToggle && (now - s_LastMenuWheelDown > 300)) {
                s_LastMenuWheelDown = now;
                toggleMenu = true;
            }
        }
    }

    if (toggleMenu) {
        cfg.menu.isVisible = !cfg.menu.isVisible;
        LOG_INFO("Menu toggled: %s", cfg.menu.isVisible ? "VISIBLE" : "HIDDEN");

        WNDPROC original = s_OriginalWndProc;
        if (original) {
            ReleaseAllKeys(hWnd, original);
        }

        if (ImGui::GetCurrentContext()) {
            ImGuiIO& io = ImGui::GetIO();
            io.ClearInputKeys();
            io.ClearEventsQueue();
        }
        return 0;
    }

    if (cfg.menu.isVisible) {
        if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) {
                WNDPROC original = s_OriginalWndProc;
                if (original) {
                    CallWindowProc(original, hWnd, uMsg, wParam, lParam);
                }
            }
            return true;
        }

        if (cfg.menu.blockInputWhenOpen) {
            if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
                return true;
            }
            if (uMsg == WM_INPUT) {
                RAWINPUT raw{};
                UINT size = sizeof(raw);
                if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != (UINT)-1) {
                    if (raw.header.dwType == RIM_TYPEKEYBOARD && (raw.data.keyboard.Flags & RI_KEY_BREAK)) {
                        WNDPROC original = s_OriginalWndProc;
                        lock.unlock();
                        if (original) {
                            return CallWindowProc(original, hWnd, uMsg, wParam, lParam);
                        }
                        return 0;
                    }
                }
                return true;
            }
            if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) {
                if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) {
                    WNDPROC original = s_OriginalWndProc;
                    lock.unlock();
                    if (original) {
                        return CallWindowProc(original, hWnd, uMsg, wParam, lParam);
                    }
                    return 0;
                }
                return true;
            }
        } else if (ImGui::GetCurrentContext()) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse && (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)) {
                return true;
            }
            if (io.WantCaptureKeyboard && (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)) {
                if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) {
                    WNDPROC original = s_OriginalWndProc;
                    lock.unlock();
                    if (original) {
                        return CallWindowProc(original, hWnd, uMsg, wParam, lParam);
                    }
                    return 0;
                }
                return true;
            }
        }
    }

    WNDPROC original = s_OriginalWndProc;
    lock.unlock();
    if (original) {
        return CallWindowProc(original, hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

}
