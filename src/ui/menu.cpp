#include "menu.h"
#include "original/framework.h"
#include "original/renderer.h"
#include "original/bridge.h"
#include "core/config.h"
#include "core/config_store.h"
#include "game/state.h"
#include "game/runtime.h"
#include <algorithm>
#include <string>
#include <vector>
#include <initializer_list>
#include <unordered_map>

using namespace IdaLovesMe;
namespace UI {
namespace {
int currentTab = 0;
float alpha = 0;

void KeyBind(const char* id, int& key, int& mode) {
    int sourceMode = mode == 0 ? 1 : mode == 1 ? 2 : mode == 2 ? 0 : 3;
    int previous = sourceMode;
    ui::KeyBind(id, &key, &sourceMode);
    if (sourceMode != previous) mode = sourceMode == 1 ? 0 : sourceMode == 2 ? 1 : sourceMode == 0 ? 2 : 3;
}
void Color(const char* id, float* rgba) {
    int color[4]; for(int i=0;i<4;++i) color[i]=static_cast<int>(std::clamp(rgba[i],0.f,1.f)*255+.5f);
    int before[4]; std::copy(color,color+4,before);
    ui::ColorPicker(id,color);
    for(int i=0;i<4;++i) if(color[i]!=before[i]) rgba[i]=color[i]/255.f;
}
void Status(const std::string& text) {

    float width=ui::GetWindowSize().x-64;
    std::string line,word;
    auto flush=[&] { if(!line.empty()) {ui::Label(line.c_str()); line.clear();} };
    for(char c:text+" ") {
        if(c==' ' || c=='\n') {
            std::string next=line.empty()?word:line+" "+word;
            if(Render::Draw->GetTextSize(Render::Fonts::Verdana,next.c_str()).x>width) flush();
            if(!word.empty()) {if(!line.empty()) line+=' ';line+=word;word.clear();}
            if(c=='\n') flush();
        } else word+=c;
    }
    flush();
}
void EspOptions(bool& box, bool& corners, bool& lines, bool& name, bool& distance) {
    bool oldBox=box,oldCorners=corners;
    ui::Checkbox("Box",&box);
    ui::Checkbox("Corner box",&corners);
    if(box&&!oldBox) corners=false;
    if(corners&&!oldCorners) box=false;
    ui::Checkbox("Snaplines",&lines);
    ui::Checkbox("Name",&name);
    ui::Checkbox("Distance",&distance);
}
void Configs() {
    auto& store=Core::ConfigStore::Get();
    static std::vector<std::string> names;
    static std::string selected,status,pendingDelete;
    static char name[64]{};
    static bool initialized=false;
    static int listTop=0;
    auto refresh=[&] {
        std::string error;
        if(!store.List(names,error)) status=error;
        if(std::find(names.begin(),names.end(),selected)==names.end()) selected.clear();
    };
    if(!initialized) {refresh();initialized=true;}
    if(ui::BeginListbox("##ConfigList")) {
        auto* list=ui::GetCurrentWindow();
        int last=(std::max)(0,static_cast<int>(names.size())-7);
        if(ui::IsInsideWindow(list)) listTop-=static_cast<int>(Globals::Gui_Ctx->MouseWheel*3);
        listTop=std::clamp(listTop,0,last);
        for(int i=listTop;i<(std::min)(listTop+7,static_cast<int>(names.size()));++i) {
            const auto& entry=names[i];
            if(ui::Selectable(entry.c_str(),entry==selected)) {selected=entry;pendingDelete.clear();}
        }
        if(last>0) {
            int opacity=Globals::Gui_Ctx->MenuAlpha;
            Render::Draw->FilledRect(list->Pos+Vec2(list->Size.x-6,1),Vec2(5,list->Size.y-2),ImColor(40,40,40,opacity));
            float thumb=(list->Size.y-4)*7.f/static_cast<float>(names.size());
            Render::Draw->FilledRect(list->Pos+Vec2(list->Size.x-5,2+(list->Size.y-4-thumb)*listTop/last),Vec2(3,thumb),ImColor(65,65,65,opacity));
        }
        ui::EndListbox();
    }
    ui::InputText("##ConfigName",name);
    if(ui::Button("Refresh")) {refresh();pendingDelete.clear();}
    if(ui::Button("Create")) {
        std::string error;
        if(store.Save(name,false,error)) {selected=name;refresh();status="Config created.";pendingDelete.clear();}
        else status=error;
    }
    auto action=[&](bool load) {
        if(selected.empty()) {status="Select a config first.";return;}
        std::string error;
        bool ok=load?store.Load(selected,error):store.Save(selected,true,error);
        status=ok?(load?"Config loaded.":"Config saved."):error;
        pendingDelete.clear();
    };
    if(ui::Button("Save")) action(false);
    if(ui::Button("Load")) action(true);
    if(ui::Button("Delete")) {
        if(selected.empty()) status="Select a config first.";
        else {pendingDelete=selected;status="Click Confirm delete to remove the selected config.";}
    }
    if(!pendingDelete.empty() && ui::Button("Confirm delete")) {
        std::string error;
        status=store.Delete(pendingDelete,error)?"Config deleted.":error;
        pendingDelete.clear();refresh();
    }
    if(!status.empty()) Status(status);
}
struct RemovalOption {
    const char* label;
    bool* enabled;
};
void RemovalGroup(const char* label,std::initializer_list<RemovalOption> options) {

    std::vector<const char*> names;
    std::unordered_map<int,bool> selected;
    int index=0;
    for(const auto& option:options) {names.push_back(option.label);selected[index++]=*option.enabled;}
    if(ui::MultiSelect(label,&selected,names)) {
        index=0;
        for(const auto& option:options) *option.enabled=selected[index++];
    }
}
void Cosmetics(Core::Config& cfg) {
    auto& gs=Game::GameState::Get();
    std::string result;
    if(gs.TakeCosmeticResult(cfg.cosmeticParser.actionSuccess,result)) {
        cfg.cosmeticParser.hasStatus=true;
        sprintf_s(cfg.cosmeticParser.statusMessage,"%s",result.c_str());
    }
    auto state=Game::Runtime::Get().ReadState();
    if(GetTickCount64()-state.timestamp>1000) state.players.clear();
    std::vector<Game::ServerPlayerInfo> players;
    for(auto& p:state.players) if(!p.isLocal && players.size()<32) players.push_back(p);
    if(players.empty()) {
        cfg.cosmeticParser.selectedPlayerAddress=0;
        cfg.cosmeticParser.selectedPlayerIdx=-1;
        Status("No players detected. Enter a lobby or game.");
    } else {
        std::vector<std::string> names={"Select player"};
        int index=0;
        for(size_t i=0;i<players.size();++i) {
            names.push_back(players[i].name);
            if(players[i].address==cfg.cosmeticParser.selectedPlayerAddress) index=static_cast<int>(i)+1;
        }
        ui::SingleSelect("Target player",&index,names);
        uintptr_t target=index>0?players[index-1].address:0;
        cfg.cosmeticParser.selectedPlayerAddress=target;
        cfg.cosmeticParser.selectedPlayerIdx=index-1;
        auto copy=[&](const char* label,bool cosmetics,bool colors) {
            if(ui::Button(label) && target) {
                bool queued=gs.QueueCosmeticChange(target,cosmetics,colors);
                cfg.cosmeticParser.hasStatus=true;cfg.cosmeticParser.actionSuccess=false;
                sprintf_s(cfg.cosmeticParser.statusMessage,"%s",queued?"Applying on game update...":"Game update unavailable.");
            }
        };
        copy("Copy cosmetics + colors",true,true);
        copy("Copy cosmetics",true,false);
        copy("Copy colors",false,true);
    }
    if(cfg.cosmeticParser.hasStatus) Status(cfg.cosmeticParser.statusMessage);
}
void Unlocks() {
    auto state=Game::Runtime::Get().ReadState();
    if(state.hasCosmeticStats && GetTickCount64()-state.timestamp<1000) {
        char status[80];sprintf_s(status,"Unlocked: %d / %d",state.unlocked,state.total);ui::Label(status);
    } else Status("Enter main menu or lobby to load cosmetics.");
    static std::string status;
    bool success=false;
    if(Game::Runtime::Get().TakeResult(Game::Action::UnlockCosmetics,success)) status=success?"Cosmetics unlocked and saved.":"Unlock failed. Wait for the session.";
    if(ui::Button("Unlock all cosmetics")) status=Game::Runtime::Get().Queue(Game::Action::UnlockCosmetics)?"Applying on game update...":"Game update unavailable.";
    if(!status.empty()) Status(status);
}
}
Menu& Menu::Get() {static Menu instance;return instance;}
void Menu::Initialize(ID3D11Device* device,HWND window) {
    Render::Draw->Init(device,window);
    if(!Globals::Gui_Ctx) {
        ui::CreateContext();
        Globals::Gui_Ctx->ItemSpacing=Vec2(0,6);
        Globals::Gui_Ctx->MenuAlpha=1;
    }
}
void Menu::Shutdown() {
    if(Globals::Gui_Ctx) ui::DeleteContext(Globals::Gui_Ctx);
    DrawList::Drawlist.clear();
    Render::Draw->ReleaseObjects();
    alpha=0;currentTab=0;
}
void Menu::Render() {
    if(!Globals::Gui_Ctx) return;
    auto& cfg=Core::Config::Get();
    float fc=ImGui::GetIO().DeltaTime*255*10;
    if(!cfg.menu.isVisible&&alpha>0) alpha=std::clamp(alpha-fc/255.f,0.f,1.f);
    if(cfg.menu.isVisible&&alpha<1) alpha=std::clamp(alpha+fc/255.f,0.f,1.f);
    Globals::Gui_Ctx->MenuAlpha=static_cast<int>(floor(alpha*255));
    if(!cfg.menu.isVisible&&alpha==0) return;
    Render::Draw->Reset();
    ui::GetInputFromWindow(Render::Draw->GetWindow());
    ImGui::SetNextFrameWantCaptureMouse(cfg.menu.isVisible);
    ImGui::SetNextFrameWantCaptureKeyboard(cfg.menu.isVisible);
    ui::SetNextWindowSize({660,560});
    ui::Begin("Main",GuiFlags_None);
    ui::TabButton("A",&currentTab,0,7);
    ui::TabButton("G",&currentTab,1,7);
    ui::TabButton("C",&currentTab,2,7);
    ui::TabButton("D",&currentTab,3,7);
    ui::TabButton("E",&currentTab,4,7);
    ui::TabButton("F",&currentTab,5,7);
    ui::TabButton("H",&currentTab,6,7);
    ui::TabButton("I",&currentTab,7,7);
    switch(currentTab) {
    case 0:
        ui::BeginChild("God mode",{Vec2(0,0),Vec2(3,10)});
        ui::Checkbox("Enabled",&cfg.godMode.enabled);KeyBind("##God",cfg.godMode.toggleKey,cfg.godMode.keyMode);
        ui::Checkbox("Invulnerability",&cfg.godMode.preventDeath);
        ui::Checkbox("Auto heal",&cfg.godMode.healToMax);
        ui::Checkbox("No fall damage",&cfg.godMode.noFallDamage);
        ui::Checkbox("Auto revive",&cfg.godMode.autoRevive);
        if(ui::Button("Instant full heal")) Game::Runtime::Get().Queue(Game::Action::Heal);
        if(ui::Button("Revive self")) Game::Runtime::Get().Queue(Game::Action::Revive);
        ui::EndChild();
        ui::BeginChild("Movement",{Vec2(6,0),Vec2(3,10)});
        ui::Checkbox("NoClip",&cfg.noclip.enabled);KeyBind("##NoClip",cfg.noclip.toggleKey,cfg.noclip.keyMode);
        ui::SliderFloat("Flight speed",&cfg.noclip.speed,2,60,"%.1f m/s",.5f);
        ui::SliderFloat("Sprint multiplier",&cfg.noclip.sprintMultiplier,1,5,"%.1fx",.1f);
        ui::Checkbox("Through walls",&cfg.noclip.disableColliders);
        ui::Checkbox("Freeze when idle",&cfg.noclip.freezeWhenIdle);
        ui::Checkbox("Speedhack",&cfg.movement.speedEnabled);KeyBind("##Speed",cfg.movement.speedKey,cfg.movement.speedKeyMode);
        ui::Checkbox("Always active",&cfg.movement.speedAlwaysOn);
        ui::SliderFloat("Speed multiplier",&cfg.movement.speedMultiplier,1,8,"%.1fx",.1f);
        ui::EndChild();break;
    case 1:
        ui::BeginChild("Stamina & strength",{Vec2(0,0),Vec2(3,10)});
        ui::Checkbox("Infinite stamina",&cfg.movement.infiniteStamina);
        ui::Checkbox("Infinite strength",&cfg.movement.infiniteStrength);
        ui::SliderFloat("Strength level",&cfg.movement.strengthMultiplier,5,100,"%.0fx",1);
        ui::EndChild();
        ui::BeginChild("Movement utilities",{Vec2(6,0),Vec2(3,5)});
        ui::Checkbox("Extend grab range",&cfg.movement.extendGrabRange);
        ui::SliderFloat("Grab range",&cfg.movement.grabRange,4,30,"%.0f m",1);
        ui::Checkbox("Grab through walls",&cfg.movement.grabThroughWalls);
        ui::Checkbox("Super jump",&cfg.movement.superJump);
        ui::SliderFloat("Jump multiplier",&cfg.movement.jumpMultiplier,1,4,"%.1fx",.1f);
        ui::Checkbox("No ragdoll",&cfg.movement.noTumble);
        ui::EndChild();
        ui::BeginChild("Physics",{Vec2(6,7),Vec2(3,3)});
        ui::Checkbox("Moon gravity",&cfg.troll.featherFall);
        ui::Checkbox("Zero gravity",&cfg.troll.zeroGravity);
        ui::EndChild();break;
    case 2:
        ui::BeginChild("Player ESP",{Vec2(0,0),Vec2(3,6)});
        ui::Checkbox("Enabled",&cfg.player.enabled);Color("##PlayerColor",cfg.player.color);
        EspOptions(cfg.player.box2D,cfg.player.cornerBox,cfg.player.snaplines,cfg.player.showName,cfg.player.showDistance);
        ui::Checkbox("Health",&cfg.player.showHealth);
        ui::Checkbox("Hide local player",&cfg.player.hideLocalPlayer);
        ui::Label("Dead player");Color("##DeadColor",cfg.player.colorDead);
        ui::EndChild();
        ui::BeginChild("General ESP",{Vec2(0,8),Vec2(3,2)});
        {float previous=cfg.monster.maxDistance;float distance=previous;ui::SliderFloat("Maximum distance",&distance,10,500,"%.0f m",1);
        if(distance!=previous) cfg.monster.maxDistance=cfg.player.maxDistance=cfg.valuable.maxDistance=cfg.item.maxDistance=distance;}
        ui::Checkbox("Text background",&cfg.visual.textBackground);
        ui::EndChild();
        ui::BeginChild("Monster ESP",{Vec2(6,0),Vec2(3,3)});
        ui::Checkbox("Enabled",&cfg.monster.enabled);Color("##MonsterColor",cfg.monster.colorDefault);
        EspOptions(cfg.monster.box2D,cfg.monster.cornerBox,cfg.monster.snaplines,cfg.monster.showName,cfg.monster.showDistance);
        ui::Checkbox("Highlight chasing",&cfg.monster.highlightChasing);
        ui::EndChild();
        ui::BeginChild("Items & loot",{Vec2(6,5),Vec2(3,5)});
        {bool enabled=cfg.valuable.enabled||cfg.item.enabled;ui::Checkbox("Enabled",&enabled);cfg.valuable.enabled=cfg.item.enabled=enabled;}
        ui::Checkbox("Name",&cfg.valuable.showName);cfg.item.showName=cfg.valuable.showName;
        ui::Checkbox("Price",&cfg.valuable.showValue);cfg.item.showValue=cfg.valuable.showValue;
        ui::Checkbox("Distance",&cfg.valuable.showDistance);cfg.item.showDistance=cfg.valuable.showDistance;
        ui::Checkbox("Hide extracted",&cfg.valuable.hideExtracted);
        ui::Label("Valuable color");Color("##LootColor",cfg.valuable.color);
        ui::Label("Item color");Color("##ItemColor",cfg.item.color);
        ui::EndChild();break;
    case 3:
        ui::BeginChild("Removals",{Vec2(0,0),Vec2(3,10)});
        ui::Checkbox("Enabled",&cfg.removals.enabled);
        RemovalGroup("Environment",{{"Fog",&cfg.removals.noFog}});
        RemovalGroup("Post-processing",{
            {"All post-processing",&cfg.removals.noPostProcessing},
            {"Film grain",&cfg.removals.noGrain},
            {"Vignette",&cfg.removals.noVignette},
            {"Bloom",&cfg.removals.noBloom},
            {"Motion blur",&cfg.removals.noMotionBlur},
            {"Lens distortion",&cfg.removals.noLensDistortion},
            {"Chromatic aberration",&cfg.removals.noChromaticAberration}});
        RemovalGroup("Camera & screen",{
            {"Shake / bob",&cfg.removals.noCameraShake},
            {"Sway",&cfg.removals.noCameraNoise},
            {"Glitches",&cfg.removals.noCameraGlitch},
            {"Damage overlay",&cfg.removals.noHurtOverlay}});
        ui::EndChild();
        ui::BeginChild("Fullbright",{Vec2(6,0),Vec2(3,4)});
        ui::Checkbox("Enabled",&cfg.fullbright.enabled);
        ui::SliderFloat("Brightness",&cfg.fullbright.brightness,.1f,3,"%.2f",.05f);
        ui::EndChild();
        ui::BeginChild("Crosshair",{Vec2(6,6),Vec2(3,4)});
        ui::Checkbox("Enabled",&cfg.visual.drawCrosshair);Color("##CrosshairColor",cfg.visual.crosshairColor);
        ui::SliderFloat("Size",&cfg.visual.crosshairSize,2,20,"%.0f px",1);
        ui::EndChild();break;
    case 4:
        ui::BeginChild("Cosmetic Parser",{Vec2(0,0),Vec2(3,10)});Cosmetics(cfg);ui::EndChild();
        ui::BeginChild("Cosmetic unlocker",{Vec2(6,0),Vec2(3,10)});Unlocks();ui::EndChild();break;
    case 5:
        ui::BeginChild("Camera",{Vec2(0,0),Vec2(3,10)});
        ui::Checkbox("Third person",&cfg.thirdPerson.enabled);KeyBind("##ThirdPerson",cfg.thirdPerson.toggleKey,cfg.thirdPerson.keyMode);
        ui::SliderFloat("Camera distance",&cfg.thirdPerson.distance,1,6,"%.1f m",.1f);
        ui::SliderFloat("Shoulder offset",&cfg.thirdPerson.shoulderOffset,-1.5f,1.5f,"%.2f m",.05f);
        ui::SliderFloat("Height offset",&cfg.thirdPerson.heightOffset,-.5f,1.5f,"%.2f m",.05f);
        ui::Checkbox("Wall collision",&cfg.thirdPerson.collision);
        if(ui::Button("Right shoulder")){cfg.thirdPerson.distance=2.5f;cfg.thirdPerson.shoulderOffset=.45f;cfg.thirdPerson.heightOffset=.2f;}
        if(ui::Button("Left shoulder")){cfg.thirdPerson.distance=2.5f;cfg.thirdPerson.shoulderOffset=-.45f;cfg.thirdPerson.heightOffset=.2f;}
        if(ui::Button("Centered")){cfg.thirdPerson.distance=2.8f;cfg.thirdPerson.shoulderOffset=0;cfg.thirdPerson.heightOffset=.35f;}
        if(ui::Button("Cinematic wide")){cfg.thirdPerson.distance=4.2f;cfg.thirdPerson.shoulderOffset=.6f;cfg.thirdPerson.heightOffset=.45f;}
        ui::EndChild();
        ui::BeginChild("Spectator list",{Vec2(6,0),Vec2(3,10)});
        ui::Checkbox("Enabled",&cfg.spectatorList.enabled);
        ui::Checkbox("Show watching target",&cfg.spectatorList.showTarget);
        ui::Checkbox("Show view mode",&cfg.spectatorList.showMode);
        ui::Checkbox("Hide when empty",&cfg.spectatorList.hideWhenEmpty);
        ui::Checkbox("Show spectator count",&cfg.spectatorList.showSpectatorCount);
        ui::Label("Header");Color("##SpectatorHeader",cfg.spectatorList.colorHeader);
        ui::Label("Spectator name");Color("##SpectatorName",cfg.spectatorList.colorSpectator);
        ui::Label("Target: you");Color("##TargetMe",cfg.spectatorList.colorTargetMe);
        ui::Label("Target: other");Color("##TargetOther",cfg.spectatorList.colorTargetOther);
        ui::EndChild();break;
    case 6:
        ui::BeginChild("Settings",{Vec2(0,0),Vec2(3,10)});
        ui::Label("Menu key");{int key=static_cast<int>(cfg.menu.toggleKey);ui::KeyBind("##MenuKey",&key,nullptr);cfg.menu.toggleKey=key;}
        ui::Label("Menu color");ui::ColorPicker("##MenuColor",cfg.menu.color);
        if(ui::Button("Reset menu key")) cfg.menu.toggleKey=VK_DELETE;
        if(ui::Button("Reset settings")) cfg.ResetToDefaults();
        if(ui::Button("Unload")) cfg.menu.requestUnload=true;
        ui::EndChild();
        ui::BeginChild("Configs",{Vec2(6,0),Vec2(3,10)});
        Configs();
        ui::EndChild();break;
    case 7:
        ui::BeginChild("Miscellaneous",{Vec2(0,0),Vec2(3,10)});
        ui::Checkbox("Global voice",&cfg.troll.globalVoice);
        ui::Checkbox("Override animation speed",&cfg.troll.overrideAnimSpeed);
        ui::SliderFloat("Animation speed",&cfg.troll.animSpeed,.1f,5,"%.1fx",.1f);
        if(ui::Button("Matrix (0.2x)"))cfg.troll.animSpeed=.2f;
        if(ui::Button("Normal (1.0x)"))cfg.troll.animSpeed=1;
        if(ui::Button("Scooby-Doo (3.0x)"))cfg.troll.animSpeed=3;
        if(ui::Button("Sonic (5.0x)"))cfg.troll.animSpeed=5;
        ui::EndChild();
        ui::BeginChild("Controls",{Vec2(6,0),Vec2(3,10)});
        Status("DELETE / INSERT open the menu. Right-click a keybind for activation modes.");
        ui::EndChild();break;
    }
    ui::SetCurrentWindow(Globals::Gui_Ctx->Windows.front());
    ui::End();
}
}
