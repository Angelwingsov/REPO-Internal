#include "spectatorlist.h"
#include "core/config.h"
#include "core/logger.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "mono/mono_bridge.h"
#include "imgui/imgui.h"

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

void SpectatorList::Render() {
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

    if (cfg.spectatorList.hideWhenEmpty && displayList.empty()) {
        return;
    }

    char windowTitle[64];
    if (cfg.spectatorList.showSpectatorCount) {
        sprintf_s(windowTitle, "Spectators (%zu)###SpectatorListWindow", displayList.size());
    } else {
        strcpy_s(windowTitle, "Spectators###SpectatorListWindow");
    }

    ImGui::SetNextWindowPos(ImVec2(25.0f, 65.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250.0f, 0.0f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(cfg.spectatorList.colorHeader[0] * 0.35f, cfg.spectatorList.colorHeader[1] * 0.35f, cfg.spectatorList.colorHeader[2] * 0.35f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(cfg.spectatorList.colorHeader[0] * 0.55f, cfg.spectatorList.colorHeader[1] * 0.55f, cfg.spectatorList.colorHeader[2] * 0.55f, 1.0f));

    if (ImGui::Begin(windowTitle, &cfg.spectatorList.enabled, flags)) {
        if (displayList.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 0.8f), "No spectators active");
        } else {
            int columns = 1;
            if (cfg.spectatorList.showTarget) columns++;
            if (cfg.spectatorList.showMode) columns++;

            if (ImGui::BeginTable("##SpectatorTable", columns, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
                ImGui::TableSetupColumn("Spectator");
                if (cfg.spectatorList.showTarget) {
                    ImGui::TableSetupColumn("Target");
                }
                if (cfg.spectatorList.showMode) {
                    ImGui::TableSetupColumn("Mode");
                }

                for (const auto* s : displayList) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (s->isSpectatingMe) {
                        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "[!]");
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(cfg.spectatorList.colorSpectator[0], cfg.spectatorList.colorSpectator[1], cfg.spectatorList.colorSpectator[2], 1.0f), "%s", s->spectatorName.c_str());
                    } else if (s->isLocalSpectator) {
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", s->spectatorName.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(cfg.spectatorList.colorSpectator[0], cfg.spectatorList.colorSpectator[1], cfg.spectatorList.colorSpectator[2], 1.0f), "%s", s->spectatorName.c_str());
                    }

                    if (cfg.spectatorList.showTarget) {
                        ImGui::TableNextColumn();
                        if (s->isSpectatingMe) {
                            ImGui::TextColored(ImVec4(cfg.spectatorList.colorTargetMe[0], cfg.spectatorList.colorTargetMe[1], cfg.spectatorList.colorTargetMe[2], 1.0f), "-> YOU");
                        } else if (s->targetName == "-") {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                        } else {
                            ImGui::TextColored(ImVec4(cfg.spectatorList.colorTargetOther[0], cfg.spectatorList.colorTargetOther[1], cfg.spectatorList.colorTargetOther[2], 1.0f), "-> %s", s->targetName.c_str());
                        }
                    }

                    if (cfg.spectatorList.showMode) {
                        ImGui::TableNextColumn();
                        ImVec4 modeColor = ImVec4(0.6f, 0.6f, 0.6f, 0.8f);
                        if (s->modeStr == "3rd Person") {
                            modeColor = ImVec4(0.2f, 0.85f, 1.0f, 0.95f);
                        } else if (s->modeStr == "Head Cam") {
                            modeColor = ImVec4(1.0f, 0.65f, 0.2f, 0.95f);
                        } else if (s->modeStr == "Death Cam") {
                            modeColor = ImVec4(0.85f, 0.35f, 0.35f, 0.95f);
                        }

                        ImGui::TextColored(modeColor, "[%s]", s->modeStr.c_str());
                    }
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

}
