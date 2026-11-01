#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "core/config_store.h"
#include "game/player_bounds.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>

int main() {
    auto& cfg=Core::Config::Global();cfg.ResetToDefaults();
    cfg.godMode.enabled=true;cfg.movement.speedMultiplier=4.25f;cfg.movement.grabThroughWalls=true;cfg.noclip.keyMode=3;
    cfg.fullbright.enabled=true;cfg.fullbright.brightness=2.25f;cfg.removals.noFog=false;
    cfg.player.color[1]=.12345f;cfg.menu.color[0]=37;cfg.menu.toggleKey=VK_F6;
    cfg.visual.crosshairSize=12;cfg.troll.animSpeed=1.75f;
    std::string expected=Core::ConfigStore::Serialize(cfg),error;
    assert(expected.find("schema_version=1")!=std::string::npos);
    assert(expected.find("requestUnload")==std::string::npos);
    assert(expected.find("selectedPlayerAddress")==std::string::npos);
    assert(expected.find("speedActive")==std::string::npos);
    auto roundTrip=cfg;roundTrip.ResetToDefaults();
    assert(Core::ConfigStore::Deserialize(expected,roundTrip,error));
    assert(Core::ConfigStore::Serialize(roundTrip)==expected);
    for(const auto& bad:{"schema_version=2\n", "schema_version=1\nfullbright.brightness=nan\n", "schema_version=1\nfullbright.brightness=999\n", "schema_version=1\nplayer.enabled=2\n", "schema_version=1\nplayer.enabled=1\nplayer.enabled=0\n", "schema_version=1\nunknown=1\n", "schema_version=1\nmenu.toggleKey=99999999999999999999\n", "fullbright.enabled=1\n"}) {
        auto before=Core::ConfigStore::Serialize(roundTrip);
        assert(!Core::ConfigStore::Deserialize(bad,roundTrip,error));assert(!error.empty());
        assert(Core::ConfigStore::Serialize(roundTrip)==before);
    }
    assert(!Core::ConfigStore::Deserialize(std::string(65537,'a'),roundTrip,error));
    for(const auto& name:{"../escape","C:\\escape","CON","com1","bad.ini"," leading","trailing ",""}) assert(!Core::ConfigStore::ValidName(name));
    assert(Core::ConfigStore::ValidName("My profile-1"));
    auto directory=std::filesystem::absolute("build")/("config-test-"+std::to_string(GetCurrentProcessId()));
    Core::ConfigStore store(directory);std::vector<std::string> names;
    assert(store.List(names,error) && names.empty());
    assert(store.Save("roundtrip",false,error));assert(!store.Save("roundtrip",false,error));
    assert(store.List(names,error) && names.size()==1 && names[0]=="roundtrip");
    cfg.ResetToDefaults();cfg.menu.requestUnload=true;cfg.menu.isVisible=false;
    cfg.cosmeticParser.selectedPlayerAddress=1234;cfg.diagnostics.playersFound=7;
    assert(store.Load("roundtrip",error));assert(Core::ConfigStore::Serialize(cfg)==expected);
    assert(cfg.menu.requestUnload && !cfg.menu.isVisible);
    assert(cfg.cosmeticParser.selectedPlayerAddress==1234 && cfg.diagnostics.playersFound==7);
    cfg.fullbright.brightness=1.5f;assert(store.Save("roundtrip",true,error));
    cfg.fullbright.brightness=1;assert(store.Load("roundtrip",error));assert(cfg.fullbright.brightness==1.5f);
    {std::ofstream corrupt(directory/"broken.ini");corrupt<<"schema_version=1\nfullbright.brightness=2\nmenu.toggleKey=oops\n";}
    auto before=Core::ConfigStore::Serialize(cfg);assert(!store.Load("broken",error));assert(Core::ConfigStore::Serialize(cfg)==before);
    assert(store.Delete("broken",error));assert(store.Delete("roundtrip",error));assert(!store.Delete("roundtrip",error));
    assert(store.List(names,error) && names.empty());
    assert(std::filesystem::remove(directory));

    auto project=[](const Unity::Vector3& p,Unity::Vector3& screen) {if(p.z<=0) return false;screen={400+p.x*20,500-p.y*20,p.z};return true;};
    Game::ScreenBounds screen;
    auto bounds=Game::PlayerFallbackBounds({2,10,10});
    assert(Game::ProjectPlayerBounds(bounds,project,screen));
    assert(std::abs(screen.min.y-264)<.01f && std::abs(screen.max.y-300)<.01f);
    assert(std::abs(screen.min.x-433)<.01f && std::abs(screen.max.x-447)<.01f);

    bounds={{2,10.45f,10},{.4f,.45f,.3f}};
    assert(Game::ProjectPlayerBounds(bounds,project,screen));
    assert(std::abs(screen.min.y-282)<.01f && std::abs(screen.max.y-300)<.01f);
    bounds={{2,10.3f,10},{.9f,.3f,.5f}};
    assert(Game::ProjectPlayerBounds(bounds,project,screen));
    assert(screen.max.x-screen.min.x>screen.max.y-screen.min.y);
    bounds=Game::PlayerFallbackBounds({0,0,10});assert(Game::ProjectPlayerBounds(bounds,project,screen));assert(screen.max.y==500);
    bounds={{0,0,.1f},{.3f,.3f,.3f}};auto unchanged=screen;
    assert(!Game::ProjectPlayerBounds(bounds,project,screen));assert(screen.min.x==unchanged.min.x);
    bounds={{0,0,10},{0,0,0}};assert(!Game::ProjectPlayerBounds(bounds,project,screen));
    bounds=Game::PlayerFallbackBounds({0,0,10});
    assert(!Game::ProjectPlayerBounds(bounds,[](const Unity::Vector3&,Unity::Vector3& s){s.x=std::numeric_limits<float>::infinity();return true;},screen));
    auto animatedHeight=[&](Unity::Bounds body,Unity::Bounds head) {
        Game::PlayerBoundsAccumulator parts;
        parts.Add(body);parts.Add(head);
        Unity::Bounds visible;assert(parts.Get(visible));
        Game::ScreenBounds projected;assert(Game::ProjectPlayerBounds(visible,project,projected));
        return projected.max.y-projected.min.y;
    };
    float standing=animatedHeight({{0,.55f,10},{.3f,.55f,.3f}},{{0,1.4f,10},{.3f,.3f,.3f}});
    float crouching=animatedHeight({{0,.3f,10},{.3f,.3f,.3f}},{{0,.8f,10},{.3f,.25f,.3f}});
    float prone=animatedHeight({{0,.2f,10},{.8f,.2f,.3f}},{{.9f,.25f,10},{.25f,.2f,.25f}});
    assert(standing>crouching && crouching>prone);
    Game::PlayerBoundsAccumulator invalidParts;Unity::Bounds ignored;
    invalidParts.Add({{0,0,0},{0,0,0}});assert(!invalidParts.Get(ignored));
    auto capsule=Game::PlayerFallbackBounds({0,0,10});
    Unity::Bounds headFit;
    assert(Game::FitPlayerBoundsToHead(capsule,{0,.9f,10},headFit));
    assert(headFit.extents.y<capsule.extents.y);
    assert(Game::FitPlayerBoundsToHead(capsule,{.8f,.35f,10},headFit));
    assert(headFit.extents.y<.25f && headFit.extents.x>capsule.extents.x);
    assert(!Game::FitPlayerBoundsToHead(capsule,{100,.35f,10},headFit));
    std::cout<<"PASS: config roundtrip, transactional load, validation, file lifecycle, runtime-state exclusion; player bounds, feet anchor, crouch/tumble, projection failures\n";
}
