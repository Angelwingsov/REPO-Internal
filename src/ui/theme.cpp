#include "theme.h"
#include "imgui/imgui.h"
#include <Windows.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace UI {
namespace {
ImFont* textFont = nullptr;
ImFont* iconFont = nullptr;
ImFont* boldFont = nullptr;
ImFont* smallFont = nullptr;
ImFont* legitIconFont = nullptr;
ID3D11ShaderResourceView* background = nullptr;
const void* Resource(int id, DWORD& size) {
    HMODULE module = nullptr;
    size = 0;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Resource), &module)) return nullptr;
    HRSRC res = FindResourceW(module, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10));
    if (!res) return nullptr;
    size = SizeofResource(module, res);
    return LockResource(LoadResource(module, res));
}
}
void Theme::Apply() {

    ImGui::StyleColorsDark();
}
void Theme::InitializeAssets(ID3D11Device* device) {
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    wchar_t windows[MAX_PATH]{};
    GetWindowsDirectoryW(windows, MAX_PATH);
    std::wstring path = std::wstring(windows) + L"\\Fonts\\verdana.ttf";
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        char utf8[MAX_PATH * 3]{};
        WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, utf8, sizeof(utf8), nullptr, nullptr);
        textFont = io.Fonts->AddFontFromFileTTF(utf8, 12, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    }
    path = std::wstring(windows) + L"\\Fonts\\verdanab.ttf";
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        char utf8[MAX_PATH * 3]{};
        WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, utf8, sizeof(utf8), nullptr, nullptr);
        boldFont = io.Fonts->AddFontFromFileTTF(utf8, 12, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    }
    auto embeddedFont = [&](int id, float fontSize) -> ImFont* {
        DWORD bytes = 0;
        const void* font = Resource(id, bytes);
        if (!font || !bytes) return nullptr;
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        return io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(font), static_cast<int>(bytes), fontSize, &cfg);
    };
    iconFont = embeddedFont(202, 47);
    legitIconFont = embeddedFont(203, 32);
    smallFont = embeddedFont(204, 8);
    DWORD size = 0;
    if (!device) return;
    const void* png = Resource(201, size);
    if (!png || !size) return;

    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    {
        using Microsoft::WRL::ComPtr;
        ComPtr<IWICImagingFactory> factory;
        ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> converter;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) &&
            SUCCEEDED(factory->CreateStream(&stream)) &&
            SUCCEEDED(stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(png)), size)) &&
            SUCCEEDED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder)) &&
            SUCCEEDED(decoder->GetFrame(0, &frame)) &&
            SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
            SUCCEEDED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom))) {
            std::vector<BYTE> pixels(4096 * 4096 * 4);
            if (SUCCEEDED(converter->CopyPixels(nullptr, 16384, static_cast<UINT>(pixels.size()), pixels.data()))) {
                D3D11_TEXTURE2D_DESC desc{};
                desc.Width = desc.Height = 4096;
                desc.MipLevels = desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_IMMUTABLE;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                D3D11_SUBRESOURCE_DATA data{pixels.data(), 16384, 0};
                ComPtr<ID3D11Texture2D> texture;
                if (SUCCEEDED(device->CreateTexture2D(&desc, &data, &texture)))
                    device->CreateShaderResourceView(texture.Get(), nullptr, &background);
            }
        }
    }
    if (SUCCEEDED(com)) CoUninitialize();
}
void Theme::ShutdownAssets() {
    if (background) { background->Release(); background = nullptr; }
    textFont = iconFont = boldFont = smallFont = legitIconFont = nullptr;
}
ImFont* Theme::TextFont() { return textFont ? textFont : ImGui::GetFont(); }
ImFont* Theme::IconFont() { return iconFont; }
ImFont* Theme::BoldFont() { return boldFont ? boldFont : TextFont(); }
ImFont* Theme::SmallFont() { return smallFont ? smallFont : TextFont(); }
ImFont* Theme::LegitIconFont() { return legitIconFont ? legitIconFont : IconFont(); }
ID3D11ShaderResourceView* Theme::Background() { return background; }
}
