#pragma once
#include <windows.h>
#include <cstdint>

namespace Features {

class ThirdPerson {
public:
    static ThirdPerson& Get();

    void Update();

private:
    ThirdPerson() = default;

    bool m_PrevKeyDown = false;
    bool m_ActiveState = false;
    bool m_WasActive = false;
};

}
