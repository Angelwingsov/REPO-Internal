#include "dx11hook.h"
#include "input.h"
#include "core/config.h"
#include "core/logger.h"
#include "mono/mono_bridge.h"
#include "unity/unity_engine.h"
#include "game/state.h"
#include "features/esp.h"
#include "features/godmode.h"
#include "features/noclip.h"
#include "features/player.h"
#include "features/thirdperson.h"
#include "features/spectatorlist.h"
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
    LOG_INFO("ResizeBuffers invoked: %ux%u", Width, Height);

    if (s_Context) {
        s_Context->OMSetRenderTargets(0, nullptr, nullptr);
    }
    CleanupRenderTarget();

    HRESULT hr = s_OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    CreateRenderTarget(pSwapChain);
    return hr;
}

HRESULT __stdcall DX11Hook::PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!s_ImGuiInitialized) {
        LOG_INFO("First Present hook trigger. Resolving DX11 device & context...");

        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&s_Device)))) {
            s_Device->GetImmediateContext(&s_Context);

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            s_Window = sd.OutputWindow;

            LOG_OK("DX11 Device: %p, Context: %p, Window: %p", s_Device, s_Context, s_Window);

            CreateRenderTarget(pSwapChain);
            InputHook::Initialize(s_Window);

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 6.0f;
            style.FrameRounding = 4.0f;

            ImGui_ImplWin32_Init(s_Window);
            ImGui_ImplDX11_Init(s_Device, s_Context);

            s_ImGuiInitialized = true;
            LOG_OK("Dear ImGui DX11 overlay initialized successfully!");
        } else {
            LOG_ERR("Failed to get DX11 Device from SwapChain!");
            return s_OriginalPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    static bool s_SubsystemsInitialized = false;
    if (!s_SubsystemsInitialized) {
        if (Mono::Bridge::Get().Initialize()) {
            Unity::Engine::Get().Initialize();
            Game::GameState::Get().Initialize();
            s_SubsystemsInitialized = true;
            LOG_OK("All game introspection subsystems connected!");
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    auto& cfg = Core::Config::Get();

    io.MouseDrawCursor = cfg.menu.isVisible;

    if (s_SubsystemsInitialized) {
        Unity::Engine::Get().UpdateCursorState(cfg.menu.isVisible);
    } else if (cfg.menu.isVisible) {
        ::ClipCursor(nullptr);
    }

    if (s_SubsystemsInitialized) {
        __try {
            Features::GodMode::Get().Update();
            Features::NoClip::Get().Update();
            Features::PlayerMods::Get().Update();
            Features::ThirdPerson::Get().Update();
            Features::SpectatorList::Get().Update();
            Features::ESP::Get().Render(ImGui::GetForegroundDrawList(), io.DisplaySize.x, io.DisplaySize.y);
            Features::SpectatorList::Get().Render();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            static bool s_LoggedOnce = false;
            if (!s_LoggedOnce) {
                LOG_ERR("Exception in Features Update/Render (Code: 0x%08X)", GetExceptionCode());
                s_LoggedOnce = true;
            }
        }
    }

    UI::Menu::Get().Render();

    io.MouseDrawCursor = cfg.menu.isVisible;
    if (s_SubsystemsInitialized) {
        Unity::Engine::Get().UpdateCursorState(cfg.menu.isVisible);
    }

    ImGui::Render();
    if (s_RenderTargetView && s_Context) {
        s_Context->OMSetRenderTargets(1, &s_RenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    if (Core::Config::Get().menu.requestUnload) {
        Core::Config::Get().menu.requestUnload = false;
        LOG_INFO("Unload requested by user. Spawning eject thread...");
        CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Shutdown), nullptr, 0, nullptr);
    }

    return s_OriginalPresent(pSwapChain, SyncInterval, Flags);
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

    if (MH_CreateHook(pPresent, &PresentHook, reinterpret_cast<void**>(&s_OriginalPresent)) != MH_OK) {
        LOG_ERR("Failed to create hook for Present!");
        return false;
    }
    if (MH_EnableHook(pPresent) != MH_OK) {
        LOG_ERR("Failed to enable hook for Present!");
        return false;
    }
    LOG_OK("Present hooked successfully!");

    if (MH_CreateHook(pResizeBuffers, &ResizeBuffersHook, reinterpret_cast<void**>(&s_OriginalResizeBuffers)) != MH_OK) {
        LOG_ERR("Failed to create hook for ResizeBuffers!");
        return false;
    }
    if (MH_EnableHook(pResizeBuffers) != MH_OK) {
        LOG_ERR("Failed to enable hook for ResizeBuffers!");
        return false;
    }
    LOG_OK("ResizeBuffers hooked successfully!");

    s_Hooked = true;
    return true;
}

void DX11Hook::Shutdown() {
    if (!s_Hooked) return;
    s_Hooked = false;

    LOG_INFO("Detaching cheat hooks and restoring state...");

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    LOG_OK("MinHook uninitialized!");

    InputHook::Shutdown();

    if (s_ImGuiInitialized) {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        Unity::Engine::Get().UpdateCursorState(false);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        s_ImGuiInitialized = false;
        LOG_OK("ImGui context destroyed!");
    }

    CleanupRenderTarget();
    if (s_Context) { s_Context->Release(); s_Context = nullptr; }
    if (s_Device)  { s_Device->Release();  s_Device = nullptr;  }

    LOG_OK("Cheat detached cleanly! Goodbye.");

    Sleep(1000);
    HMODULE hMod = s_ModuleHandle;
    Core::Logger::Shutdown();

    if (hMod) {
        FreeLibraryAndExitThread(hMod, 0);
    }
}

}
