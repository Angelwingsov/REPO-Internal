#include "../src/features/player.cpp"
#include <cassert>
#include <cstring>
#include <limits>
#include <iostream>
#include <unordered_set>

namespace {
struct Avatar { void* header[2]{}; uint8_t local = 1; };
struct Visuals {
    void* header[2]{};
    Avatar* avatar = nullptr;
    float deltaTime = 0, speed = 1;
    uint8_t expression = 0;
};
std::unordered_set<void*> objects;
int updates = 0;
void DeltaTimeUpdate(void* object) {
    auto& visuals = *static_cast<Visuals*>(object);
    ++updates;
    visuals.deltaTime = visuals.expression ? .016f : (std::max)(0.f, .016f * visuals.speed);
}
bool Near(float a, float b) { return std::abs(a - b) < .000001f; }
}
namespace Game {
Runtime& Runtime::Get() { static Runtime r; return r; }
GameState& GameState::Get() { static GameState s; return s; }
bool GameState::IsMapLoadingOrRestarting() { return false; }
void GameState::SetAnimationSpeed(float) {}
}
namespace Mono {
Bridge& Bridge::Get() { static Bridge b; return b; }
MonoMethod* Bridge::FindMethod(MonoClass*, const char*, int) { return nullptr; }
int Bridge::GetFieldOffset(MonoClass*, const char*, int fallback) { return fallback; }
void* Bridge::GetStaticFieldValue(MonoClass*, const char*) { return nullptr; }
}
namespace Core { void Logger::Log(LogLevel, const char*, ...) {} }
namespace Hook { bool InputHook::IsKeyHeld(int) { return false; } }
MH_STATUS WINAPI MH_CreateHook(LPVOID, LPVOID, LPVOID*) { return MH_ERROR_NOT_INITIALIZED; }
MH_STATUS WINAPI MH_EnableHook(LPVOID) { return MH_ERROR_NOT_INITIALIZED; }
MH_STATUS WINAPI MH_RemoveHook(LPVOID) { return MH_ERROR_NOT_INITIALIZED; }
namespace Unity { bool Engine::IsValidUnityObject(void* object) { return objects.count(object) != 0; } }
namespace Memory {
bool SafeCopyMemory(void* dst, const void* src, size_t size) {
    if (!dst || !src) return false;
    std::memcpy(dst, src, size);
    return true;
}
}
int main() {
    using namespace Features;
    Avatar avatar;
    Visuals visuals;
    visuals.avatar = &avatar;
    objects.insert(&avatar);
    objects.insert(&visuals);
    visualsAvatarOffset = offsetof(Visuals, avatar);
    visualsDeltaTimeOffset = offsetof(Visuals, deltaTime);
    visualsSpeedOffset = offsetof(Visuals, speed);
    visualsExpressionOffset = offsetof(Visuals, expression);
    avatarLocalOffset = offsetof(Avatar, local);
    originalDeltaTimeUpdate = DeltaTimeUpdate;
    auto& config = Core::Config::Get().troll;
    config.overrideAnimSpeed = true;
    for (float speed : {.1f, .2f, 1.f, 3.f, 5.f}) {
        visuals.speed = speed;
        float position = 0;
        for (int frame = 0; frame < 60; ++frame) {
            VisualsDeltaTimeHook(&visuals);
            assert(Near(visuals.deltaTime, .016f) && visuals.speed == speed);
            position += (10.f - position) * (std::min)(1.f, 25.f * visuals.deltaTime);
        }
        assert(position > 9.99f);
    }
    visuals.speed = .2f;
    avatar.local = 0;
    VisualsDeltaTimeHook(&visuals);
    assert(Near(visuals.deltaTime, .0032f));
    avatar.local = 1;
    config.overrideAnimSpeed = false;
    VisualsDeltaTimeHook(&visuals);
    assert(Near(visuals.deltaTime, .0032f));
    config.overrideAnimSpeed = true;
    visuals.expression = 1;
    VisualsDeltaTimeHook(&visuals);
    assert(Near(visuals.deltaTime, .016f));
    visuals.expression = 0;
    objects.erase(&avatar);
    VisualsDeltaTimeHook(&visuals);
    assert(Near(visuals.deltaTime, .0032f));
    objects.insert(&avatar);
    for (float speed : {0.f, -1.f, std::numeric_limits<float>::infinity()}) {
        visuals.speed = speed;
        VisualsDeltaTimeHook(&visuals);
        assert(speed > 0 ? std::isinf(visuals.deltaTime) : visuals.deltaTime == 0);
    }
    assert(updates == 307);
    assert(Core::HookActivity::WaitForIdle());
    std::cout << "PASS: local visual follow stays at normal speed for 0.1x through 5x animations; remote, disabled, expression and invalid cases preserved\n";
}
