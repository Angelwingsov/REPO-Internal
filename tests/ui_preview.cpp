

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/original/framework.h"
#include "ui/original/renderer.h"
#include "core/config.h"
#include "game/runtime.h"
#include "features/player.h"
#include "features/removals.h"
#include "hook/input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include "game/entities.h"
#include "core/snapshot.h"
#include "features/esp.h"
#include "unity/unity_engine.h"
using Microsoft::WRL::ComPtr;
namespace Core { void Logger::Log(LogLevel,const char*,...) {} }
namespace Unity {
Engine& Engine::Get(){static Engine e;return e;}
MonoObject* Engine::FindActiveCamera(const char**){return nullptr;}
Vector3 Engine::GetPlayerOrCameraPosition(MonoObject*){return {};}
void Engine::UpdateFrameCameraMetrics(MonoObject*,float,float){}
}
namespace Game {
bool GameState::IsMapLoadingOrRestarting(){return false;}
EntityManager& EntityManager::Get(){static EntityManager e;return e;}
void EntityManager::CollectMonsters(MonoObject*,const Unity::Vector3&,float,float,float,std::vector<MonsterEntity>&){}
void EntityManager::CollectPlayers(MonoObject*,const Unity::Vector3&,float,float,float,bool,std::vector<PlayerEntity>&){}
void EntityManager::CollectValuables(MonoObject*,const Unity::Vector3&,float,float,float,bool,float,std::vector<ValuableEntity>&){}
void EntityManager::CollectItems(MonoObject*,const Unity::Vector3&,float,float,float,std::vector<ItemEntity>&){}
}
namespace Features {
struct ESPRenderTestAccess {
    static void Label(ImDrawList* draw,const ImVec2& center,const char* text) { ESP::Get().DrawTextPill(draw,center,IM_COL32_WHITE,text,0); }
    static void Box(float height) {
        ESP::Frame snapshot;snapshot.available=true;snapshot.timestamp=GetTickCount64();snapshot.width=1080;snapshot.height=780;
        Game::PlayerEntity player;player.hasScreenBox=true;player.screenBoxMin={300,400-height,1};player.screenBoxMax={330,400,1};
        player.topScreenPosition={315,400-height,1};player.bottomScreenPosition={315,400,1};player.screenPosition={315,400-height*.5f,1};
        snapshot.players.push_back(player);ESP::Get().m_Frame.Publish(snapshot);
    }
};
}

namespace Game {
Runtime& Runtime::Get() { static Runtime r; RuntimeState state; state.timestamp=GetTickCount64(); state.hasCosmeticStats=true; state.total=120; state.unlocked=42; state.players.push_back({123,"Preview teammate",false,false,100}); r.m_State.Publish(state); return r; }
bool Runtime::Queue(Action) { return true; }
bool Runtime::TakeResult(Action,bool&) { return false; }
GameState& GameState::Get() { static GameState s; return s; }
bool GameState::QueueCosmeticChange(uintptr_t,bool,bool) { return true; }
bool GameState::TakeCosmeticResult(bool&,std::string&) { return false; }
}
namespace Features { Removals& Removals::Get() { static Removals r; return r; } const char* PlayerMods::GetKeyName(int) { return "G"; } }
namespace Hook { bool InputHook::ConsumeWheelUp() { return false; } bool InputHook::ConsumeWheelDown() { return false; } }
static void Check(HRESULT hr) { assert(SUCCEEDED(hr)); }
bool testKeys[256]{};
short OriginalGuiReadKeyState(int key) { return testKeys[key]?static_cast<short>(0x8000):0; }
int main() {
    Check(CoInitializeEx(nullptr,COINIT_MULTITHREADED));
    std::filesystem::create_directories("build/ui-preview");
    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context;
    Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
    D3D11_TEXTURE2D_DESC desc{}; desc.Width=1080; desc.Height=780; desc.MipLevels=desc.ArraySize=1; desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count=1; desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> target,readback; ComPtr<ID3D11RenderTargetView> rtv;
    Check(device->CreateTexture2D(&desc,nullptr,&target)); Check(device->CreateRenderTargetView(target.Get(),nullptr,&rtv));
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ; Check(device->CreateTexture2D(&desc,nullptr,&readback));
    ImGui::CreateContext(); auto& io=ImGui::GetIO(); io.IniFilename=nullptr;io.DisplaySize=ImVec2(1080,780);io.DeltaTime=1.f/60;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    UI::Theme::Apply();UI::Theme::InitializeAssets(device.Get()); assert(UI::Theme::Background());assert(UI::Theme::IconFont());assert(UI::Theme::TextFont());
    ImGui_ImplDX11_Init(device.Get(),context.Get());
    auto& cfg=Core::Config::Get(); cfg.godMode.enabled=true;cfg.noclip.enabled=true;cfg.movement.speedEnabled=true;cfg.movement.extendGrabRange=true;cfg.movement.superJump=true;cfg.visual.drawCrosshair=true;cfg.troll.overrideAnimSpeed=true;cfg.fullbright.enabled=true;
    UI::Menu::Get().Initialize(device.Get(), nullptr);
    IdaLovesMe::ui::SetNextWindowPos({80,80});
    auto frame=[&] {
        ImGui_ImplDX11_NewFrame();ImGui::NewFrame();UI::Menu::Get().Render();ImGui::Render();
        float clear[4]={.055f,.06f,.07f,1};context->ClearRenderTargetView(rtv.Get(),clear);
        ID3D11RenderTargetView* view=rtv.Get();context->OMSetRenderTargets(1,&view,nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    };
    auto click=[&](float x,float y,bool right=false) {
        io.AddMousePosEvent(x,y);frame();testKeys[right?VK_RBUTTON:VK_LBUTTON]=true;frame();
        testKeys[right?VK_RBUTTON:VK_LBUTTON]=false;frame();frame();
    };
    auto find=[](const char* name) { auto* window=IdaLovesMe::ui::FindWindowByName(name); assert(window);return window; };
    for(int i=0;i<10;++i) frame();

    {
    ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
    ImDrawList textActual(ImGui::GetDrawListSharedData()),textExpected(ImGui::GetDrawListSharedData());
    textActual._ResetForNewFrame();textExpected._ResetForNewFrame();textActual._SetPixelDensity(1);textExpected._SetPixelDensity(1);
    textActual.PushClipRectFullScreen();textExpected.PushClipRectFullScreen();
    const char* label="Player [100 HP]";ImVec2 center(400,200);
    auto* menuFont=UI::Theme::TextFont();assert(menuFont!=ImGui::GetFont());
    auto extent=menuFont->CalcTextSizeA(menuFont->LegacySize,FLT_MAX,0,label);
    Features::ESPRenderTestAccess::Label(&textActual,center,label);
    textExpected.AddText(menuFont,menuFont->LegacySize,ImVec2(center.x-extent.x*.5f,center.y-extent.y*.5f),IM_COL32_WHITE,label);
    assert(textActual.VtxBuffer.Size>0 && textActual.VtxBuffer.Size==textExpected.VtxBuffer.Size);
    for(int i=0;i<textActual.VtxBuffer.Size;++i) {
        const auto& a=textActual.VtxBuffer[i];const auto& b=textExpected.VtxBuffer[i];
        assert(a.pos.x==b.pos.x && a.pos.y==b.pos.y && a.uv.x==b.uv.x && a.uv.y==b.uv.y && a.col==b.col);
    }
    ImGui::Render();
    }

    auto savedCfg=Core::Config::Copy();
    cfg.monster.enabled=cfg.valuable.enabled=cfg.item.enabled=false;cfg.player.enabled=true;
    cfg.player.box2D=true;cfg.player.cornerBox=false;cfg.player.snaplines=false;
    cfg.player.showName=cfg.player.showDistance=cfg.player.showHealth=false;cfg.visual.drawCrosshair=false;
    auto boxHeight=[&](float measuredHeight) {
        ImGui_ImplDX11_NewFrame();ImGui::NewFrame();Features::ESPRenderTestAccess::Box(measuredHeight);ImDrawList rendered(ImGui::GetDrawListSharedData());rendered._ResetForNewFrame();rendered._SetPixelDensity(1);rendered.PushClipRectFullScreen();
        Features::ESP::Get().Render(&rendered,1080,780);
        float low=FLT_MAX,high=-FLT_MAX;for(const auto& v:rendered.VtxBuffer){low=(std::min)(low,v.pos.y);high=(std::max)(high,v.pos.y);}
        assert(rendered.VtxBuffer.Size>0);ImGui::Render();return high-low;
    };
    float tall=boxHeight(40),crouched=boxHeight(20),lying=boxHeight(2);
    assert(tall>crouched && crouched>lying && lying<8);
    Core::Config::Global()=savedCfg;

    assert(UI::Theme::IconFont()->LegacySize==47);
    assert(find("Main")->Size==IdaLovesMe::Vec2(660,560));
    for(int tab=0;tab<8;++tab) {
        click(123,80+19+66*tab+33);frame();frame();
        if(tab==0) {
            auto* panel=find("God mode");
            float keyWidth=Render::Draw->GetTextSize(Render::Fonts::SmallFont,"[G]").x;
            click(panel->Pos.x+panel->Size.x-25-keyWidth*.5f,panel->Pos.y+28,true);
            auto* popup=find("##God");assert(popup->Opened);
            click(popup->Pos.x+15,popup->Pos.y+10);assert(cfg.godMode.keyMode==2);
        }
        if(tab==2) {
            auto* panel=find("Player ESP");
            click(panel->Pos.x+panel->Size.x-32,panel->Pos.y+30);
            auto* picker=find("##PlayerColor");assert(picker->Opened);
            click(picker->Pos.x+50,picker->Pos.y+50);
            click(panel->Pos.x+panel->Size.x-32,panel->Pos.y+30);
            assert(!picker->Opened);
        }
        if(tab==3) {
            auto* removals=find("Removals");
            auto* cameraGroup=find("Camera & screen");

            bool priorShake=cfg.removals.noCameraShake,priorSway=cfg.removals.noCameraNoise;
            click(cameraGroup->Pos.x+30,cameraGroup->Pos.y-10);
            assert(cameraGroup->Opened);
            click(cameraGroup->Pos.x+30,cameraGroup->Pos.y+10);
            assert(cfg.removals.noCameraShake!=priorShake && cfg.removals.noCameraNoise==priorSway);
            assert(cameraGroup->Opened);
            click(cameraGroup->Pos.x+30,cameraGroup->Pos.y+30);
            assert(cfg.removals.noCameraNoise!=priorSway);
            click(removals->Pos.x+20,removals->Pos.y+removals->Size.y-20);
            assert(!cameraGroup->Opened);
            auto* environment=find("Environment");
            bool priorFog=cfg.removals.noFog;
            click(environment->Pos.x+30,environment->Pos.y-10);assert(environment->Opened);
            click(environment->Pos.x+30,environment->Pos.y+10);assert(cfg.removals.noFog!=priorFog);
            click(removals->Pos.x+20,removals->Pos.y+removals->Size.y-20);
            auto* panel=find("Fullbright");
            click(panel->Pos.x+25,panel->Pos.y+28);assert(!cfg.fullbright.enabled);
            click(panel->Pos.x+25,panel->Pos.y+28);assert(cfg.fullbright.enabled);

            float barX=panel->PevCursorPos.x+20;
            float barY=panel->PevCursorPos.y+Render::Draw->GetTextSize(Render::Fonts::Verdana,"Brightness").y+4;
            io.AddMousePosEvent(barX+125,barY);frame();
            testKeys[VK_LBUTTON]=true;frame();frame();testKeys[VK_LBUTTON]=false;frame();
            assert(cfg.fullbright.brightness>1.5f && cfg.fullbright.brightness<=3);
        }
        context->CopyResource(readback.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE map{};Check(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&map));
        ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapEncoder> encoder;ComPtr<IWICBitmapFrameEncode> output;
        Check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)));Check(factory->CreateStream(&stream));
        wchar_t filename[120];swprintf_s(filename,L"build/ui-preview/tab-%d.png",tab);Check(stream->InitializeFromFilename(filename,GENERIC_WRITE));
        Check(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder));Check(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache));Check(encoder->CreateNewFrame(&output,nullptr));Check(output->Initialize(nullptr));Check(output->SetSize(1080,780));
        WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;Check(output->SetPixelFormat(&format));Check(output->WritePixels(780,map.RowPitch,map.RowPitch*780,static_cast<BYTE*>(map.pData)));Check(output->Commit());Check(encoder->Commit());context->Unmap(readback.Get(),0);
    }
    cfg.menu.isVisible=false;for(int i=0;i<10;++i)frame();cfg.menu.isVisible=true;for(int i=0;i<10;++i)frame();

    assert(ImGui::GetForegroundDrawList()->_ClipRectStack.Size==1);
    UI::Menu::Get().Shutdown();assert(!IdaLovesMe::Globals::Gui_Ctx);
    UI::Theme::ShutdownAssets(); assert(!UI::Theme::Background()); ImGui_ImplDX11_Shutdown(); ImGui::DestroyContext();CoUninitialize();
    std::puts("GUI/ESP passed: 8 original tabs, GUI font matches ESP vertices, standing/crouched/prone box heights, controls, clipping, cleanup.");
}
