#pragma once
#include "core/snapshot.h"
#include "state.h"
#include <atomic>
#include <deque>
#include <mutex>
#include <chrono>

namespace Game {
enum class Action { Heal, Revive, UnlockCosmetics };
struct RuntimeState {
    std::vector<ServerPlayerInfo> players;
    int unlocked = 0, total = 0;
    bool hasCosmeticStats = false;
    ULONGLONG timestamp = 0;
};
struct ActionResult { Action action = Action::Heal; bool success = false; };
class Runtime {
public:
    static Runtime& Get();
    void Tick();
    void SetViewport(float width, float height);
    bool Queue(Action action);
    bool TakeResult(Action action, bool& success);
    RuntimeState ReadState() const { return m_State.Read(); }
    bool StopAndWait();
    bool IsStopping() const { return m_Stopping.load(); }
private:
    Core::Snapshot<RuntimeState> m_State;
    Core::Snapshot<std::pair<float, float>> m_Viewport;
    std::mutex m_ActionsMutex;
    std::deque<Action> m_Actions;
    std::deque<ActionResult> m_Results;
    std::atomic<bool> m_Stopping{false}, m_Stopped{false};
    std::atomic<ULONGLONG> m_LastTick{0};
    std::chrono::steady_clock::time_point m_LastFrame{};
};
}
