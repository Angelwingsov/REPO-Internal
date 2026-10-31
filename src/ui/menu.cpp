#include "menu.h"
#include "imgui/imgui.h"
#include "core/config.h"
#include "mono/mono_bridge.h"
#include "game/state.h"
#include "features/player.h"

namespace UI {

Menu& Menu::Get() {
    static Menu instance;
    return instance;
}

void Menu::Render() {
    auto& cfg = Core::Config::Get();
    if (!cfg.menu.isVisible) return;

    ImGui::SetNextWindowSize(ImVec2(460, 620), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("R.E.P.O Internal [DEL / INS]", &cfg.menu.isVisible)) {
        if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("Speedhack");
            ImGui::Checkbox("Enable Speedhack", &cfg.movement.speedEnabled);

            if (cfg.movement.speedEnabled) {
                ImGui::Checkbox("Always Active (No Keybind / Constant)", &cfg.movement.speedAlwaysOn);

                if (!cfg.movement.speedAlwaysOn) {
                    static bool s_BindingSpeedKey = false;
                    static bool s_WaitRelease = false;
                    char keyBtnText[64];
                    if (s_BindingSpeedKey) {
                        strcpy_s(keyBtnText, "[ Press Any Key... ]");
                        if (s_WaitRelease) {
                            if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                                s_WaitRelease = false;
                            }
                        } else {
                            for (int k = 1; k < 255; ++k) {
                                if (k == VK_ESCAPE) {
                                    s_BindingSpeedKey = false;
                                    break;
                                }
                                if (GetAsyncKeyState(k) & 0x8000) {
                                    cfg.movement.speedKey = k;
                                    s_BindingSpeedKey = false;
                                    break;
                                }
                            }
                        }
                    } else {
                        sprintf_s(keyBtnText, "Key: [ %s ]##Speed", Features::PlayerMods::GetKeyName(cfg.movement.speedKey));
                    }

                    if (ImGui::Button(keyBtnText, ImVec2(130, 24))) {
                        s_BindingSpeedKey = true;
                        s_WaitRelease = true;
                    }

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
                char tpKeyBtnText[64];
                if (s_BindingTPKey) {
                    strcpy_s(tpKeyBtnText, "[ Press Any Key... ]##TP");
                    if (s_WaitReleaseTP) {
                        if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                            s_WaitReleaseTP = false;
                        }
                    } else {
                        for (int k = 1; k < 255; ++k) {
                            if (k == VK_ESCAPE) {
                                s_BindingTPKey = false;
                                break;
                            }
                            if (GetAsyncKeyState(k) & 0x8000) {
                                cfg.thirdPerson.toggleKey = k;
                                s_BindingTPKey = false;
                                break;
                            }
                        }
                    }
                } else {
                    sprintf_s(tpKeyBtnText, "Key: [ %s ]##ThirdPerson", Features::PlayerMods::GetKeyName(cfg.thirdPerson.toggleKey));
                }

                if (ImGui::Button(tpKeyBtnText, ImVec2(130, 24))) {
                    s_BindingTPKey = true;
                    s_WaitReleaseTP = true;
                }

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

        if (ImGui::CollapsingHeader("Unlocks", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SeparatorText("Cosmetics Unlocker");
            static int s_UnlockedCount = 0;
            static int s_TotalCount = 0;
            static bool s_UnlockAttempted = false;
            static bool s_UnlockSuccess = false;
            static char s_StatusMessage[128] = "";

            auto& gs = Game::GameState::Get();
            int currentUnlocked = 0, currentTotal = 0;
            bool hasStats = gs.GetCosmeticStats(currentUnlocked, currentTotal);
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
            if (ImGui::Button("Unlock All Cosmetics Permanently", ImVec2(-1.0f, 32.0f))) {
                s_UnlockAttempted = true;
                int u = 0, t = 0;
                if (gs.UnlockAllCosmetics(&u, &t)) {
                    s_UnlockSuccess = true;
                    s_UnlockedCount = u;
                    s_TotalCount = t;
                    sprintf_s(s_StatusMessage, "[+] Success! Unlocked and saved %d items!", u);
                } else {
                    s_UnlockSuccess = false;
                    sprintf_s(s_StatusMessage, "[-] Error: MetaManager is not loaded yet.");
                }
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
            const char* keyNames[] = { "DELETE", "INSERT", "F11" };
            int currentKeyIdx = (cfg.menu.toggleKey == VK_INSERT) ? 1 : (cfg.menu.toggleKey == VK_F11 ? 2 : 0);
            if (ImGui::Combo("##ToggleKey", &currentKeyIdx, keyNames, IM_ARRAYSIZE(keyNames))) {
                if (currentKeyIdx == 0) cfg.menu.toggleKey = VK_DELETE;
                else if (currentKeyIdx == 1) cfg.menu.toggleKey = VK_INSERT;
                else if (currentKeyIdx == 2) cfg.menu.toggleKey = VK_F11;
            }

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
