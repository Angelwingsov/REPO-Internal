#pragma once
#include <windows.h>
#include <cstdint>

namespace Features {

class GodMode {
public:
    static GodMode& Get();

    void Update();

    bool ReviveLocalPlayer();
    bool HealLocalPlayer(int healAmount = 0);

private:
    GodMode() = default;

    bool m_PrevKeyDown = false;
    bool m_ActiveState = false;

    uintptr_t m_LastPlayerAvatar = 0;
    uintptr_t m_LastPlayerHealth = 0;
    bool m_GodModeSaved = false;
};

}
