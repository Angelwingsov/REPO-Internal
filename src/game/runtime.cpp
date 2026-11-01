#include "runtime.h"
#include "core/config.h"
#include "core/logger.h"
#include "features/esp.h"
#include "features/godmode.h"
#include "features/noclip.h"
#include "features/player.h"
#include "features/thirdperson.h"
#include "features/spectatorlist.h"
#include "features/removals.h"

#include "features/chams.h"
#include "features/antiaim.h"
#include "features/animbreaker.h"
#include "features/spawner.h"

#include "unity/unity_engine.h"
#include <exception>

namespace Game {
Runtime& Runtime::Get() { static Runtime runtime; return runtime; }
void Runtime::SetViewport(float width, float height) { m_Viewport.Publish({width, height}); }
bool Runtime::Queue(Action action) {
    if (m_Stopping || !GameState::Get().IsReady()) return false;
    std::lock_guard<std::mutex> lock(m_ActionsMutex);
    if (m_Actions.size() >= 16) return false;
    m_Actions.push_back(action);
    return true;
}
bool Runtime::TakeResult(Action action, bool& success) {
    std::lock_guard<std::mutex> lock(m_ActionsMutex);
    for (auto it = m_Results.begin(); it != m_Results.end(); ++it) {
        if (it->action == action) {
            success = it->success;
            m_Results.erase(it);
            return true;
        }
    }
    return false;
}
void Runtime::Tick() {
    if (!GameState::Get().IsReady()) return;
    Features::Removals::Get().PollRestore();
    if (m_Stopped) return;
    auto config = Core::Config::Copy();
    ULONGLONG now = GetTickCount64();
    auto frameStart = std::chrono::steady_clock::now();
    float dt = m_LastTick ? std::chrono::duration<float>(frameStart - m_LastFrame).count() : 0.016f;
    m_LastFrame = frameStart;
    m_LastTick = now;
    bool stop = m_Stopping.load();
    try {
        {

            Core::Config::Scope scope(config);
            if (stop) {
                config.godMode.enabled = config.noclip.enabled = config.movement.speedEnabled = false;
                config.movement.infiniteStamina = config.movement.infiniteStrength = false;
                config.movement.extendGrabRange = config.movement.grabThroughWalls = config.movement.superJump = config.movement.noTumble = false;
                config.thirdPerson.enabled = false;
                config.chams.enabled = false;
                config.antiAim.enabled = false;
                config.animBreaker.enabled = false;
                config.troll = Core::TrollConfig();
                config.menu.isVisible = false;
            }
            Unity::Engine::Get().UpdateCursorState(config.menu.isVisible);
            Features::GodMode::Get().Update();
            Features::NoClip::Get().Update(dt);
            Features::PlayerMods::Get().Update();
            Features::ThirdPerson::Get().Update();
            Features::Chams::Get().Update();
            Features::AntiAim::Get().Update(dt);
            Features::AnimBreaker::Get().Update(dt);
            Features::Spawner::Get().Update(dt);

            if (!stop) {
                std::deque<Action> actions;
                { std::lock_guard<std::mutex> lock(m_ActionsMutex); actions.swap(m_Actions); }
                for (Action action : actions) {
                    bool success = false;
                    auto& gs = GameState::Get();
                    if (action == Action::UnlockCosmetics || !gs.IsMapLoadingOrRestarting()) {
                        switch (action) {
                        case Action::Heal: success = gs.HealLocalPlayer(); break;
                        case Action::Revive: success = gs.ReviveLocalPlayer(); break;
                        case Action::UnlockCosmetics: success = gs.UnlockAllCosmetics(); break;
                        }
                    }
                    std::lock_guard<std::mutex> lock(m_ActionsMutex);
                    if (m_Results.size() >= 16) m_Results.pop_front();
                    m_Results.push_back({action, success});
                }

                Features::SpectatorList::Get().Update();
                auto viewport = m_Viewport.Read();
                Features::ESP::Get().Update(viewport.first, viewport.second);
                RuntimeState state;
                state.timestamp = now;
                if (!GameState::Get().IsMapLoadingOrRestarting()) {
                    state.players = GameState::Get().GetServerPlayers();
                    state.hasCosmeticStats = GameState::Get().GetCosmeticStats(state.unlocked, state.total);
                }
                m_State.Publish(std::move(state));
            }
        }
        {
            std::lock_guard<std::recursive_mutex> lock(Core::Config::Mutex());
            auto& global = Core::Config::Get();
            global.godMode.active = config.godMode.active;
            global.noclip.active = config.noclip.active;
            global.movement.speedActive = config.movement.speedActive;
            global.thirdPerson.active = config.thirdPerson.active;
            global.antiAim.active = config.antiAim.active;
            float fps = global.diagnostics.fps, frameTime = global.diagnostics.frameTimeMs;
            global.diagnostics = config.diagnostics;
            global.diagnostics.fps = fps;
            global.diagnostics.frameTimeMs = frameTime;
        }
        if (stop) { m_State.Publish({}); m_Stopped = true; }
    } catch (const std::exception& exception) {
        LOG_ERR("Game tick failed: %s", exception.what());
        m_State.Publish({});
    }
}
bool Runtime::StopAndWait() {
    m_Stopping = true;
    for (int i = 0; i < 400 && !m_Stopped; ++i) Sleep(5);
    if (m_Stopped || !m_LastTick) {
        Features::Chams::Get().Restore();
        Features::AntiAim::Get().Reset();
        Features::AnimBreaker::Get().Reset();
        return true;
    }
    m_Stopping = false;
    LOG_WARN("Game thread did not acknowledge shutdown; DLL remains loaded");
    return false;
}
}
