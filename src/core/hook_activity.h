#pragma once
#include <atomic>
#include <windows.h>

namespace Core {
class HookActivity {
public:
    HookActivity() { s_Active.fetch_add(1); }
    ~HookActivity() { s_Active.fetch_sub(1); }
    HookActivity(const HookActivity&) = delete;
    static bool WaitForIdle() {
        for (int i = 0; i < 400; ++i) {
            if (s_Active.load() == 0) return true;
            Sleep(5);
        }
        return false;
    }
private:
    static inline std::atomic<unsigned> s_Active{0};
};
}
