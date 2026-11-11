#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "config_store.h"
#include <ShlObj.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace Core {
namespace {
enum class Type { Boolean, Integer, Unsigned, Real };
struct Field { std::string key; void* value; Type type; double min; double max; };
struct Schema {
    std::vector<Field> fields;
    void Add(const char* key, bool& v, double = 0, double = 1) { fields.push_back({key,&v,Type::Boolean,0,1}); }
    void Add(const char* key, int& v, double min, double max) { fields.push_back({key,&v,Type::Integer,min,max}); }
    void Add(const char* key, DWORD& v, double min, double max) { fields.push_back({key,&v,Type::Unsigned,min,max}); }
    void Add(const char* key, float& v, double min, double max) { fields.push_back({key,&v,Type::Real,min,max}); }
};
Schema Bind(Config& c) {
    Schema s;

    s.Add("monster.enabled", c.monster.enabled);
    s.Add("monster.box2D", c.monster.box2D);
    s.Add("monster.cornerBox", c.monster.cornerBox);
    s.Add("monster.snaplines", c.monster.snaplines);
    s.Add("monster.showName", c.monster.showName);
    s.Add("monster.showDistance", c.monster.showDistance);
    s.Add("monster.highlightChasing", c.monster.highlightChasing);
    s.Add("monster.showStunned", c.monster.showStunned);
    s.Add("monster.maxDistance", c.monster.maxDistance, 1, 5000);
    s.Add("monster.boxThickness", c.monster.boxThickness, 0.1, 10);
    s.Add("monster.colorDefault[0]", c.monster.colorDefault[0], 0, 1);
    s.Add("monster.colorDefault[1]", c.monster.colorDefault[1], 0, 1);
    s.Add("monster.colorDefault[2]", c.monster.colorDefault[2], 0, 1);
    s.Add("monster.colorDefault[3]", c.monster.colorDefault[3], 0, 1);
    s.Add("monster.colorChasing[0]", c.monster.colorChasing[0], 0, 1);
    s.Add("monster.colorChasing[1]", c.monster.colorChasing[1], 0, 1);
    s.Add("monster.colorChasing[2]", c.monster.colorChasing[2], 0, 1);
    s.Add("monster.colorChasing[3]", c.monster.colorChasing[3], 0, 1);
    s.Add("monster.colorStunned[0]", c.monster.colorStunned[0], 0, 1);
    s.Add("monster.colorStunned[1]", c.monster.colorStunned[1], 0, 1);
    s.Add("monster.colorStunned[2]", c.monster.colorStunned[2], 0, 1);
    s.Add("monster.colorStunned[3]", c.monster.colorStunned[3], 0, 1);
    s.Add("player.enabled", c.player.enabled);
    s.Add("player.box2D", c.player.box2D);
    s.Add("player.cornerBox", c.player.cornerBox);
    s.Add("player.snaplines", c.player.snaplines);
    s.Add("player.showName", c.player.showName);
    s.Add("player.showDistance", c.player.showDistance);
    s.Add("player.showHealth", c.player.showHealth);
    s.Add("player.hideLocalPlayer", c.player.hideLocalPlayer);
    s.Add("player.maxDistance", c.player.maxDistance, 1, 5000);
    s.Add("player.boxThickness", c.player.boxThickness, 0.1, 10);
    s.Add("player.color[0]", c.player.color[0], 0, 1);
    s.Add("player.color[1]", c.player.color[1], 0, 1);
    s.Add("player.color[2]", c.player.color[2], 0, 1);
    s.Add("player.color[3]", c.player.color[3], 0, 1);
    s.Add("player.colorDead[0]", c.player.colorDead[0], 0, 1);
    s.Add("player.colorDead[1]", c.player.colorDead[1], 0, 1);
    s.Add("player.colorDead[2]", c.player.colorDead[2], 0, 1);
    s.Add("player.colorDead[3]", c.player.colorDead[3], 0, 1);
    s.Add("chams.enabled", c.chams.enabled);
    s.Add("chams.colorVisible[0]", c.chams.colorVisible[0], 0, 1);
    s.Add("chams.colorVisible[1]", c.chams.colorVisible[1], 0, 1);
    s.Add("chams.colorVisible[2]", c.chams.colorVisible[2], 0, 1);
    s.Add("chams.colorVisible[3]", c.chams.colorVisible[3], 0, 1);
    s.Add("chams.colorInvisible[0]", c.chams.colorInvisible[0], 0, 1);
    s.Add("chams.colorInvisible[1]", c.chams.colorInvisible[1], 0, 1);
    s.Add("chams.colorInvisible[2]", c.chams.colorInvisible[2], 0, 1);
    s.Add("chams.colorInvisible[3]", c.chams.colorInvisible[3], 0, 1);
    s.Add("valuable.enabled", c.valuable.enabled);
    s.Add("valuable.showName", c.valuable.showName);
    s.Add("valuable.showValue", c.valuable.showValue);
    s.Add("valuable.showDistance", c.valuable.showDistance);
    s.Add("valuable.hideExtracted", c.valuable.hideExtracted);
    s.Add("valuable.minDollarValue", c.valuable.minDollarValue, 0, 1000000000.0);
    s.Add("valuable.maxDistance", c.valuable.maxDistance, 1, 5000);
    s.Add("valuable.color[0]", c.valuable.color[0], 0, 1);
    s.Add("valuable.color[1]", c.valuable.color[1], 0, 1);
    s.Add("valuable.color[2]", c.valuable.color[2], 0, 1);
    s.Add("valuable.color[3]", c.valuable.color[3], 0, 1);
    s.Add("item.enabled", c.item.enabled);
    s.Add("item.showName", c.item.showName);
    s.Add("item.showValue", c.item.showValue);
    s.Add("item.showDistance", c.item.showDistance);
    s.Add("item.maxDistance", c.item.maxDistance, 1, 5000);
    s.Add("item.color[0]", c.item.color[0], 0, 1);
    s.Add("item.color[1]", c.item.color[1], 0, 1);
    s.Add("item.color[2]", c.item.color[2], 0, 1);
    s.Add("item.color[3]", c.item.color[3], 0, 1);
    s.Add("godMode.enabled", c.godMode.enabled);
    s.Add("godMode.healToMax", c.godMode.healToMax);
    s.Add("godMode.preventDeath", c.godMode.preventDeath);
    s.Add("godMode.noFallDamage", c.godMode.noFallDamage);
    s.Add("godMode.autoRevive", c.godMode.autoRevive);
    s.Add("godMode.toggleKey", c.godMode.toggleKey, 0, 165);
    s.Add("godMode.keyMode", c.godMode.keyMode, 0, 3);
    s.Add("movement.speedEnabled", c.movement.speedEnabled);
    s.Add("movement.speedAlwaysOn", c.movement.speedAlwaysOn);
    s.Add("movement.speedMultiplier", c.movement.speedMultiplier, 1, 8);
    s.Add("movement.speedKey", c.movement.speedKey, 0, 165);
    s.Add("movement.speedKeyMode", c.movement.speedKeyMode, 0, 3);
    s.Add("movement.infiniteStamina", c.movement.infiniteStamina);
    s.Add("movement.infiniteStrength", c.movement.infiniteStrength);
    s.Add("movement.strengthMultiplier", c.movement.strengthMultiplier, 5, 100);
    s.Add("movement.extendGrabRange", c.movement.extendGrabRange);
    s.Add("movement.grabRange", c.movement.grabRange, 4, 30);
    s.Add("movement.grabThroughWalls", c.movement.grabThroughWalls);
    s.Add("movement.superJump", c.movement.superJump);
    s.Add("movement.jumpMultiplier", c.movement.jumpMultiplier, 1, 4);
    s.Add("movement.noTumble", c.movement.noTumble);
    s.Add("noclip.enabled", c.noclip.enabled);
    s.Add("noclip.toggleKey", c.noclip.toggleKey, 0, 165);
    s.Add("noclip.keyMode", c.noclip.keyMode, 0, 3);
    s.Add("noclip.speed", c.noclip.speed, 2, 60);
    s.Add("noclip.sprintMultiplier", c.noclip.sprintMultiplier, 1, 5);
    s.Add("noclip.freezeWhenIdle", c.noclip.freezeWhenIdle);
    s.Add("noclip.disableColliders", c.noclip.disableColliders);
    s.Add("noclip.verticalFollowCam", c.noclip.verticalFollowCam);
    s.Add("thirdPerson.enabled", c.thirdPerson.enabled);
    s.Add("thirdPerson.toggleKey", c.thirdPerson.toggleKey, 0, 165);
    s.Add("thirdPerson.keyMode", c.thirdPerson.keyMode, 0, 3);
    s.Add("thirdPerson.distance", c.thirdPerson.distance, 1, 6);
    s.Add("thirdPerson.shoulderOffset", c.thirdPerson.shoulderOffset, -1.5, 1.5);
    s.Add("thirdPerson.heightOffset", c.thirdPerson.heightOffset, -0.5, 1.5);
    s.Add("thirdPerson.collision", c.thirdPerson.collision);
    s.Add("spectatorList.enabled", c.spectatorList.enabled);
    s.Add("spectatorList.showTarget", c.spectatorList.showTarget);
    s.Add("spectatorList.showMode", c.spectatorList.showMode);
    s.Add("spectatorList.onlySpectatingMe", c.spectatorList.onlySpectatingMe);
    s.Add("spectatorList.showSpectatorCount", c.spectatorList.showSpectatorCount);
    s.Add("troll.globalVoice", c.troll.globalVoice);
    s.Add("troll.zeroGravity", c.troll.zeroGravity);
    s.Add("troll.featherFall", c.troll.featherFall);
    s.Add("troll.overrideAnimSpeed", c.troll.overrideAnimSpeed);
    s.Add("troll.animSpeed", c.troll.animSpeed, 0.1, 5);
    s.Add("removals.enabled", c.removals.enabled);
    s.Add("removals.noFog", c.removals.noFog);
    s.Add("removals.noGrain", c.removals.noGrain);
    s.Add("removals.noVignette", c.removals.noVignette);
    s.Add("removals.noBloom", c.removals.noBloom);
    s.Add("removals.noMotionBlur", c.removals.noMotionBlur);
    s.Add("removals.noLensDistortion", c.removals.noLensDistortion);
    s.Add("removals.noChromaticAberration", c.removals.noChromaticAberration);
    s.Add("removals.noCameraShake", c.removals.noCameraShake);
    s.Add("removals.noCameraNoise", c.removals.noCameraNoise);
    s.Add("removals.noCameraGlitch", c.removals.noCameraGlitch);
    s.Add("removals.noHurtOverlay", c.removals.noHurtOverlay);
    s.Add("removals.noPostProcessing", c.removals.noPostProcessing);
    s.Add("fullbright.enabled", c.fullbright.enabled);
    s.Add("fullbright.brightness", c.fullbright.brightness, 0.1, 3);
    s.Add("visual.category", c.visual.category, 0, 3);
    s.Add("visual.textBackground", c.visual.textBackground);
    s.Add("visual.textBgAlpha", c.visual.textBgAlpha, 0, 1);
    s.Add("visual.drawCrosshair", c.visual.drawCrosshair);
    s.Add("visual.crosshairSize", c.visual.crosshairSize, 2, 20);
    s.Add("visual.crosshairColor[0]", c.visual.crosshairColor[0], 0, 1);
    s.Add("visual.crosshairColor[1]", c.visual.crosshairColor[1], 0, 1);
    s.Add("visual.crosshairColor[2]", c.visual.crosshairColor[2], 0, 1);
    s.Add("visual.crosshairColor[3]", c.visual.crosshairColor[3], 0, 1);
    s.Add("visual.snaplineThickness", c.visual.snaplineThickness, 0.1, 10);
    s.Add("menu.toggleKey", c.menu.toggleKey, 0, 165);
    s.Add("menu.blockInputWhenOpen", c.menu.blockInputWhenOpen);
    s.Add("menu.color[0]", c.menu.color[0], 0, 255);
    s.Add("menu.color[1]", c.menu.color[1], 0, 255);
    s.Add("menu.color[2]", c.menu.color[2], 0, 255);
    s.Add("menu.color[3]", c.menu.color[3], 0, 255);
    s.Add("spawner.selectedCategory", c.spawner.selectedCategory, 0, 10);
    s.Add("spawner.selectedItemIndex", c.spawner.selectedItemIndex, 0, 1000);
    s.Add("spawner.spawnAmount", c.spawner.spawnAmount, 1, 20);
    s.Add("spawner.spawnDistance", c.spawner.spawnDistance, 0.5, 20.0);
    s.Add("spawner.spawnAtCrosshair", c.spawner.spawnAtCrosshair);
    s.Add("spawner.spawnMode", c.spawner.spawnMode, 0, 3);
    s.Add("spawner.hotkey", c.spawner.hotkey, 0, 165);
    s.Add("spawner.hotkeyMode", c.spawner.hotkeyMode, 0, 3);
    s.Add("antiAim.enabled", c.antiAim.enabled);
    s.Add("antiAim.toggleKey", c.antiAim.toggleKey, 0, 165);
    s.Add("antiAim.keyMode", c.antiAim.keyMode, 0, 3);
    s.Add("antiAim.yawMode", c.antiAim.yawMode, 0, 5);
    s.Add("antiAim.spinSpeed", c.antiAim.spinSpeed, 10, 3600);
    s.Add("antiAim.jitterRange", c.antiAim.jitterRange, 10, 180);
    s.Add("antiAim.pitchMode", c.antiAim.pitchMode, 0, 5);
    s.Add("antiAim.rollMode", c.antiAim.rollMode, 0, 3);
    s.Add("antiAim.leanAngle", c.antiAim.leanAngle, -85, 85);
    s.Add("animBreaker.enabled", c.animBreaker.enabled);
    s.Add("animBreaker.slideWalk", c.animBreaker.slideWalk);
    s.Add("animBreaker.staticLegs", c.animBreaker.staticLegs);
    s.Add("animBreaker.forceCrawl", c.animBreaker.forceCrawl);
    s.Add("animBreaker.poseSpammer", c.animBreaker.poseSpammer);
    s.Add("animBreaker.poseSpamSpeed", c.animBreaker.poseSpamSpeed, 1, 60);
    s.Add("animBreaker.headBreaker", c.animBreaker.headBreaker);
    s.Add("animBreaker.headSpinSpeed", c.animBreaker.headSpinSpeed, 10, 3600);
    s.Add("animBreaker.spineBreaker", c.animBreaker.spineBreaker);
    s.Add("animBreaker.spineTwitchAmount", c.animBreaker.spineTwitchAmount, 5, 90);
    return s;
}
std::string Trim(std::string s) {
    auto first=s.find_first_not_of(" \t\r");
    if(first==std::string::npos) return {};
    return s.substr(first,s.find_last_not_of(" \t\r")-first+1);
}
std::string Read(const Field& f) {
    std::ostringstream out;out.imbue(std::locale::classic());
    switch(f.type) {
    case Type::Boolean:out<<(*static_cast<bool*>(f.value)?1:0);break;
    case Type::Integer:out<<*static_cast<int*>(f.value);break;
    case Type::Unsigned:out<<*static_cast<DWORD*>(f.value);break;
    case Type::Real:out<<std::setprecision(std::numeric_limits<float>::max_digits10)<<*static_cast<float*>(f.value);break;
    }
    return out.str();
}
bool Write(const Field& f,const std::string& text) {
    double number=0;
    if(f.type==Type::Real) {
        std::istringstream input(text);input.imbue(std::locale::classic());input>>std::noskipws>>number;
        if(!input || !input.eof()) return false;
    } else {
        long long value=0;auto result=std::from_chars(text.data(),text.data()+text.size(),value);
        if(result.ec!=std::errc{} || result.ptr!=text.data()+text.size()) return false;
        number=static_cast<double>(value);
    }
    if(!std::isfinite(number) || number<f.min || number>f.max) return false;
    switch(f.type) {
    case Type::Boolean:*static_cast<bool*>(f.value)=number!=0;break;
    case Type::Integer:*static_cast<int*>(f.value)=static_cast<int>(number);break;
    case Type::Unsigned:*static_cast<DWORD*>(f.value)=static_cast<DWORD>(number);break;
    case Type::Real:*static_cast<float*>(f.value)=static_cast<float>(number);break;
    }
    return true;
}
std::filesystem::path DefaultDirectory() {
    PWSTR path=nullptr;
    if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&path))) return {};
    std::filesystem::path result=std::filesystem::path(path)/L"REPO-Internal"/L"configs";
    CoTaskMemFree(path);return result;
}
bool Ready(const std::filesystem::path& path,const std::string& name,std::string& error) {
    error.clear();
    if(path.empty()) {error="Config directory is unavailable.";return false;}
    if(!ConfigStore::ValidName(name)) {error="Use 1-48 letters, digits, spaces, _ or -.";return false;}
    return true;
}
}
ConfigStore::ConfigStore(std::filesystem::path directory):m_Directory(std::move(directory)) {}
ConfigStore& ConfigStore::Get() {static ConfigStore store(DefaultDirectory());return store;}
bool ConfigStore::ValidName(const std::string& name) {
    if(name.empty() || name.size()>48 || name.front()==' ' || name.back()==' ') return false;
    for(unsigned char c:name) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c==' ')) return false;
    std::string upper=name;for(char& c:upper) if(c>='a'&&c<='z') c-=32;
    if(upper=="CON"||upper=="PRN"||upper=="AUX"||upper=="NUL") return false;
    if(upper.size()==4 && (upper.substr(0,3)=="COM"||upper.substr(0,3)=="LPT") && upper[3]>='1'&&upper[3]<='9') return false;
    return true;
}
std::string ConfigStore::Serialize(Config& config) {
    std::string text="; R.E.P.O Internal configuration\nschema_version=1\n";
    for(const auto& f:Bind(config).fields) text+=f.key+"="+Read(f)+"\n";
    return text;
}
bool ConfigStore::Deserialize(const std::string& text,Config& config,std::string& error) {
    error.clear();if(text.size()>65536) {error="Config is too large.";return false;}
    auto candidate=config;auto schema=Bind(candidate);
    std::map<std::string,Field*> fields;for(auto& f:schema.fields) fields.emplace(f.key,&f);
    std::set<std::string> seen;std::istringstream input(text);std::string line;bool version=false;
    while(std::getline(input,line)) {
        line=Trim(line);if(line.empty()||line[0]==';'||line[0]=='#') continue;
        auto split=line.find('=');if(split==std::string::npos) {error="Invalid config line.";return false;}
        auto key=Trim(line.substr(0,split)),value=Trim(line.substr(split+1));
        if(!seen.insert(key).second) {error="Duplicate config setting: "+key;return false;}
        if(key=="schema_version") {if(value!="1") {error="Unsupported config version.";return false;}version=true;continue;}
        auto field=fields.find(key);
        if(field==fields.end() || !Write(*field->second,value)) {error="Invalid config setting: "+key;return false;}
    }
    if(!version) {error="Missing config version.";return false;}
    config=std::move(candidate);return true;
}
bool ConfigStore::List(std::vector<std::string>& names,std::string& error) const {
    names.clear();error.clear();if(m_Directory.empty()) {error="Config directory is unavailable.";return false;}
    std::error_code ec;
    if(!std::filesystem::exists(m_Directory,ec)) {if(ec) error="Cannot read config directory.";return !ec;}
    std::filesystem::directory_iterator it(m_Directory,ec),end;
    while(!ec && it!=end) {
        const auto& entry=*it;std::error_code typeError;
        if(entry.is_regular_file(typeError) && !typeError && entry.path().extension()==L".ini") {
            auto name=entry.path().stem().u8string();if(ValidName(name)) names.push_back(name);
        }
        it.increment(ec);
    }
    if(ec) {error="Cannot read config directory.";names.clear();return false;}
    std::sort(names.begin(),names.end());return true;
}
bool ConfigStore::Save(const std::string& name,bool overwrite,std::string& error) const {
    if(!Ready(m_Directory,name,error)) return false;
    auto config=Config::Copy();auto contents=Serialize(config);
    if(!Deserialize(contents,config,error)) return false;
    std::error_code ec;std::filesystem::create_directories(m_Directory,ec);
    if(ec) {error="Cannot create config directory.";return false;}
    auto target=m_Directory/(name+".ini");
    auto temp=target;temp+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    HANDLE file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) {error="Cannot create config file.";return false;}
    DWORD written=0;bool ok=WriteFile(file,contents.data(),static_cast<DWORD>(contents.size()),&written,nullptr)&&written==contents.size();
    if(ok) ok=FlushFileBuffers(file)!=FALSE;CloseHandle(file);
    if(ok) ok=MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH|(overwrite?MOVEFILE_REPLACE_EXISTING:0))!=FALSE;
    if(!ok) {DeleteFileW(temp.c_str());error=overwrite?"Cannot save config.":"Cannot create config; this name may already exist.";return false;}
    return true;
}
bool ConfigStore::Load(const std::string& name,std::string& error) const {
    if(!Ready(m_Directory,name,error)) return false;
    auto path=m_Directory/(name+".ini");std::error_code ec;auto size=std::filesystem::file_size(path,ec);
    if(ec||size>65536) {error="Cannot read config or file is too large.";return false;}
    std::ifstream file(path,std::ios::binary);std::string text(static_cast<size_t>(size),'\0');
    if(!file.read(text.data(),static_cast<std::streamsize>(size))) {error="Cannot read config.";return false;}
    auto candidate=Config::Copy();if(!Deserialize(text,candidate,error)) return false;
    std::lock_guard<std::recursive_mutex> lock(Config::Mutex());
    auto destination=Bind(Config::Global()),source=Bind(candidate);
    for(size_t i=0;i<source.fields.size();++i) Write(destination.fields[i],Read(source.fields[i]));
    return true;
}
bool ConfigStore::Delete(const std::string& name,std::string& error) const {
    if(!Ready(m_Directory,name,error)) return false;
    if(!DeleteFileW((m_Directory/(name+".ini")).c_str())) {error="Cannot delete config.";return false;}
    return true;
}
}
