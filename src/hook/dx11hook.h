#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atomic>

namespace Hook {

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

class DX11Hook {
public:
    static bool Initialize();
    static void Shutdown();

    static HRESULT __stdcall PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT __stdcall ResizeBuffersHook(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

    static void CreateRenderTarget(IDXGISwapChain* pSwapChain);
    static void CleanupRenderTarget();
    static void RenderOverlay(IDXGISwapChain* swapChain);
    static void CleanupOverlay();
    static void GuardedRender(IDXGISwapChain* swapChain);
    static void InitializeGame();
    static DWORD WINAPI ShutdownThread(LPVOID);

    static inline bool s_Hooked = false;
    static inline bool s_ImGuiInitialized = false;
    static inline std::atomic<bool> s_Unloading{false}, s_RenderCleanupRequested{false};
    static inline std::atomic<bool> s_OverlayStopped{false}, s_RenderCleanupDone{false};
    static inline std::atomic<bool> s_GameReady{false}, s_GameInitAttempted{false};
    static inline HMODULE s_ModuleHandle = nullptr;

    static inline Present_t s_OriginalPresent = nullptr;
    static inline ResizeBuffers_t s_OriginalResizeBuffers = nullptr;

    static inline ID3D11Device* s_Device = nullptr;
    static inline ID3D11DeviceContext* s_Context = nullptr;
    static inline ID3D11RenderTargetView* s_RenderTargetView = nullptr;
    static inline HWND s_Window = nullptr;
    static inline std::atomic<IDXGISwapChain*> s_SwapChain{nullptr};
};

}
