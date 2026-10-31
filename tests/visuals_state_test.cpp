// Standalone boundary test: runs the production hooks with an in-memory Mono/Unity adapter.
// Compile with MSVC, src/core/config.cpp, /I src and /I .
#include "../src/features/removals.cpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_set>

namespace {
struct FakeProperty {
    size_t size = 0;
    alignas(16) unsigned char value[16]{};
    bool failRead = false;
    bool failWrite = false;
};
std::unordered_set<void*> objects;
template<typename T> void Set(FakeProperty& property, T value) {
    property.size = sizeof(T);
    std::memcpy(property.value, &value, sizeof(T));
}
template<typename T> T Get(FakeProperty& property) {
    T value{};
    std::memcpy(&value, property.value, sizeof(T));
    return value;
}
template<typename T> void Bind(Features::Property<T>& binding, FakeProperty& property) {
    binding = {};
    binding.get = binding.set = reinterpret_cast<MonoMethod*>(&property);
}
bool changingFog = true;
float changingLight = 0.2f;
void GameEnvironmentUpdate(void*) {
    Features::fog.Write(changingFog);
    Features::ambientMode.Write(1);
    Features::ambientIntensity.Write(0.7f);
    Features::ambientLight.Write(Unity::Color{changingLight, changingLight, changingLight, 1.0f});
}
void Noop(void*) {}
struct FakeEffect { void* header[2]{}; bool active = true; };
struct FakeVolume { void* header[2]{}; float weight = 0.35f; };
}

namespace Mono {
Bridge& Bridge::Get() { static Bridge bridge; return bridge; }
MonoObject* Bridge::InvokeMethod(MonoMethod* method, void*, void**, MonoObject** exc) {
    if (exc) *exc = nullptr;
    if (!method) return nullptr;
    auto* property = reinterpret_cast<FakeProperty*>(method);
    return property->failRead ? nullptr : reinterpret_cast<MonoObject*>(property->value);
}
bool Bridge::TryInvokeMethod(MonoMethod* method, void*, void** args) {
    if (!method || !args) return false;
    auto* property = reinterpret_cast<FakeProperty*>(method);
    if (property->failWrite) return false;
    std::memcpy(property->value, args[0], property->size);
    return true;
}
void* Bridge::SafeUnboxRaw(MonoObject* value) { return value; }
MonoClass* Bridge::FindClass(MonoImage*, const char*, const char*) { return nullptr; }
MonoMethod* Bridge::FindMethod(MonoClass*, const char*, int) { return nullptr; }
int Bridge::GetFieldOffset(MonoClass*, const char* name, int fallback) {
    if (std::strcmp(name, "weight") == 0) return static_cast<int>(offsetof(FakeVolume, weight));
    return std::strcmp(name, "active") == 0 ? static_cast<int>(offsetof(FakeEffect, active)) : fallback;
}
}
namespace Unity {
bool Engine::IsValidUnityObject(void* object) { return objects.count(object) != 0; }
}
namespace Memory {
bool SafeCopyMemory(void* target, const void* source, size_t size) {
    if (!target || !source) return false;
    std::memcpy(target, source, size);
    return true;
}
}
namespace Core { void Logger::Log(LogLevel, const char*, ...) {} }
MH_STATUS WINAPI MH_CreateHook(LPVOID, LPVOID, LPVOID*) { return MH_ERROR_NOT_INITIALIZED; }
MH_STATUS WINAPI MH_EnableHook(LPVOID) { return MH_ERROR_NOT_INITIALIZED; }
MH_STATUS WINAPI MH_RemoveHook(LPVOID) { return MH_ERROR_NOT_INITIALIZED; }

int main() {
    using namespace Features;
    FakeProperty fogProperty, modeProperty, lightProperty, intensityProperty;
    Set(fogProperty, true); Set(modeProperty, 1); Set(intensityProperty, 0.7f);
    Set(lightProperty, Unity::Color{0.2f, 0.2f, 0.2f, 1.0f});
    Bind(fog, fogProperty); Bind(ambientMode, modeProperty);
    Bind(ambientLight, lightProperty); Bind(ambientIntensity, intensityProperty);
    environmentUpdate = GameEnvironmentUpdate;
    int scene = 0, newScene = 0;
    auto& config = Core::Config::Get();

    config.removals.enabled = true;
    EnvironmentHook(&scene);
    assert(!Get<bool>(fogProperty));
    assert(Get<Unity::Color>(lightProperty).r == 0.2f); // No Fog does not enable Fullbright.
    config.fullbright.enabled = true;
    config.fullbright.brightness = 1.5f;
    for (int frame = 0; frame < 5; ++frame) EnvironmentHook(&scene);
    assert(Get<int>(modeProperty) == 3);
    assert(Get<Unity::Color>(lightProperty).r == 1.5f);
    assert(Get<float>(intensityProperty) == 1.0f);

    // Vanilla lighting changes while overridden; turning off must restore the new room, not the initial room.
    changingLight = 0.4f;
    EnvironmentHook(&scene);
    config.fullbright.enabled = false;
    EnvironmentHook(&scene);
    assert(Get<Unity::Color>(lightProperty).r == 0.4f);
    assert(Get<int>(modeProperty) == 1);
    assert(!Get<bool>(fogProperty));
    config.removals.enabled = false;
    EnvironmentHook(&scene);
    assert(Get<bool>(fogProperty));

    // A new scene with fog already off must not inherit the previous scene's fog setting.
    config.fullbright.enabled = true;
    config.removals.enabled = true;
    changingFog = false;
    changingLight = 0.3f;
    EnvironmentHook(&newScene);
    config.fullbright.enabled = false;
    config.removals.enabled = false;
    EnvironmentHook(&newScene);
    assert(!Get<bool>(fogProperty));
    assert(Get<Unity::Color>(lightProperty).r == 0.3f);

    // A failed Unity getter must never fabricate a baseline or mark the override as applied.
    fogProperty.failRead = true;
    config.removals.enabled = true;
    EnvironmentHook(&newScene);
    assert(!fog.applied);
    fogProperty.failRead = false;

    FakeEffect grain;
    FakeVolume volume;
    struct FakePost { void* header[2]{}; FakeEffect* grain = nullptr; FakeVolume* volume = nullptr; } post;
    post.grain = &grain;
    post.volume = &volume;
    objects.insert(&grain);
    objects.insert(&volume);
    volumeOffset = static_cast<int>(offsetof(FakePost, volume));
    effectOffsets.fill(-1);
    effectOffsets[0] = static_cast<int>(offsetof(FakePost, grain));
    objectClass = [](MonoObject* obj) -> MonoClass* { return reinterpret_cast<MonoClass*>(obj); };
    postUpdate = Noop;
    config.removals.noGrain = true;
    PostHook(&post);
    assert(!grain.active);

    config.removals.noPostProcessing = true;
    PostHook(&post);
    assert(volume.weight == 0.0f);
    config.removals.noPostProcessing = false;
    PostHook(&post);
    assert(volume.weight == 0.35f); // Restore the actual weight, not a hard-coded 1.0.

    // Reapplying a hidden overlay must not replace its original enabled state with false.
    FakeProperty enabled;
    Set(enabled, true);
    int image = 0;
    objects.insert(&image);
    ObjectSwitch hidden;
    hidden.Disable(reinterpret_cast<uintptr_t>(&image), &enabled, &enabled);
    hidden.Disable(reinterpret_cast<uintptr_t>(&image), &enabled, &enabled);
    assert(!Get<bool>(enabled));
    hidden.Restore();
    assert(Get<bool>(enabled));
    config.removals.noGrain = false;
    PostHook(&post);
    assert(grain.active);
    config.removals.noGrain = true;
    PostHook(&post);
    assert(!grain.active);

    // The unload handshake restores effects and render settings before hook detachment.
    config.fullbright.enabled = true;
    EnvironmentHook(&newScene);
    stopping = true;
    EnvironmentHook(&newScene);
    assert(restored && !overridesActive);
    assert(grain.active);
    assert(Get<int>(modeProperty) == 1);
    assert(Get<Unity::Color>(lightProperty).r == 0.3f);
    std::cout << "PASS: fog/fullbright independence, room changes, getter failures, effect toggles, unload restoration\n";
}
