#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "unity/unity_math.h"

namespace Features {

struct SpectatorEntry {
    std::string spectatorName;
    std::string targetName;
    bool isLocalSpectator = false;
    bool isSpectatingMe = false;
    std::string modeStr;
};

class SpectatorList {
public:
    static SpectatorList& Get();

    void Update();
    void Render();

    const std::vector<SpectatorEntry>& GetSpectators() const { return m_Spectators; }
    size_t GetCount() const { return m_Spectators.size(); }

private:
    SpectatorList() = default;

    std::vector<SpectatorEntry> m_Spectators;
};

}
