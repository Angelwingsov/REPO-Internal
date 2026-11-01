#include "dx11hook.h"
#include "input.h"
#include "core/config.h"
#include "core/logger.h"
#include "mono/mono_bridge.h"
#include "unity/unity_engine.h"
#include "game/state.h"
#include "game/runtime.h"
#include "core/hook_activity.h"
#include "features/esp.h"
#include "features/godmode.h"
#include "features/noclip.h"
#include "features/player.h"
#include "features/thirdperson.h"
#include "features/spectatorlist.h"
#include "features/removals.h"
#include "ui/theme.h"
#include "ui/menu.h"
#include "minhook/include/MinHook.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

namespace Hook {

void DX11Hook::CreateRenderTarget(IDXGISwapChain* pSwapChain) {
    if (!pSwapChain || !s_Device) return;

    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (SUCCEEDED(hr) && pBackBuffer) {
        s_Device->CreateRenderTargetView(pBackBuffer, nullptr, &s_RenderTargetView);
        pBackBuffer->Release();
    }
}

void DX11Hook::CleanupRenderTarget() {
    if (s_RenderTargetView) {
        s_RenderTargetView->Release();
        s_RenderTargetView = nullptr;
    }
}

HRESULT __stdcall DX11Hook::ResizeBuffersHook(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    Core::HookActivity activity;
    if (s_SwapChain.load() != pSwapChain)
        return s_OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    LOG_INFO("ResizeBuffers invoked: %ux%u", Width, Height);

    {
        std::lock_guard<std::recursive_mutex> lock(Core::Config::Mutex());
        if (s_Context) s_Context->OMSetRenderTargets(0, nullptr, nullptr);
        CleanupRenderTarget();
    }

    HRESULT hr = s_OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    {
        std::lock_guard<std::recursive_mutex> lock(Core::Config::Mutex());
        if (SUCCEEDED(hr) && !s_OverlayStopped) CreateRenderTarget(pSwapChain);
    }
    return hr;
}

HRESULT __stdcall DX11Hook::PresentHook(IDXGISwapChain* swapChain, UINT interval, UINT flags) {
    Core::HookActivity activity;
    IDXGISwapChain* owner = s_SwapChain.load();
    if (owner && owner != swapChain) return s_OriginalPresent(swapChain, interval, flags);
    if (!s_OverlayStopped && !s_Unloading) InitializeGame();
    if (!s_OverlayStopped || (s_RenderCleanupRequested && !s_RenderCleanupDone)) {
        std::lock_guard<std::recursive_mutex> lock(Core::Config::Mutex());
        if (s_RenderCleanupRequested) CleanupOverlay();
        else GuardedRender(swapChain);
    }
    return s_OriginalPresent(swapChain, interval, flags);
}

void DX11Hook::InitializeGame() {
    if (s_GameInitAttempted) return;
    if (!Mono::Bridge::Get().Initialize()) return;
    if (s_GameInitAttempted.exchange(true)) return;
    s_GameReady = Unity::Engine::Get().Initialize() && Game::GameState::Get().Initialize();
    if (s_GameReady) LOG_OK("Game-thread runtime initialized");
    else LOG_ERR("Game initialization failed; features disabled until DLL reload");
}

void DX11Hook::GuardedRender(IDXGISwapChain* swapChain) {
    __try { RenderOverlay(swapChain); }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        LOG_ERR("Overlay disabled after native exception 0x%08X", GetExceptionCode());
        s_OverlayStopped = true;
    }
}

void DX11Hook::RenderOverlay(IDXGISwapChain* pSwapChain) {
    if (!s_ImGuiInitialized) {
        LOG_INFO("First Present hook trigger. Resolving DX11 device & context...");
        DXGI_SWAP_CHAIN_DESC sd{};
        if (FAILED(pSwapChain->GetDesc(&sd)) || !IsWindow(sd.OutputWindow)) return;

        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&s_Device)))) {
            s_Device->GetImmediateContext(&s_Context);

            s_Window = sd.OutputWindow;

            LOG_OK("DX11 Device: %p, Context: %p, Window: %p", s_Device, s_Context, s_Window);

            CreateRenderTarget(pSwapChain);
            if (!s_RenderTargetView) {
                s_Context->Release(); s_Context = nullptr;
                s_Device->Release(); s_Device = nullptr;
                return;
            }

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            UI::Theme::Apply();
            UI::Theme::InitializeAssets(s_Device);
            UI::Menu::Get().Initialize(s_Device, s_Window);

            if (!ImGui_ImplWin32_Init(s_Window) || !ImGui_ImplDX11_Init(s_Device, s_Context) || !InputHook::Initialize(s_Window)) {
                LOG_ERR("Overlay backend initialization failed");
                if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplDX11_Shutdown();
                if (ImGui::GetIO().BackendPlatformUserData) ImGui_ImplWin32_Shutdown();
                UI::Menu::Get().Shutdown();
                UI::Theme::ShutdownAssets();
                ImGui::DestroyContext();
                CleanupRenderTarget();
                s_Context->Release(); s_Context = nullptr;
                s_Device->Release(); s_Device = nullptr;
                s_OverlayStopped = true;
                return;
            }

            s_SwapChain = pSwapChain;
            s_ImGuiInitialized = true;
            LOG_OK("Dear ImGui DX11 overlay initialized successfully!");
        } else {
            LOG_ERR("Failed to get DX11 Device from SwapChain!");
            return;
        }
    }

    bool s_SubsystemsInitialized = s_GameReady.load();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    auto& cfg = Core::Config::Get();

    io.MouseDrawCursor = cfg.menu.isVisible;

    Game::Runtime::Get().SetViewport(io.DisplaySize.x, io.DisplaySize.y);
    if (!s_SubsystemsInitialized && cfg.menu.isVisible) ::ClipCursor(nullptr);
    if (s_SubsystemsInitialized) {
        Features::ESP::Get().Render(ImGui::GetForegroundDrawList(), io.DisplaySize.x, io.DisplaySize.y);
        Features::SpectatorList::Get().Render();
    }

    UI::Menu::Get().Render();

    io.MouseDrawCursor = cfg.menu.isVisible;

    ImGui::Render();
    if (s_RenderTargetView && s_Context) {
        s_Context->OMSetRenderTargets(1, &s_RenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    if (Core::Config::Get().menu.requestUnload) {
        Core::Config::Get().menu.requestUnload = false;
        if (!s_Unloading.exchange(true)) {
            HANDLE worker = CreateThread(nullptr, 0, ShutdownThread, nullptr, 0, nullptr);
            if (worker) CloseHandle(worker);
            else s_Unloading = false;
        }
    }

    return;
}

bool DX11Hook::Initialize() {
    LOG_INFO("Creating dummy DX11 device & SwapChain for VMT resolution...");

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, TEXT("REPO Hook Dummy"), nullptr };
    RegisterClassEx(&wc);
    HWND dummyHwnd = CreateWindow(wc.lpszClassName, TEXT(""), WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    D3D_FEATURE_LEVEL obtainedLevel;
    ID3D11Device* dummyDevice = nullptr;
    ID3D11DeviceContext* dummyContext = nullptr;
    IDXGISwapChain* dummySwapChain = nullptr;

    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = dummyHwnd;
    scd.SampleDesc.Count = 1;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Windowed = TRUE;
    scd.BufferDesc.Width = 1;
    scd.BufferDesc.Height = 1;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        requestedLevels, sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL),
        D3D11_SDK_VERSION, &scd, &dummySwapChain, &dummyDevice, &obtainedLevel, &dummyContext
    );

    if (FAILED(hr)) {
        LOG_ERR("D3D11CreateDeviceAndSwapChain failed! HRESULT: 0x%08X", hr);
        DestroyWindow(dummyHwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    void** pVMT = *reinterpret_cast<void***>(dummySwapChain);
    void* pPresent = pVMT[8];
    void* pResizeBuffers = pVMT[13];

    LOG_OK("Found IDXGISwapChain VTable: %p (Present: %p, ResizeBuffers: %p)", pVMT, pPresent, pResizeBuffers);

    dummySwapChain->Release();
    dummyDevice->Release();
    dummyContext->Release();
    DestroyWindow(dummyHwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    if (MH_Initialize() != MH_OK) {
        LOG_ERR("MH_Initialize failed!");
        return false;
    }

    if (MH_CreateHook(pPresent, &PresentHook, reinterpret_cast<void**>(&s_OriginalPresent)) != MH_OK ||
        MH_CreateHook(pResizeBuffers, &ResizeBuffersHook, reinterpret_cast<void**>(&s_OriginalResizeBuffers)) != MH_OK) {
        LOG_ERR("Failed to create DX11 hooks; rolling back initialization");
        MH_Uninitialize();
        return false;
    }

    if (MH_QueueEnableHook(pPresent) != MH_OK || MH_QueueEnableHook(pResizeBuffers) != MH_OK || MH_ApplyQueued() != MH_OK) {
        LOG_ERR("Failed to enable DX11 hooks; rolling back initialization");
        MH_DisableHook(MH_ALL_HOOKS);
        if (Core::HookActivity::WaitForIdle()) MH_Uninitialize();
        return false;
    }
    LOG_OK("Present and ResizeBuffers hooks enabled");

    s_Hooked = true;
    return true;
}

void DX11Hook::CleanupOverlay() {

    InputHook::Shutdown();
    if (s_ImGuiInitialized) {
        ImGui::GetIO().MouseDrawCursor = false;
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        UI::Menu::Get().Shutdown();
        UI::Theme::ShutdownAssets();
        ImGui::DestroyContext();
        s_ImGuiInitialized = false;
    }
    CleanupRenderTarget();
    if (s_Context) { s_Context->Release(); s_Context = nullptr; }
    if (s_Device) { s_Device->Release(); s_Device = nullptr; }
    s_OverlayStopped = true;
    s_RenderCleanupDone = true;
}

DWORD WINAPI DX11Hook::ShutdownThread(LPVOID) { Shutdown(); return 0; }

void DX11Hook::Shutdown() {
    if (!s_Hooked) { s_Unloading = false; return; }
    if (!Game::Runtime::Get().StopAndWait() || !Features::Removals::Get().RestoreBeforeUnload()) {
        s_Unloading = false;
        return;
    }
    LOG_INFO("Stopping overlay and waiting for hook callbacks to finish...");
    s_RenderCleanupRequested = true;
    for (int i = 0; i < 400 && !s_RenderCleanupDone; ++i) Sleep(5);
    if (!s_RenderCleanupDone) {
        LOG_WARN("Render cleanup was not acknowledged; DLL remains loaded");
        s_Unloading = false;
        return;
    }
    if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK || !Core::HookActivity::WaitForIdle()) {
        LOG_WARN("Hooks are still active; keeping their code and trampolines loaded");
        s_Unloading = false;
        return;
    }
    MH_Uninitialize();
    s_Hooked = false;
    LOG_OK("Hooks stopped and graphics resources released");
    HMODULE module = s_ModuleHandle;
    Core::Logger::Shutdown();
    if (module) FreeLibraryAndExitThread(module, 0);
}

}
