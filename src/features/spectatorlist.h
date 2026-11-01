#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "unity/unity_math.h"
#include "core/snapshot.h"

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

    std::vector<SpectatorEntry> GetSpectators() const { return m_Snapshot.Read(); }
    size_t GetCount() const { return m_Snapshot.Read().size(); }

private:
    SpectatorList() = default;

    void Collect(std::vector<SpectatorEntry>& m_Spectators);
    Core::Snapshot<std::vector<SpectatorEntry>> m_Snapshot;
};

}
