#include "menu.h"
#include "imgui/imgui.h"
#include "core/config.h"
#include "mono/mono_bridge.h"
#include "game/state.h"
#include "game/runtime.h"
#include "features/player.h"
#include "features/godmode.h"
#include "features/removals.h"
#include "hook/input.h"
#include <vector>
#include <string>

namespace UI {

static void DrawKeybindWidget(const char* label, int& key, bool& isBinding, bool& waitRelease, float width = 140.0f) {
    char btnText[64];
    if (isBinding) {
        strcpy_s(btnText, "[ Press Key/Wheel... ]");

        ImGuiIO& io = ImGui::GetIO();
        if (waitRelease) {
            if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                waitRelease = false;
            }
        } else {
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                isBinding = false;
            }
            else if (io.MouseWheel > 0.1f || Hook::InputHook::ConsumeWheelUp()) {
                key = VK_MWHEELUP;
                isBinding = false;
                io.MouseWheel = 0.0f;
            }
            else if (io.MouseWheel < -0.1f || Hook::InputHook::ConsumeWheelDown()) {
                key = VK_MWHEELDOWN;
                isBinding = false;
                io.MouseWheel = 0.0f;
            }
            else if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                key = VK_MBUTTON;
                isBinding = false;
            }
            else if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
                key = VK_RBUTTON;
                isBinding = false;
            }
            else if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
                key = VK_XBUTTON1;
                isBinding = false;
            }
            else if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
                key = VK_XBUTTON2;
                isBinding = false;
            }
            else {
                for (int k = VK_BACK; k <= 0xFE; ++k) {
                    if (k == VK_ESCAPE || k == VK_MWHEELUP || k == VK_MWHEELDOWN) continue;
                    if (GetAsyncKeyState(k) & 0x8000) {
                        key = k;
                        isBinding = false;
                        break;
                    }
                }
            }
        }
    } else {
        sprintf_s(btnText, "Key: [ %s ]##%s", Features::PlayerMods::GetKeyName(key), label);
    }

    if (ImGui::Button(btnText, ImVec2(width, 24))) {
        isBinding = !isBinding;
        waitRelease = isBinding;
    }

    char popupId[64];
    sprintf_s(popupId, "Context##%s", label);
    if (ImGui::BeginPopupContextItem(popupId)) {
        ImGui::TextDisabled("Quick Wheel Binds:");
        if (ImGui::Selectable("MWheelDown (Scroll Down)")) {
            key = VK_MWHEELDOWN;
            isBinding = false;
        }
        if (ImGui::Selectable("MWheelUp (Scroll Up)")) {
            key = VK_MWHEELUP;
            isBinding = false;
        }
        if (ImGui::Selectable("M3 (Wheel Click)")) {
            key = VK_MBUTTON;
            isBinding = false;
        }
        ImGui::Separator();
        if (ImGui::Selectable("Clear / None")) {
            key = 0;
            isBinding = false;
        }
        ImGui::EndPopup();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Left-click: press any key or scroll / click mouse wheel\nRight-click: quick wheel bind menu\nESC: cancel");
    }
}

Menu& Menu::Get() {
    static Menu instance;
    return instance;
}

void Menu::Render() {
    auto& cfg = Core::Config::Get();
    if (!cfg.menu.isVisible) return;

    ImGui::SetNextWindowSize(ImVec2(480, 680), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("R.E.P.O Internal [DEL / INS]", &cfg.menu.isVisible)) {
        if (ImGui::CollapsingHeader("God Mode (Invulnerability)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable God Mode", &cfg.godMode.enabled);

            if (cfg.godMode.enabled) {
                static bool s_BindingGodKey = false;
                static bool s_WaitReleaseGod = false;
                DrawKeybindWidget("GodMode", cfg.godMode.toggleKey, s_BindingGodKey, s_WaitReleaseGod, 140.0f);

                ImGui::SameLine();
                ImGui::RadioButton("Hold##God", &cfg.godMode.keyMode, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Toggle##God", &cfg.godMode.keyMode, 1);

                ImGui::SameLine();
                if (cfg.godMode.active) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ACTIVE]");
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[IDLE]");
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Protection Options");
                ImGui::Checkbox("Invulnerability (Block All Damage)", &cfg.godMode.preventDeath);
                ImGui::Checkbox("Infinite Health (Auto Heal to 100%)", &cfg.godMode.healToMax);
                ImGui::Checkbox("Immunity to Fall Damage", &cfg.godMode.noFallDamage);
                ImGui::Checkbox("Auto Revive on Death", &cfg.godMode.autoRevive);

                ImGui::Spacing();
                ImGui::SeparatorText("Actions");
                if (ImGui::Button("Instant Full Heal", ImVec2(150, 24))) {
                    Game::Runtime::Get().Queue(Game::Action::Heal);
                }
                ImGui::SameLine();
                if (ImGui::Button("Revive Self (If Dead)", ImVec2(160, 24))) {
                    Game::Runtime::Get().Queue(Game::Action::Revive);
                }
            }
        }

        if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("NoClip");
            ImGui::Checkbox("Enable NoClip", &cfg.noclip.enabled);

            if (cfg.noclip.enabled) {
                static bool s_BindingNoClipKey = false;
                static bool s_WaitReleaseNoClip = false;
                DrawKeybindWidget("NoClip", cfg.noclip.toggleKey, s_BindingNoClipKey, s_WaitReleaseNoClip, 140.0f);

                ImGui::SameLine();
                ImGui::RadioButton("Hold##NC", &cfg.noclip.keyMode, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Toggle##NC", &cfg.noclip.keyMode, 1);

                ImGui::SameLine();
                if (cfg.noclip.active) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ACTIVE]");
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[IDLE]");
                }

                ImGui::SliderFloat("Flight Speed", &cfg.noclip.speed, 2.0f, 60.0f, "%.1f m/s");
                ImGui::SliderFloat("Sprint Multiplier", &cfg.noclip.sprintMultiplier, 1.0f, 5.0f, "%.1fx");
                ImGui::Checkbox("Through Walls", &cfg.noclip.disableColliders);
                ImGui::Checkbox("Freeze When Idle", &cfg.noclip.freezeWhenIdle);
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Speedhack");
            ImGui::Checkbox("Enable Speedhack", &cfg.movement.speedEnabled);

            if (cfg.movement.speedEnabled) {
                ImGui::Checkbox("Always Active (No Keybind / Constant)", &cfg.movement.speedAlwaysOn);

                if (!cfg.movement.speedAlwaysOn) {
                    static bool s_BindingSpeedKey = false;
                    static bool s_WaitReleaseSpeed = false;
                    DrawKeybindWidget("Speed", cfg.movement.speedKey, s_BindingSpeedKey, s_WaitReleaseSpeed, 140.0f);

                    ImGui::SameLine();
                    ImGui::RadioButton("Hold", &cfg.movement.speedKeyMode, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("Toggle", &cfg.movement.speedKeyMode, 1);

                    ImGui::SameLine();
                    if (cfg.movement.speedActive) {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ACTIVE]");
                    } else {
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[IDLE]");
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ACTIVE - ALWAYS ON]");
                }

                ImGui::SliderFloat("Speed Multiplier", &cfg.movement.speedMultiplier, 1.0f, 8.0f, "%.1fx");
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Stamina & Strength");
            ImGui::Checkbox("Infinite Stamina", &cfg.movement.infiniteStamina);
            ImGui::Checkbox("Infinite Strength", &cfg.movement.infiniteStrength);
            if (cfg.movement.infiniteStrength) {
                ImGui::SliderFloat("Strength Level", &cfg.movement.strengthMultiplier, 5.0f, 100.0f, "%.0fx");
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Extra Utilities");
            ImGui::Checkbox("Extend Grab Range", &cfg.movement.extendGrabRange);
            if (cfg.movement.extendGrabRange) {
                ImGui::SliderFloat("Grab Range", &cfg.movement.grabRange, 4.0f, 30.0f, "%.0f m");
            }

            ImGui::Checkbox("Super Jump", &cfg.movement.superJump);
            if (cfg.movement.superJump) {
                ImGui::SliderFloat("Jump Multiplier", &cfg.movement.jumpMultiplier, 1.0f, 4.0f, "%.1fx");
            }

            ImGui::Checkbox("No Ragdoll", &cfg.movement.noTumble);
        }

        if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("Third Person View");
            ImGui::Checkbox("Enable Third Person View", &cfg.thirdPerson.enabled);

            if (cfg.thirdPerson.enabled) {
                static bool s_BindingTPKey = false;
                static bool s_WaitReleaseTP = false;
                DrawKeybindWidget("ThirdPerson", cfg.thirdPerson.toggleKey, s_BindingTPKey, s_WaitReleaseTP, 140.0f);

                ImGui::SameLine();
                ImGui::RadioButton("Hold##TP", &cfg.thirdPerson.keyMode, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Toggle##TP", &cfg.thirdPerson.keyMode, 1);

                ImGui::SameLine();
                if (cfg.thirdPerson.active) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ACTIVE]");
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[IDLE]");
                }

                ImGui::SliderFloat("Camera Distance", &cfg.thirdPerson.distance, 1.0f, 6.0f, "%.1f m");
                ImGui::SliderFloat("Shoulder Offset", &cfg.thirdPerson.shoulderOffset, -1.5f, 1.5f, "%.2f m");
                ImGui::SliderFloat("Height Offset", &cfg.thirdPerson.heightOffset, -0.5f, 1.5f, "%.2f m");
                ImGui::Checkbox("Anti-Clip Wall Collision", &cfg.thirdPerson.collision);

                ImGui::Spacing();
                ImGui::SeparatorText("View Presets");
                if (ImGui::Button("Right Shoulder")) {
                    cfg.thirdPerson.distance = 2.5f;
                    cfg.thirdPerson.shoulderOffset = 0.45f;
                    cfg.thirdPerson.heightOffset = 0.20f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Left Shoulder")) {
                    cfg.thirdPerson.distance = 2.5f;
                    cfg.thirdPerson.shoulderOffset = -0.45f;
                    cfg.thirdPerson.heightOffset = 0.20f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Centered")) {
                    cfg.thirdPerson.distance = 2.8f;
                    cfg.thirdPerson.shoulderOffset = 0.0f;
                    cfg.thirdPerson.heightOffset = 0.35f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cinematic Wide")) {
                    cfg.thirdPerson.distance = 4.2f;
                    cfg.thirdPerson.shoulderOffset = 0.60f;
                    cfg.thirdPerson.heightOffset = 0.45f;
                }
            }

            ImGui::Spacing();

            ImGui::SeparatorText("Screen Crosshair");
            ImGui::Checkbox("Draw Crosshair", &cfg.visual.drawCrosshair);
            if (cfg.visual.drawCrosshair) {
                ImGui::SliderFloat("Crosshair Size", &cfg.visual.crosshairSize, 2.0f, 20.0f, "%.0f px");
                ImGui::ColorEdit4("Crosshair Color", cfg.visual.crosshairColor, ImGuiColorEditFlags_NoInputs);
            }
        }

        if (ImGui::CollapsingHeader("Removals", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable Removals", &cfg.removals.enabled);
            ImGui::BeginDisabled(!cfg.removals.enabled);
            ImGui::SeparatorText("Environment");
            ImGui::Checkbox("No Fog", &cfg.removals.noFog);
            ImGui::SeparatorText("Post-Processing");
            ImGui::Checkbox("Disable All Post-Processing", &cfg.removals.noPostProcessing);
            ImGui::BeginDisabled(cfg.removals.noPostProcessing);
            ImGui::Checkbox("No Film Grain", &cfg.removals.noGrain);
            ImGui::Checkbox("No Vignette", &cfg.removals.noVignette);
            ImGui::Checkbox("No Bloom", &cfg.removals.noBloom);
            ImGui::Checkbox("No Motion Blur", &cfg.removals.noMotionBlur);
            ImGui::Checkbox("No Lens Distortion", &cfg.removals.noLensDistortion);
            ImGui::Checkbox("No Chromatic Aberration", &cfg.removals.noChromaticAberration);
            ImGui::EndDisabled();
            ImGui::SeparatorText("Camera & Screen Effects");
            ImGui::Checkbox("No Camera Shake", &cfg.removals.noCameraShake);
            ImGui::Checkbox("No Camera Sway", &cfg.removals.noCameraNoise);
            ImGui::Checkbox("No Camera Glitches", &cfg.removals.noCameraGlitch);
            ImGui::Checkbox("No Damage Overlay", &cfg.removals.noHurtOverlay);
            ImGui::EndDisabled();
            ImGui::TextWrapped("Switch off Removals to restore the game's visual effects.");
            if (cfg.removals.enabled && !Features::Removals::Get().IsPostProcessingReady())
                ImGui::TextWrapped("Post-processing controls are unavailable in this session.");
        }

        if (ImGui::CollapsingHeader("Fullbright", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable Fullbright", &cfg.fullbright.enabled);
            ImGui::BeginDisabled(!cfg.fullbright.enabled);
            ImGui::SliderFloat("Brightness", &cfg.fullbright.brightness, 0.1f, 3.0f, "%.2f");
            ImGui::EndDisabled();
            ImGui::TextWrapped("Keeps ambient lighting bright in every room. Works independently of Removals.");
            if (cfg.fullbright.enabled && !Features::Removals::Get().IsLightingReady())
                ImGui::TextWrapped("Lighting controls are unavailable in this session.");
        }

        if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("General Settings");
            float maxDist = cfg.monster.maxDistance;
            if (ImGui::SliderFloat("Max Distance", &maxDist, 10.0f, 500.0f, "%.0f m")) {
                cfg.monster.maxDistance = maxDist;
                cfg.player.maxDistance = maxDist;
                cfg.valuable.maxDistance = maxDist;
                cfg.item.maxDistance = maxDist;
            }
            ImGui::Checkbox("Draw Text Background Box", &cfg.visual.textBackground);

            ImGui::Spacing();

            ImGui::SeparatorText("Monsters ESP");
            ImGui::Checkbox("Enable Monster ESP", &cfg.monster.enabled);
            if (cfg.monster.enabled) {
                if (ImGui::Checkbox("2D Box##M", &cfg.monster.box2D)) {
                    if (cfg.monster.box2D) cfg.monster.cornerBox = false;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("Corner Box##M", &cfg.monster.cornerBox)) {
                    if (cfg.monster.cornerBox) cfg.monster.box2D = false;
                }
                ImGui::SameLine();
                ImGui::Checkbox("Snaplines##M", &cfg.monster.snaplines);

                ImGui::Checkbox("Name##M", &cfg.monster.showName);
                ImGui::SameLine();
                ImGui::Checkbox("Distance##MD", &cfg.monster.showDistance);
                ImGui::SameLine();
                ImGui::Checkbox("Highlight Chasing", &cfg.monster.highlightChasing);

                ImGui::ColorEdit4("Monster Color", cfg.monster.colorDefault, ImGuiColorEditFlags_NoInputs);
            }

            ImGui::Spacing();

            ImGui::SeparatorText("Player ESP (Teammates)");
            ImGui::Checkbox("Enable Player ESP", &cfg.player.enabled);
            if (cfg.player.enabled) {
                if (ImGui::Checkbox("2D Box##P", &cfg.player.box2D)) {
                    if (cfg.player.box2D) cfg.player.cornerBox = false;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("Corner Box##P", &cfg.player.cornerBox)) {
                    if (cfg.player.cornerBox) cfg.player.box2D = false;
                }
                ImGui::SameLine();
                ImGui::Checkbox("Snaplines##P", &cfg.player.snaplines);

                ImGui::Checkbox("Name##P", &cfg.player.showName);
                ImGui::SameLine();
                ImGui::Checkbox("Distance##P", &cfg.player.showDistance);
                ImGui::SameLine();
                ImGui::Checkbox("Health (HP)##P", &cfg.player.showHealth);

                ImGui::Checkbox("Hide Local Player", &cfg.player.hideLocalPlayer);

                ImGui::ColorEdit4("Player Color", cfg.player.color, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Dead Color", cfg.player.colorDead, ImGuiColorEditFlags_NoInputs);
            }

            ImGui::Spacing();

            ImGui::SeparatorText("Items & Loot ESP");
            bool lootEnabled = cfg.valuable.enabled || cfg.item.enabled;
            if (ImGui::Checkbox("Enable Items / Valuables ESP", &lootEnabled)) {
                cfg.valuable.enabled = lootEnabled;
                cfg.item.enabled = lootEnabled;
            }
            if (lootEnabled) {
                ImGui::Checkbox("Item Name", &cfg.valuable.showName);
                cfg.item.showName = cfg.valuable.showName;
                ImGui::SameLine();
                ImGui::Checkbox("Item Price ($)", &cfg.valuable.showValue);
                cfg.item.showValue = cfg.valuable.showValue;

                ImGui::Checkbox("Distance##I", &cfg.valuable.showDistance);
                cfg.item.showDistance = cfg.valuable.showDistance;

                ImGui::SameLine();
                ImGui::Checkbox("Hide Extracted", &cfg.valuable.hideExtracted);

                ImGui::ColorEdit4("Loot Color ($)", cfg.valuable.color, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Item Color", cfg.item.color, ImGuiColorEditFlags_NoInputs);
            }
        }

        if (ImGui::CollapsingHeader("Spectator List", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("Window Settings");
            ImGui::Checkbox("Enable Spectator List Window", &cfg.spectatorList.enabled);

            if (cfg.spectatorList.enabled) {
                ImGui::Checkbox("Show Watching Target (-> Player / -> YOU)", &cfg.spectatorList.showTarget);
                ImGui::SameLine();
                ImGui::Checkbox("Show View Mode ([3rd Person] / [Head])", &cfg.spectatorList.showMode);

                ImGui::Checkbox("Hide When Empty (Auto-Hide)", &cfg.spectatorList.hideWhenEmpty);
                ImGui::SameLine();
                ImGui::Checkbox("Show Spectator Count in Title", &cfg.spectatorList.showSpectatorCount);

                ImGui::Spacing();
                ImGui::SeparatorText("Spectator List Colors");
                ImGui::ColorEdit4("Header Color", cfg.spectatorList.colorHeader, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Spectator Name", cfg.spectatorList.colorSpectator, ImGuiColorEditFlags_NoInputs);

                ImGui::ColorEdit4("Target: YOU (Alert)", cfg.spectatorList.colorTargetMe, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Target: Other", cfg.spectatorList.colorTargetOther, ImGuiColorEditFlags_NoInputs);
            }
        }

        if (ImGui::CollapsingHeader("Trolling", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("Voice & Audio");
            ImGui::Checkbox("Global Voice", &cfg.troll.globalVoice);
            if (cfg.troll.globalVoice) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[Direct Map Audio]");
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Physics (Self)");
            ImGui::Checkbox("Moon Gravity (Feather Fall)", &cfg.troll.featherFall);
            ImGui::SameLine();
            ImGui::Checkbox("Zero Gravity (Weightless Float)", &cfg.troll.zeroGravity);

            ImGui::Spacing();
            ImGui::SeparatorText("Animation (Visible to Everyone)");
            ImGui::Checkbox("Override Animation Speed", &cfg.troll.overrideAnimSpeed);
            if (cfg.troll.overrideAnimSpeed) {
                ImGui::SliderFloat("Anim Speed", &cfg.troll.animSpeed, 0.1f, 5.0f, "%.1fx");
                if (ImGui::Button("Matrix (0.2x)")) cfg.troll.animSpeed = 0.2f;
                ImGui::SameLine();
                if (ImGui::Button("Normal (1.0x)")) cfg.troll.animSpeed = 1.0f;
                ImGui::SameLine();
                if (ImGui::Button("Scooby-Doo (3.0x)")) cfg.troll.animSpeed = 3.0f;
                ImGui::SameLine();
                if (ImGui::Button("Sonic (5.0x)")) cfg.troll.animSpeed = 5.0f;
            }
        }

        if (ImGui::CollapsingHeader("Cosmetic Parser", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& gs = Game::GameState::Get();
            std::string resultMessage;
            if (gs.TakeCosmeticResult(cfg.cosmeticParser.actionSuccess, resultMessage)) {
                cfg.cosmeticParser.hasStatus = true;
                sprintf_s(cfg.cosmeticParser.statusMessage, "%s", resultMessage.c_str());
            }
            auto queueAppearance = [&](uintptr_t target, bool cosmetics, bool colors) {
                bool queued = gs.QueueCosmeticChange(target, cosmetics, colors);
                cfg.cosmeticParser.hasStatus = true;
                cfg.cosmeticParser.actionSuccess = false;
                sprintf_s(cfg.cosmeticParser.statusMessage, "%s", queued
                    ? "Applying on next game update..." : "[-] Game update unavailable. Re-enter the session or reload the DLL.");
            };

            auto state = Game::Runtime::Get().ReadState();
            if (GetTickCount64() - state.timestamp > 1000) state.players.clear();
            std::vector<Game::ServerPlayerInfo> serverPlayers = std::move(state.players);
            for (auto it = serverPlayers.begin(); it != serverPlayers.end();) {
                if (it->isLocal) it = serverPlayers.erase(it);
                else ++it;
            }
            if (serverPlayers.empty()) {
                cfg.cosmeticParser.selectedPlayerAddress = 0;
                cfg.cosmeticParser.selectedPlayerIdx = -1;
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No players detected. Enter a lobby or game to scan players.");
            } else {
                std::vector<const char*> playerNames;
                static char s_Labels[32][128];

                int displayIdx = 0;
                for (size_t i = 0; i < serverPlayers.size() && displayIdx < 32; ++i) {
                    const auto& p = serverPlayers[i];
                    snprintf(s_Labels[displayIdx], sizeof(s_Labels[displayIdx]), "%s%s (HP: %d)##%zu",
                        p.name.c_str(),
                        p.isLocal ? " [YOU]" : "",
                        p.health,
                        i);
                    playerNames.push_back(s_Labels[displayIdx]);
                    displayIdx++;
                }

                // Keep the same player selected when the server list is reordered.
                cfg.cosmeticParser.selectedPlayerIdx = -1;
                for (size_t i = 0; i < playerNames.size(); ++i) {
                    if (serverPlayers[i].address == cfg.cosmeticParser.selectedPlayerAddress)
                        cfg.cosmeticParser.selectedPlayerIdx = static_cast<int>(i);
                }
                if (cfg.cosmeticParser.selectedPlayerIdx < 0) cfg.cosmeticParser.selectedPlayerAddress = 0;

                ImGui::SetNextItemWidth(300.0f);
                bool targetChanged = ImGui::Combo("Select Target Player", &cfg.cosmeticParser.selectedPlayerIdx,
                    playerNames.data(), static_cast<int>(playerNames.size()));

                uintptr_t targetAddr = 0;
                if (cfg.cosmeticParser.selectedPlayerIdx >= 0 && cfg.cosmeticParser.selectedPlayerIdx < static_cast<int>(serverPlayers.size())) {
                    targetAddr = serverPlayers[cfg.cosmeticParser.selectedPlayerIdx].address;
                }
                if (targetChanged && targetAddr) {
                    cfg.cosmeticParser.selectedPlayerAddress = targetAddr;
                }

                ImGui::Spacing();
                ImGui::BeginDisabled(targetAddr == 0);
                if (ImGui::Button("Copy Cosmetics + Colors", ImVec2(-1, 32.0f))) {
                    if (targetAddr) {
                        queueAppearance(targetAddr, true, true);
                    }
                }

                if (ImGui::Button("Copy Cosmetics Only", ImVec2(150, 24.0f))) {
                    if (targetAddr) {
                        queueAppearance(targetAddr, true, false);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Copy Colors Only", ImVec2(130, 24.0f))) {
                    if (targetAddr) {
                        queueAppearance(targetAddr, false, true);
                    }
                }
                ImGui::EndDisabled();
            }

            if (cfg.cosmeticParser.hasStatus) {
                ImGui::Spacing();
                if (cfg.cosmeticParser.actionSuccess) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "%s", cfg.cosmeticParser.statusMessage);
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", cfg.cosmeticParser.statusMessage);
                }
            }
        }

        if (ImGui::CollapsingHeader("Unlocks", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("Cosmetics Unlocker");
            static int s_UnlockedCount = 0;
            static int s_TotalCount = 0;
            static bool s_UnlockAttempted = false;
            static bool s_UnlockSuccess = false;
            static char s_StatusMessage[128] = "";

            auto state = Game::Runtime::Get().ReadState();
            int currentUnlocked = state.unlocked, currentTotal = state.total;
            bool hasStats = state.hasCosmeticStats && GetTickCount64() - state.timestamp < 1000;
            if (hasStats) {
                s_UnlockedCount = currentUnlocked;
                s_TotalCount = currentTotal;
            }

            if (s_TotalCount > 0) {
                float frac = (s_TotalCount > 0) ? ((float)s_UnlockedCount / (float)s_TotalCount) : 0.0f;
                if (frac > 1.0f) frac = 1.0f;
                char buf[64];
                sprintf_s(buf, "%d / %d (%.0f%%)", s_UnlockedCount, s_TotalCount, frac * 100.0f);
                ImGui::Text("Unlocked Cosmetics:");
                ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f), buf);
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Status: [ Enter main menu or lobby to load ]");
            }

            ImGui::Spacing();
            bool unlockResult = false;
            if (Game::Runtime::Get().TakeResult(Game::Action::UnlockCosmetics, unlockResult)) {
                s_UnlockSuccess = unlockResult;
                sprintf_s(s_StatusMessage, "%s", unlockResult ? "[+] Cosmetics unlocked and saved." : "[-] Unlock failed; wait until the session is ready.");
            }
            if (ImGui::Button("Unlock All Cosmetics Permanently", ImVec2(-1.0f, 32.0f))) {
                s_UnlockAttempted = true;
                s_UnlockSuccess = false;
                bool queued = Game::Runtime::Get().Queue(Game::Action::UnlockCosmetics);
                sprintf_s(s_StatusMessage, "%s", queued ? "Applying on game update..." : "[-] Game update unavailable.");
            }

            if (s_UnlockAttempted) {
                if (s_UnlockSuccess) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "%s", s_StatusMessage);
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", s_StatusMessage);
                }
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Permanently unlocks all hats, skins, and outfits to your save file.");
            }
        }

        if (ImGui::CollapsingHeader("Settings")) {
            ImGui::SeparatorText("Keybinds");
            ImGui::Text("Menu Toggle Key:");
            ImGui::SameLine();

            static bool s_BindingMenuKey = false;
            static bool s_WaitReleaseMenu = false;
            int menuKey = static_cast<int>(cfg.menu.toggleKey);
            DrawKeybindWidget("MenuToggle", menuKey, s_BindingMenuKey, s_WaitReleaseMenu, 140.0f);
            cfg.menu.toggleKey = static_cast<DWORD>(menuKey);

            ImGui::SameLine();
            if (ImGui::Button("DEL##ResetMenuKey", ImVec2(40, 24))) {
                cfg.menu.toggleKey = VK_DELETE;
                s_BindingMenuKey = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("INS##ResetMenuKey", ImVec2(40, 24))) {
                cfg.menu.toggleKey = VK_INSERT;
                s_BindingMenuKey = false;
            }

            ImGui::TextDisabled("Note: DELETE and INSERT will always toggle the menu as safe fallbacks.");

            ImGui::Spacing();
            ImGui::SeparatorText("Unload");
            if (ImGui::Button("Safe Unload / Eject Cheat", ImVec2(-1, 24))) {
                cfg.menu.requestUnload = true;
            }
        }

        ImGui::Separator();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::TextDisabled("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / (io.Framerate > 0.0f ? io.Framerate : 1.0f));

        ImGui::End();
    }
}

}
