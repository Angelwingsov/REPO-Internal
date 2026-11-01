#include "removals.h"
#include "core/config.h"
#include "core/logger.h"
#include "core/hook_activity.h"
#include "memory/memory.h"
#include "mono/mono_bridge.h"
#include "unity/unity_engine.h"
#include "minhook/include/MinHook.h"
#include <array>
#include <atomic>
#include <vector>

namespace Features {
namespace {
using UpdateMethod = void (*)(void*);
using CompileMethod = void* (*)(MonoMethod*);
using ObjectClass = MonoClass* (*)(MonoObject*);
using ParentClass = MonoClass* (*)(MonoClass*);
CompileMethod compileMethod = nullptr;
ObjectClass objectClass = nullptr;
ParentClass parentClass = nullptr;
UpdateMethod environmentUpdate = nullptr, postUpdate = nullptr, shakeUpdate = nullptr;
UpdateMethod noiseUpdate = nullptr, glitchUpdate = nullptr, hurtUpdate = nullptr;
UpdateMethod bobUpdate = nullptr;
MonoMethod* effectLocalPositionSet = nullptr;
MonoMethod* effectLocalRotationSet = nullptr;
std::atomic<bool> stopping{false}, restored{false}, overridesActive{false};

bool Valid(uintptr_t object) {
    return Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(object));
}
int Field(MonoClass* klass, const char* name) {
    auto& bridge = Mono::Bridge::Get();
    for (auto* current = klass; current; current = parentClass ? parentClass(current) : nullptr) {
        int offset = bridge.GetFieldOffset(current, name, -1);
        if (offset > 0) return offset;
    }
    return -1;
}

template<typename T> struct Property {
    MonoMethod* get = nullptr;
    MonoMethod* set = nullptr;
    T original{};
    bool applied = false;
    bool Read(T& value) {
        auto& bridge = Mono::Bridge::Get();
        MonoObject* exc = nullptr;
        MonoObject* result = bridge.InvokeMethod(get, nullptr, nullptr, &exc);
        return result && !exc && bridge.Unbox(result, value);
    }
    bool Write(T value) {
        void* args[] = { &value };
        return Mono::Bridge::Get().TryInvokeMethod(set, nullptr, args);
    }
    void Restore() {
        if (applied) Write(original);
        applied = false;
    }
    void Apply(T value) {
        if (get && set && Read(original)) applied = Write(value);
    }
};
Property<bool> fog;
Property<int> ambientMode;
Property<Unity::Color> ambientLight;
Property<float> ambientIntensity;
uintptr_t environmentOwner = 0;

struct FieldOverride {
    Mono::PinnedObject pin;
    uintptr_t object = 0;
    int offset = -1;
    std::array<unsigned char, 4> original{};
    size_t size = 0;
    void Restore() const {
        if (Valid(object)) Memory::SafeCopyMemory(reinterpret_cast<void*>(object + offset), original.data(), size);
    }
};
std::vector<FieldOverride> postOverrides;
template<typename T> void OverrideField(uintptr_t object, int offset, T value) {
    static_assert(sizeof(T) <= 4);
    if (!Valid(object) || offset <= 0) return;
    FieldOverride saved;
    saved.pin = Mono::PinnedObject(reinterpret_cast<MonoObject*>(object));
    if (!saved.pin) return;
    saved.object = object;
    saved.offset = offset;
    saved.size = sizeof(T);
    if (Memory::SafeCopyMemory(saved.original.data(), reinterpret_cast<void*>(object + offset), sizeof(T)) &&
        Memory::Write(object + offset, value)) postOverrides.push_back(std::move(saved));
}

MonoMethod* behaviourGet = nullptr;
MonoMethod* behaviourSet = nullptr;
MonoMethod* activeGet = nullptr;
MonoMethod* activeSet = nullptr;
struct ObjectSwitch {
    Mono::PinnedObject pin;
    uintptr_t object = 0;
    bool original = false;
    bool applied = false;
    MonoMethod* setter = nullptr;
    void Restore() {
        if (applied && Valid(object)) {
            void* args[] = { &original };
            Mono::Bridge::Get().TryInvokeMethod(setter, reinterpret_cast<void*>(object), args);
        }
        applied = false;
        pin.Reset();
    }
    void Disable(uintptr_t target, MonoMethod* get, MonoMethod* set) {
        if (!Valid(target) || !get || !set) return;
        auto& bridge = Mono::Bridge::Get();
        if (applied && object == target) {
            bool disabled = false;
            void* args[] = { &disabled };
            bridge.TryInvokeMethod(set, reinterpret_cast<void*>(target), args);
            return;
        }
        Restore();
        Mono::PinnedObject hold(reinterpret_cast<MonoObject*>(target));
        if (!hold) return;
        MonoObject* exc = nullptr;
        MonoObject* value = bridge.InvokeMethod(get, reinterpret_cast<void*>(target), nullptr, &exc);
        if (!value || exc || !bridge.Unbox(value, original)) return;
        object = target;
        setter = set;
        bool disabled = false;
        void* args[] = { &disabled };
        applied = bridge.TryInvokeMethod(set, reinterpret_cast<void*>(target), args);
        if (applied) pin = std::move(hold);
    }
};
ObjectSwitch hurtImage, glitchParent;
int volumeOffset = -1;
std::array<int, 7> effectOffsets{};
int hurtImageOffset = -1, glitchParentOffset = -1;
std::array<int, 2> glitchOffsets{};
std::array<int, 13> shakeOffsets{};
std::array<int, 3> noiseOffsets{};

uintptr_t ReadObject(void* object, int offset) {
    uintptr_t result = 0;
    if (offset > 0) Memory::ReadPtr(reinterpret_cast<uintptr_t>(object) + offset, result);
    return result;
}
void RestoreEnvironment() {
    fog.Restore();
    ambientMode.Restore();
    ambientLight.Restore();
    ambientIntensity.Restore();
}
void RestorePost() {
    for (const auto& saved : postOverrides) saved.Restore();
    postOverrides.clear();
}
void TrackOverrides() {
    overridesActive = fog.applied || ambientMode.applied || ambientLight.applied || ambientIntensity.applied ||
        !postOverrides.empty() || hurtImage.applied || glitchParent.applied;
}
bool StopRequested() {
    if (!stopping) return false;
    if (Valid(environmentOwner)) RestoreEnvironment();
    else fog.applied = ambientMode.applied = ambientLight.applied = ambientIntensity.applied = false;
    RestorePost();
    hurtImage.Restore();
    glitchParent.Restore();
    overridesActive = false;
    restored = true;
    return true;
}

void EnvironmentHook(void* object) {
    Core::HookActivity activity;
    if (StopRequested()) { environmentUpdate(object); return; }
    uintptr_t owner = reinterpret_cast<uintptr_t>(object);
    if (owner == environmentOwner) RestoreEnvironment();
    else {

        fog.applied = ambientMode.applied = ambientLight.applied = ambientIntensity.applied = false;
        environmentOwner = owner;
    }
    environmentUpdate(object);
    const auto config = Core::Config::Copy();
    if (config.removals.enabled && config.removals.noFog) fog.Apply(false);
    if (config.fullbright.enabled) {
        float brightness = config.fullbright.brightness;
        if (!(brightness >= 0.1f && brightness <= 3.0f)) brightness = 1.0f;
        ambientMode.Apply(3);
        ambientIntensity.Apply(1.0f);
        ambientLight.Apply(Unity::Color{brightness, brightness, brightness, 1.0f});
    }
    TrackOverrides();
}

void PostHook(void* object) {
    Core::HookActivity activity;
    if (StopRequested()) { postUpdate(object); return; }
    RestorePost();
    postUpdate(object);
    const auto config = Core::Config::Copy().removals;
    if (config.enabled && objectClass) {
        uintptr_t volume = ReadObject(object, volumeOffset);
        if (config.noPostProcessing && Valid(volume))
            OverrideField(volume, Field(objectClass(reinterpret_cast<MonoObject*>(volume)), "weight"), 0.0f);
        const bool disabled[] = {config.noGrain, config.noBloom, config.noVignette, config.noMotionBlur,
            config.noLensDistortion, config.noChromaticAberration, config.noPostProcessing};
        for (size_t i = 0; i < effectOffsets.size(); ++i) {
            if (!disabled[i] && !config.noPostProcessing) continue;
            uintptr_t effect = ReadObject(object, effectOffsets[i]);
            if (!Valid(effect)) continue;
            OverrideField(effect, Field(objectClass(reinterpret_cast<MonoObject*>(effect)), "active"), false);
        }
    }
    TrackOverrides();
}

void ClearCameraEffect(void* object) {
    if (!Valid(reinterpret_cast<uintptr_t>(object)) || !effectLocalPositionSet || !effectLocalRotationSet) return;
    auto* transform=Unity::Engine::Get().GetTransform(reinterpret_cast<MonoObject*>(object));
    if (!Valid(reinterpret_cast<uintptr_t>(transform))) return;
    Unity::Vector3 neutralPosition{};
    std::array<float,4> neutralRotation{0,0,0,1};
    void* positionArgs[]={&neutralPosition};
    void* rotationArgs[]={neutralRotation.data()};
    auto& bridge=Mono::Bridge::Get();
    bridge.TryInvokeMethod(effectLocalPositionSet,transform,positionArgs);
    bridge.TryInvokeMethod(effectLocalRotationSet,transform,rotationArgs);
}
void BobHook(void* object) {
    Core::HookActivity activity;

    bobUpdate(object);
    const auto config=Core::Config::Copy().removals;
    if (!StopRequested() && config.enabled && config.noCameraShake) ClearCameraEffect(object);
}

void ShakeHook(void* object) {
    Core::HookActivity activity;
    const auto config = Core::Config::Copy().removals;
    if (!StopRequested() && config.enabled && config.noCameraShake) {

        for (int offset : shakeOffsets) if (offset > 0) Memory::WriteFloat(reinterpret_cast<uintptr_t>(object) + offset, 0.0f);
    }
    shakeUpdate(object);
    if (!stopping && config.enabled && config.noCameraShake) ClearCameraEffect(object);
}
void NoiseHook(void* object) {
    Core::HookActivity activity;
    const auto config = Core::Config::Copy().removals;
    if (StopRequested() || !config.enabled || !config.noCameraNoise) { noiseUpdate(object); return; }

    std::array<float, 3> original{};
    std::array<bool, 3> saved{};
    for (size_t i = 0; i < noiseOffsets.size(); ++i) {
        int offset = noiseOffsets[i];
        saved[i] = offset > 0 && Memory::ReadFloat(reinterpret_cast<uintptr_t>(object) + offset, original[i]);
        if (saved[i]) Memory::WriteFloat(reinterpret_cast<uintptr_t>(object) + offset, 0.0f);
    }
    noiseUpdate(object);
    for (size_t i = 0; i < noiseOffsets.size(); ++i)
        if (saved[i]) Memory::WriteFloat(reinterpret_cast<uintptr_t>(object) + noiseOffsets[i], original[i]);
}
void GlitchHook(void* object) {
    Core::HookActivity activity;
    if (StopRequested()) { glitchUpdate(object); return; }
    const auto config = Core::Config::Copy().removals;
    if (!config.enabled || !config.noCameraGlitch) glitchParent.Restore();
    if (config.enabled && config.noCameraGlitch) {
        for (int offset : glitchOffsets) {
            if (offset <= 0) continue;
            Memory::WriteFloat(reinterpret_cast<uintptr_t>(object) + offset, 0.0f);
        }
    }
    glitchUpdate(object);
    if (config.enabled && config.noCameraGlitch)
        glitchParent.Disable(ReadObject(object, glitchParentOffset), activeGet, activeSet);
    TrackOverrides();
}
void HurtHook(void* object) {
    Core::HookActivity activity;
    if (StopRequested()) { hurtUpdate(object); return; }
    const auto config = Core::Config::Copy().removals;
    if (!config.enabled || !config.noHurtOverlay) hurtImage.Restore();
    hurtUpdate(object);
    if (config.enabled && config.noHurtOverlay)
        hurtImage.Disable(ReadObject(object, hurtImageOffset), behaviourGet, behaviourSet);
    TrackOverrides();
}
bool HookUpdate(MonoClass* klass, UpdateMethod hook, UpdateMethod& original) {
    auto& bridge = Mono::Bridge::Get();
    MonoMethod* update = bridge.FindMethod(klass, "Update", 0);
    void* code = update && compileMethod ? compileMethod(update) : nullptr;
    if (!code || MH_CreateHook(code, hook, reinterpret_cast<void**>(&original)) != MH_OK) return false;
    if (MH_EnableHook(code) == MH_OK) return true;
    MH_RemoveHook(code);
    original = nullptr;
    return false;
}
}

Removals& Removals::Get() {
    static Removals instance;
    return instance;
}
void Removals::PollRestore() { StopRequested(); }
void Removals::Initialize() {
    auto& bridge = Mono::Bridge::Get();
    if (!bridge.AttachCurrentThread()) return;
    HMODULE mono = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!mono) mono = GetModuleHandleA("mono.dll");
    if (!mono) return;
    compileMethod = reinterpret_cast<CompileMethod>(GetProcAddress(mono, "mono_compile_method"));
    objectClass = reinterpret_cast<ObjectClass>(GetProcAddress(mono, "mono_object_get_class"));
    parentClass = reinterpret_cast<ParentClass>(GetProcAddress(mono, "mono_class_get_parent"));
    auto find = [&](const char* name) { return bridge.FindClass(bridge.GetAppImage(), "", name); };
    MonoClass* render = bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "RenderSettings");
    fog.get = bridge.FindMethod(render, "get_fog", 0);
    fog.set = bridge.FindMethod(render, "set_fog", 1);
    ambientMode.get = bridge.FindMethod(render, "get_ambientMode", 0);
    ambientMode.set = bridge.FindMethod(render, "set_ambientMode", 1);
    ambientLight.get = bridge.FindMethod(render, "get_ambientLight", 0);
    ambientLight.set = bridge.FindMethod(render, "set_ambientLight", 1);
    ambientIntensity.get = bridge.FindMethod(render, "get_ambientIntensity", 0);
    ambientIntensity.set = bridge.FindMethod(render, "set_ambientIntensity", 1);
    m_LightingReady = HookUpdate(find("EnvironmentDirector"), EnvironmentHook, environmentUpdate) &&
        fog.get && fog.set && ambientMode.get && ambientMode.set && ambientLight.get && ambientLight.set &&
        ambientIntensity.get && ambientIntensity.set;
    MonoClass* post = find("PostProcessing");
    volumeOffset = Field(post, "volume");
    const char* effectNames[] = {"grain", "bloom", "vignette", "motionBlur", "lensDistortion", "chromaticAberration", "colorGrading"};
    for (size_t i = 0; i < effectOffsets.size(); ++i) effectOffsets[i] = Field(post, effectNames[i]);
    m_PostProcessingReady = objectClass && HookUpdate(post, PostHook, postUpdate);
    MonoClass* effectTransform=bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "Transform");
    effectLocalPositionSet=bridge.FindMethod(effectTransform,"set_localPosition",1);
    effectLocalRotationSet=bridge.FindMethod(effectTransform,"set_localRotation",1);
    if (!HookUpdate(find("CameraBob"), BobHook, bobUpdate)) LOG_WARN("Removals: CameraBob hook unavailable");
    MonoClass* shake = find("CameraShake");
    const char* shakeNames[] = {"Strength", "RotXOld", "RotXNew", "RotYOld", "RotYNew", "RotZOld", "RotZNew",
        "PosXOld", "PosXNew", "PosYOld", "PosYNew", "PosZOld", "PosZNew"};
    for (size_t i = 0; i < shakeOffsets.size(); ++i) shakeOffsets[i] = Field(shake, shakeNames[i]);
    if (!HookUpdate(shake, ShakeHook, shakeUpdate)) LOG_WARN("Removals: CameraShake hook unavailable");
    MonoClass* noise = find("CameraNoise");
    const char* noiseNames[] = {"StrengthDefault", "Strength", "OverrideStrength"};
    for (size_t i = 0; i < noiseOffsets.size(); ++i) noiseOffsets[i] = Field(noise, noiseNames[i]);
    if (!HookUpdate(noise, NoiseHook, noiseUpdate)) LOG_WARN("Removals: CameraNoise hook unavailable");
    MonoClass* glitch = find("CameraGlitch");
    glitchParentOffset = Field(glitch, "ActiveParent");
    const char* glitchNames[] = {"doNotLookEffectTimer", "doNotLookEffectImpulseTimer"};
    for (size_t i = 0; i < glitchOffsets.size(); ++i) glitchOffsets[i] = Field(glitch, glitchNames[i]);
    MonoClass* gameObject = bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "GameObject");
    activeGet = bridge.FindMethod(gameObject, "get_activeSelf", 0);
    activeSet = bridge.FindMethod(gameObject, "SetActive", 1);
    if (!HookUpdate(glitch, GlitchHook, glitchUpdate)) LOG_WARN("Removals: CameraGlitch hook unavailable");
    MonoClass* hurt = find("HurtVignette");
    hurtImageOffset = Field(hurt, "image");
    MonoClass* behaviour = bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "Behaviour");
    behaviourGet = bridge.FindMethod(behaviour, "get_enabled", 0);
    behaviourSet = bridge.FindMethod(behaviour, "set_enabled", 1);
    if (!HookUpdate(hurt, HurtHook, hurtUpdate)) LOG_WARN("Removals: HurtVignette hook unavailable");
    LOG_INFO("Removals initialized: lighting=%s, post-processing=%s", m_LightingReady ? "ready" : "unavailable",
        m_PostProcessingReady ? "ready" : "unavailable");
}
bool Removals::RestoreBeforeUnload() {
    restored = false;
    stopping = true;
    for (int i = 0; i < 200 && !restored; ++i) Sleep(5);
    if (restored || !overridesActive) return true;
    stopping = false;
    LOG_WARN("Removals: waiting for game Update to restore visual settings; unload cancelled");
    return false;
}
}
