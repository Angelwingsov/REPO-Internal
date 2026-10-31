#include "input.h"
#include "core/config.h"
#include "core/logger.h"
#include "imgui/imgui.h"

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

LRESULT CALLBACK InputHook::WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto& cfg = Core::Config::Get();

    if (uMsg == WM_KEYDOWN) {
        if (wParam == cfg.menu.toggleKey || wParam == VK_DELETE || wParam == VK_INSERT) {
            cfg.menu.isVisible = !cfg.menu.isVisible;
            LOG_INFO("Menu toggled: %s", cfg.menu.isVisible ? "VISIBLE" : "HIDDEN");
            return 0;
        }
    }

    if (cfg.menu.isVisible) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return true;
        }

        if (cfg.menu.blockInputWhenOpen) {
            if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
                return true;
            }
            if (uMsg == WM_INPUT) {
                return true;
            }
            if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) {
                return true;
            }
        } else if (ImGui::GetCurrentContext()) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse && (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)) {
                return true;
            }
            if (io.WantCaptureKeyboard && (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)) {
                return true;
            }
        }
    }

    if (s_OriginalWndProc) {
        return CallWindowProc(s_OriginalWndProc, hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

}
