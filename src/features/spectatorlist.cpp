#include "spectatorlist.h"
#include "core/config.h"
#include "core/logger.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "mono/mono_bridge.h"
#include "ui/theme.h"

namespace Features {

SpectatorList& SpectatorList::Get() {
    static SpectatorList instance;
    return instance;
}

void SpectatorList::Update() {
    std::vector<SpectatorEntry> spectators;
    Collect(spectators);
    m_Snapshot.Publish(std::move(spectators));
}

void SpectatorList::Collect(std::vector<SpectatorEntry>& m_Spectators) {
    m_Spectators.clear();

    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        return;
    }
    if (!gameState.klassGameDirector) {
        return;
    }

    auto& bridge = Mono::Bridge::Get();
    auto& engine = Unity::Engine::Get();

    uintptr_t gameDirector = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(gameState.klassGameDirector, "instance"));
    if (!gameDirector || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(gameDirector))) {
        return;
    }

    uintptr_t playerList = 0;
    if (!Memory::ReadPtr(gameDirector + gameState.offsets.GameDirector_PlayerList, playerList) || !playerList) {
        return;
    }

    std::vector<uintptr_t> avatars;
    if (!Memory::ReadMonoList(playerList, avatars) || avatars.empty()) {
        return;
    }

    struct ParsedPlayer {
        uintptr_t address = 0;
        std::string name;
        bool isLocal = false;
        bool isDead = false;
        bool spectating = false;
        Unity::Vector3 worldPos;
        Unity::Vector3 cameraPos;
        bool hasCameraPos = false;
    };

    std::vector<ParsedPlayer> allPlayers;
    allPlayers.reserve(avatars.size());

    ParsedPlayer localPlayer;
    bool hasLocal = false;

    for (uintptr_t avatar : avatars) {
        if (!avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
            continue;
        }

        ParsedPlayer p;
        p.address = avatar;

        uint8_t isLocal = 0;
        Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_isLocal, isLocal);
        p.isLocal = (isLocal != 0);

        uint8_t deadSet = 0;
        Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_deadSet, deadSet);

        uint8_t isDisabled = 0;
        Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_isDisabled, isDisabled);

        uint8_t spectating = 0;
        Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_spectating, spectating);

        p.isDead = (deadSet != 0 || isDisabled != 0);
        p.spectating = (spectating != 0);

        uintptr_t mName = 0;
        if (Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerName, mName) && mName) {
            p.name = bridge.ReadMonoString(reinterpret_cast<MonoString*>(mName));
        }
        if (p.name.empty()) {
            p.name = p.isLocal ? "Local Player" : "Teammate";
        }

        uintptr_t playerTransform = 0;
        Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerTransform, playerTransform);
        if (playerTransform && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerTransform))) {
            p.worldPos = engine.GetTransformPosition(reinterpret_cast<MonoObject*>(playerTransform));
        } else {
            p.worldPos = engine.GetComponentPosition(reinterpret_cast<MonoObject*>(avatar));
        }

        uint8_t camValid = 0;
        Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_clientCameraStateValid, camValid);
        p.hasCameraPos = (camValid != 0);
        if (p.hasCameraPos) {
            Memory::Read<Unity::Vector3>(avatar + gameState.offsets.PlayerAvatar_clientCameraPosition, p.cameraPos);
        }

        if (p.isLocal) {
            localPlayer = p;
            hasLocal = true;
        }
        allPlayers.push_back(p);
    }

    if (hasLocal && (localPlayer.isDead || localPlayer.spectating)) {
        std::string targetName = "Unknown";
        std::string modeStr = "3rd Person";

        uintptr_t spectateCam = 0;
        if (gameState.klassSpectateCamera) {
            spectateCam = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(gameState.klassSpectateCamera, "instance"));
        }

        if (spectateCam && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(spectateCam))) {
            uintptr_t targetAvatar = 0;
            Memory::ReadPtr(spectateCam + gameState.offsets.SpectateCamera_player, targetAvatar);

            int currentState = 1;
            Memory::ReadInt(spectateCam + gameState.offsets.SpectateCamera_currentState, currentState);

            if (targetAvatar && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(targetAvatar))) {
                uintptr_t mTargetName = 0;
                if (Memory::ReadPtr(targetAvatar + gameState.offsets.PlayerAvatar_playerName, mTargetName) && mTargetName) {
                    targetName = bridge.ReadMonoString(reinterpret_cast<MonoString*>(mTargetName));
                }
            }

            if (currentState == 0) {
                modeStr = "Death Cam";
            } else if (currentState == 2) {
                modeStr = "Head Cam";
            } else {
                modeStr = "3rd Person";
            }
        }

        SpectatorEntry entry;
        entry.spectatorName = localPlayer.name + " (You)";
        entry.targetName = targetName;
        entry.isLocalSpectator = true;
        entry.isSpectatingMe = false;
        entry.modeStr = modeStr;
        m_Spectators.push_back(entry);
    }

    for (const auto& p : allPlayers) {
        if (p.isLocal) continue;

        if (p.isDead || p.spectating) {
            std::string targetName = "-";
            std::string modeStr = "Spectating";
            bool isSpectatingMe = false;

            if (p.hasCameraPos && p.cameraPos.IsValid() && !p.cameraPos.IsZero()) {
                float closestDist = 999999.0f;
                const ParsedPlayer* closestPlayer = nullptr;

                for (const auto& lp : allPlayers) {
                    if (lp.isDead) continue;
                    if (!lp.worldPos.IsValid() || lp.worldPos.IsZero()) continue;

                    float dist = p.cameraPos.Distance(lp.worldPos);
                    if (dist < closestDist) {
                        closestDist = dist;
                        closestPlayer = &lp;
                    }
                }

                if (closestPlayer != nullptr && closestDist <= 6.5f) {
                    if (closestPlayer->isLocal) {
                        isSpectatingMe = true;
                        targetName = "YOU";
                        modeStr = "3rd Person";
                    } else {
                        isSpectatingMe = false;
                        targetName = closestPlayer->name;
                        modeStr = "3rd Person";
                    }
                } else {
                    float distToOwnCorpse = p.worldPos.Distance(p.cameraPos);
                    if (distToOwnCorpse <= 4.5f) {
                        targetName = "Death Cam";
                        modeStr = "Death";
                    } else {
                        targetName = "-";
                        modeStr = "Head / Free";
                    }
                }
            } else {
                targetName = "-";
                modeStr = "Dead";
            }

            SpectatorEntry entry;
            entry.spectatorName = p.name;
            entry.targetName = targetName;
            entry.isLocalSpectator = false;
            entry.isSpectatingMe = isSpectatingMe;
            entry.modeStr = modeStr;
            m_Spectators.push_back(entry);
        }
    }
}

void SpectatorList::Render(ImDrawList* drawList, float screenWidth, float screenHeight) {
    if (!drawList) return;

    auto m_Spectators = m_Snapshot.Read();
    auto& cfg = Core::Config::Get();
    if (!cfg.spectatorList.enabled) return;

    std::vector<const SpectatorEntry*> displayList;
    displayList.reserve(m_Spectators.size());
    for (const auto& s : m_Spectators) {
        if (cfg.spectatorList.onlySpectatingMe && !s.isSpectatingMe) {
            continue;
        }
        displayList.push_back(&s);
    }

    if (displayList.empty()) {
        return;
    }

    ImFont* font = UI::Theme::TextFont();
    const float fontSize = font->LegacySize * 1.25f;

    const float marginX = 15.0f;
    const float marginY = 20.0f;
    const float lineStep = fontSize + 3.0f;
    const float right = screenWidth - marginX;
    float y = marginY;

    auto drawRight = [&](const char* text, ImU32 color) {
        ImVec2 sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0, text);
        drawList->AddText(font, fontSize, ImVec2(right - sz.x, y), color, text);
        y += lineStep;
    };

    const ImU32 headerColor = IM_COL32(64, 166, 255, 255);
    const ImU32 spectatorColor = IM_COL32(242, 242, 242, 255);
    const ImU32 targetMeColor = IM_COL32(51, 255, 89, 255);
    const ImU32 targetOtherColor = IM_COL32(255, 217, 64, 255);

    char header[64];
    if (cfg.spectatorList.showSpectatorCount) {
        sprintf_s(header, "Spectators (%zu)", displayList.size());
    } else {
        strcpy_s(header, "Spectators");
    }
    drawRight(header, headerColor);

    for (const auto* s : displayList) {
        std::string line = s->spectatorName;

        if (cfg.spectatorList.showTarget) {
            if (s->isSpectatingMe) {
                line += " -> YOU";
            } else {
                line += " -> ";
                line += s->targetName;
            }
        }

        if (cfg.spectatorList.showMode) {
            line += " [";
            line += s->modeStr;
            line += "]";
        }

        drawRight(line.c_str(), s->isSpectatingMe ? targetMeColor : (s->isLocalSpectator ? targetOtherColor : spectatorColor));
    }
}

}
