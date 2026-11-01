
#include "../src/features/removals.cpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_set>

namespace {
struct CameraNode {void* header[2]{};float strength=4;float tuning=7;Unity::Vector3 position;std::array<float,4> rotation{0,0,0,1};};
std::unordered_set<void*> nodes;
int positionMethod=1,rotationMethod=2,updates=0;
void GameUpdate(void* object) {
    auto& node=*static_cast<CameraNode*>(object);
    ++updates;node.position={.1f,.2f,.3f};node.rotation={.1f,.2f,.3f,.9f};
}
}
namespace Mono {
Bridge& Bridge::Get(){static Bridge b;return b;}
bool Bridge::AttachCurrentThread(){return true;}
uint32_t Bridge::PinObject(MonoObject* o){return o?1:0;}
void Bridge::FreePin(uint32_t){}
MonoObject* Bridge::InvokeMethod(MonoMethod*,void*,void**,MonoObject**){return nullptr;}
void* Bridge::SafeUnboxRaw(MonoObject* value){return value;}
bool Bridge::TryInvokeMethod(MonoMethod* method,void* object,void** args){
    if(!object||!args) return false;
    auto& node=*static_cast<CameraNode*>(object);
    if(method==&positionMethod){std::memcpy(&node.position,args[0],sizeof(node.position));return true;}
    if(method==&rotationMethod){std::memcpy(node.rotation.data(),args[0],sizeof(node.rotation));return true;}
    return false;
}
MonoClass* Bridge::FindClass(MonoImage*,const char*,const char*){return nullptr;}
MonoMethod* Bridge::FindMethod(MonoClass*,const char*,int){return nullptr;}
int Bridge::GetFieldOffset(MonoClass*,const char*,int fallback){return fallback;}
}
namespace Unity {
Engine& Engine::Get(){static Engine e;return e;}
bool Engine::IsValidUnityObject(void* object){return nodes.count(object)>0;}
MonoObject* Engine::GetTransform(MonoObject* object){return IsValidUnityObject(object)?object:nullptr;}
}
namespace Memory {
bool SafeCopyMemory(void* dst,const void* src,size_t size){if(!dst||!src)return false;std::memcpy(dst,src,size);return true;}
}
namespace Core {void Logger::Log(LogLevel,const char*,...) {}}
MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){return MH_ERROR_NOT_INITIALIZED;}
MH_STATUS WINAPI MH_EnableHook(LPVOID){return MH_ERROR_NOT_INITIALIZED;}
MH_STATUS WINAPI MH_RemoveHook(LPVOID){return MH_ERROR_NOT_INITIALIZED;}
int main(){
    using namespace Features;
    CameraNode bob,shake;nodes.insert(&bob);nodes.insert(&shake);
    bobUpdate=shakeUpdate=GameUpdate;
    effectLocalPositionSet=&positionMethod;effectLocalRotationSet=&rotationMethod;
    shakeOffsets.fill(-1);shakeOffsets[0]=offsetof(CameraNode,strength);
    auto& config=Core::Config::Get().removals;
    config.enabled=true;config.noCameraShake=true;
    BobHook(&bob);ShakeHook(&shake);
    assert(updates==2);
    assert(bob.position.IsZero() && shake.position.IsZero());
    assert(bob.rotation[0]==0 && bob.rotation[3]==1 && shake.rotation[3]==1);
    assert(shake.strength==0 && shake.tuning==7 && bob.tuning==7);
    config.noCameraShake=false;BobHook(&bob);ShakeHook(&shake);
    assert(bob.position.x==.1f && shake.position.x==.1f);
    config.noCameraShake=true;config.enabled=false;BobHook(&bob);ShakeHook(&shake);
    assert(bob.position.x==.1f && shake.position.x==.1f);
    config.enabled=true;BobHook(&bob);assert(bob.position.IsZero());
    stopping=true;BobHook(&bob);ShakeHook(&shake);
    assert(bob.position.x==.1f && shake.position.x==.1f && restored);
    stopping=false;
    nodes.erase(&bob);BobHook(&bob);assert(bob.position.x==.1f);
    std::cout<<"PASS: walking bob and impact shake suppressed, original updates retained, toggles/unload restore normal behavior\n";
}
