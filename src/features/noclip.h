#pragma once
#include <windows.h>
#include <cstdint>

namespace Features {

class NoClip {
public:
    static NoClip& Get();

    void Update(float deltaTime);

private:
    NoClip() = default;

    void RestorePhysics();

    bool m_PrevKeyDown = false;
    bool m_ActiveState = false;
    bool m_WasActive = false;

    uintptr_t m_LastPlayerCtrl = 0;
    uintptr_t m_LastPlayerAvatar = 0;
};

}
