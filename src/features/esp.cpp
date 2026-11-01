#include "esp.h"
#include "ui/theme.h"
#include <algorithm>
#include "core/config.h"
#include "core/logger.h"
#include "unity/unity_engine.h"
#include "game/state.h"
#include "game/entities.h"

namespace Features {

ESP& ESP::Get() {
    static ESP instance;
    return instance;
}

void ESP::DrawCornerBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 col, float thickness) {
    float lineW = w * 0.25f;
    float lineH = h * 0.25f;
    ImU32 outlineCol = IM_COL32(0, 0, 0, 220);
    float outlineThick = thickness + 1.5f;

    drawList->AddLine(ImVec2(x - 1, y - 1), ImVec2(x + lineW + 1, y - 1), outlineCol, outlineThick);
    drawList->AddLine(ImVec2(x - 1, y - 1), ImVec2(x - 1, y + lineH + 1), outlineCol, outlineThick);

    drawList->AddLine(ImVec2(x + w + 1, y - 1), ImVec2(x + w - lineW - 1, y - 1), outlineCol, outlineThick);
    drawList->AddLine(ImVec2(x + w + 1, y - 1), ImVec2(x + w + 1, y + lineH + 1), outlineCol, outlineThick);

    drawList->AddLine(ImVec2(x - 1, y + h + 1), ImVec2(x + lineW + 1, y + h + 1), outlineCol, outlineThick);
    drawList->AddLine(ImVec2(x - 1, y + h + 1), ImVec2(x - 1, y + h - lineH - 1), outlineCol, outlineThick);

    drawList->AddLine(ImVec2(x + w + 1, y + h + 1), ImVec2(x + w - lineW - 1, y + h + 1), outlineCol, outlineThick);
    drawList->AddLine(ImVec2(x + w + 1, y + h + 1), ImVec2(x + w + 1, y + h - lineH - 1), outlineCol, outlineThick);

    drawList->AddLine(ImVec2(x, y), ImVec2(x + lineW, y), col, thickness);
    drawList->AddLine(ImVec2(x, y), ImVec2(x, y + lineH), col, thickness);

    drawList->AddLine(ImVec2(x + w, y), ImVec2(x + w - lineW, y), col, thickness);
    drawList->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + lineH), col, thickness);

    drawList->AddLine(ImVec2(x, y + h), ImVec2(x + lineW, y + h), col, thickness);
    drawList->AddLine(ImVec2(x, y + h), ImVec2(x, y + h - lineH), col, thickness);

    drawList->AddLine(ImVec2(x + w, y + h), ImVec2(x + w - lineW, y + h), col, thickness);
    drawList->AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - lineH), col, thickness);
}

void ESP::Draw2DBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 col, float thickness) {
    drawList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), IM_COL32(0, 0, 0, 220), 0.0f, thickness + 1.0f, 0);
    drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), col, 0.0f, thickness, 0);
}

void ESP::DrawTextPill(ImDrawList* drawList, const ImVec2& centerPos, ImU32 textColor, const char* text, float bgAlpha) {
    if (!text || *text == '\0') return;

    ImFont* font = UI::Theme::TextFont();
    const float fontSize = font->LegacySize;
    ImVec2 sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0, text);
    float padX = 4.0f;
    float padY = 2.0f;

    ImVec2 minPos(centerPos.x - sz.x * 0.5f - padX, centerPos.y - sz.y * 0.5f - padY);
    ImVec2 maxPos(centerPos.x + sz.x * 0.5f + padX, centerPos.y + sz.y * 0.5f + padY);

    if (bgAlpha > 0.01f) {
        int alphaByte = static_cast<int>(bgAlpha * 255.0f);
        if (alphaByte > 255) alphaByte = 255;
        drawList->AddRectFilled(minPos, maxPos, IM_COL32(10, 10, 10, alphaByte), 3.0f);
        drawList->AddRect(minPos, maxPos, IM_COL32(0, 0, 0, 180), 3.0f, 1.0f, 0);
    }

    drawList->AddText(font, fontSize, ImVec2(centerPos.x - sz.x * 0.5f, centerPos.y - sz.y * 0.5f), textColor, text);
}

void ESP::DrawCrosshair(ImDrawList* drawList, float screenW, float screenH) {
    auto& cfg = Core::Config::Get().visual;
    float midX = screenW * 0.5f;
    float midY = screenH * 0.5f;
    float sz = cfg.crosshairSize;
    ImU32 col = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(cfg.crosshairColor));

    drawList->AddLine(ImVec2(midX - sz - 1, midY), ImVec2(midX + sz + 1, midY), IM_COL32(0, 0, 0, 200), 2.5f);
    drawList->AddLine(ImVec2(midX, midY - sz - 1), ImVec2(midX, midY + sz + 1), IM_COL32(0, 0, 0, 200), 2.5f);

    drawList->AddLine(ImVec2(midX - sz, midY), ImVec2(midX + sz, midY), col, 1.5f);
    drawList->AddLine(ImVec2(midX, midY - sz), ImVec2(midX, midY + sz), col, 1.5f);
}

void ESP::Update(float screenWidth, float screenHeight) {
    m_Frame.Publish(Collect(screenWidth, screenHeight));
}

ESP::Frame ESP::Collect(float screenWidth, float screenHeight) {
    Frame frame;
    frame.width = screenWidth;
    frame.height = screenHeight;
    frame.timestamp = GetTickCount64();
    auto& config = Core::Config::Get();
    config.diagnostics.monstersFound = config.diagnostics.playersFound = 0;
    config.diagnostics.valuablesFound = config.diagnostics.itemsFound = 0;
    config.diagnostics.cameraValid = false;
    config.diagnostics.cameraSource = "None";
    config.diagnostics.mapLoading = Game::GameState::Get().IsMapLoadingOrRestarting();
    if (config.diagnostics.mapLoading || screenWidth < 50.0f || screenHeight < 50.0f) { Game::EntityManager::Get().ClearCache(); return frame; }
    auto& engine = Unity::Engine::Get();
    const char* source = "None";
    MonoObject* camera = engine.FindActiveCamera(&source);
    config.diagnostics.cameraSource = source;
    config.diagnostics.cameraValid = camera != nullptr;
    if (!camera) return frame;
    engine.UpdateFrameCameraMetrics(camera, screenWidth, screenHeight);
    auto position = engine.GetPlayerOrCameraPosition(camera);
    MonoObject* camTransform = engine.GetTransform(camera);
    Unity::Vector3 camForward = camTransform ? engine.GetTransformForward(camTransform) : Unity::Vector3{};

    auto& entities = Game::EntityManager::Get();
    if (config.monster.enabled) entities.CollectMonsters(camera, position, screenWidth, screenHeight, config.monster.maxDistance, frame.monsters);
    if (config.player.enabled) entities.CollectPlayers(camera, position, screenWidth, screenHeight, config.player.maxDistance, config.player.hideLocalPlayer, frame.players);
    if (config.valuable.enabled) entities.CollectValuables(camera, position, screenWidth, screenHeight, config.valuable.maxDistance, config.valuable.hideExtracted, config.valuable.minDollarValue, frame.valuables, camForward);
    if (config.item.enabled) entities.CollectItems(camera, position, screenWidth, screenHeight, config.item.maxDistance, frame.items, camForward);
    config.diagnostics.monstersFound = static_cast<int>(frame.monsters.size());
    config.diagnostics.playersFound = static_cast<int>(frame.players.size());
    config.diagnostics.valuablesFound = static_cast<int>(frame.valuables.size());
    config.diagnostics.itemsFound = static_cast<int>(frame.items.size());
    frame.available = true;
    return frame;
}

void ESP::Render(ImDrawList* drawList, float screenWidth, float screenHeight) {
    if (!drawList || screenWidth < 50.0f || screenHeight < 50.0f) return;
    auto frame = m_Frame.Read();
    if (!frame.available || GetTickCount64() - frame.timestamp > 1000 ||
        frame.width != screenWidth || frame.height != screenHeight) return;
    auto& config = Core::Config::Get();
    if (config.visual.drawCrosshair) {
        DrawCrosshair(drawList, screenWidth, screenHeight);
    }

    float bgAlpha = config.visual.textBackground ? config.visual.textBgAlpha : 0.0f;

    if (config.monster.enabled) {
        auto& monsters = frame.monsters;
        config.diagnostics.monstersFound = static_cast<int>(monsters.size());

        std::sort(monsters.begin(), monsters.end(), [](const Game::MonsterEntity& a, const Game::MonsterEntity& b) {
            return a.distance > b.distance;
        });

        for (const auto& monster : monsters) {
            ImU32 enemyColor;
            if (monster.isChasing && config.monster.highlightChasing) {
                enemyColor = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(config.monster.colorChasing));
            } else if (monster.isStunned && config.monster.showStunned) {
                enemyColor = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(config.monster.colorStunned));
            } else {
                enemyColor = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(config.monster.colorDefault));
            }

            float textY = monster.screenPosition.y - 12.0f;

            if (monster.hasScreenBox && (config.monster.box2D || config.monster.cornerBox)) {
                float h = std::abs(monster.bottomScreenPosition.y - monster.topScreenPosition.y);
                if (h < 8.0f) h = 8.0f;
                float w = h * 0.55f;
                float x = monster.screenPosition.x - (w * 0.5f);
                float y = monster.topScreenPosition.y;
                textY = y - 10.0f;

                if (config.monster.box2D) {
                    Draw2DBox(drawList, x, y, w, h, enemyColor, config.monster.boxThickness);
                } else if (config.monster.cornerBox) {
                    DrawCornerBox(drawList, x, y, w, h, enemyColor, config.monster.boxThickness);
                }

                if (config.monster.snaplines) {
                    drawList->AddLine(ImVec2(screenWidth * 0.5f, screenHeight),
                                      ImVec2(monster.screenPosition.x, monster.bottomScreenPosition.y),
                                      enemyColor, config.visual.snaplineThickness);
                }
            } else if (config.monster.snaplines) {
                drawList->AddLine(ImVec2(screenWidth * 0.5f, screenHeight),
                                  ImVec2(monster.screenPosition.x, monster.screenPosition.y),
                                  enemyColor, config.visual.snaplineThickness);
            }

            if (config.monster.showName || config.monster.showDistance) {
                std::string badge;
                if (monster.isStunned && config.monster.showStunned) badge = " [STUNNED]";
                else if (monster.isChasing && config.monster.highlightChasing) badge = " [CHASE]";

                char buf[128];
                if (config.monster.showName && config.monster.showDistance) {
                    snprintf(buf, sizeof(buf), "%s%s [%.0fm]", monster.name.c_str(), badge.c_str(), monster.distance);
                } else if (config.monster.showName) {
                    snprintf(buf, sizeof(buf), "%s%s", monster.name.c_str(), badge.c_str());
                } else {
                    snprintf(buf, sizeof(buf), "[%.0fm]", monster.distance);
                }

                DrawTextPill(drawList, ImVec2(monster.screenPosition.x, textY), enemyColor, buf, bgAlpha);
            }
        }
    } else {
        config.diagnostics.monstersFound = 0;
    }

    if (config.player.enabled) {
        auto& players = frame.players;
        config.diagnostics.playersFound = static_cast<int>(players.size());

        std::sort(players.begin(), players.end(), [](const Game::PlayerEntity& a, const Game::PlayerEntity& b) {
            return a.distance > b.distance;
        });

        for (const auto& player : players) {
            ImU32 playerColor;
            if (player.isDead) {
                playerColor = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(config.player.colorDead));
            } else {
                playerColor = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(config.player.color));
            }

            float textY = player.hasScreenBox ? player.topScreenPosition.y - 10.0f : player.screenPosition.y - 12.0f;

            if (player.hasScreenBox && (config.player.box2D || config.player.cornerBox)) {
                float h = player.screenBoxMax.y - player.screenBoxMin.y;
                float w = player.screenBoxMax.x - player.screenBoxMin.x;
                float x = player.screenBoxMin.x;
                float y = player.screenBoxMin.y;
                textY = y - 10.0f;

                if (config.player.box2D) {
                    Draw2DBox(drawList, x, y, w, h, playerColor, config.player.boxThickness);
                } else if (config.player.cornerBox) {
                    DrawCornerBox(drawList, x, y, w, h, playerColor, config.player.boxThickness);
                }

                if (config.player.snaplines) {
                    drawList->AddLine(ImVec2(screenWidth * 0.5f, screenHeight),
                                      ImVec2(player.screenPosition.x, player.bottomScreenPosition.y),
                                      playerColor, config.visual.snaplineThickness);
                }
            } else if (config.player.snaplines) {
                drawList->AddLine(ImVec2(screenWidth * 0.5f, screenHeight),
                                  ImVec2(player.screenPosition.x, player.hasScreenBox ? player.bottomScreenPosition.y : player.screenPosition.y),
                                  playerColor, config.visual.snaplineThickness);
            }

            if (config.player.showName || config.player.showDistance || config.player.showHealth) {
                char buf[128];
                std::string hpStr;
                if (player.isDead) {
                    hpStr = " [DEAD]";
                } else if (config.player.showHealth) {
                    hpStr = " [" + std::to_string(player.health) + " HP]";
                }

                if (config.player.showName && config.player.showDistance) {
                    snprintf(buf, sizeof(buf), "%s%s [%.0fm]", player.name.c_str(), hpStr.c_str(), player.distance);
                } else if (config.player.showName) {
                    snprintf(buf, sizeof(buf), "%s%s", player.name.c_str(), hpStr.c_str());
                } else {
                    snprintf(buf, sizeof(buf), "%s [%.0fm]", hpStr.c_str(), player.distance);
                }

                DrawTextPill(drawList, ImVec2(player.screenPosition.x, textY), playerColor, buf, bgAlpha);
            }
        }
    } else {
        config.diagnostics.playersFound = 0;
    }

    if (config.valuable.enabled) {
        auto& valuables = frame.valuables;
        config.diagnostics.valuablesFound = static_cast<int>(valuables.size());

        std::sort(valuables.begin(), valuables.end(), [](const Game::ValuableEntity& a, const Game::ValuableEntity& b) {
            return a.distance > b.distance;
        });

        ImU32 valColor = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(config.valuable.color));

        for (const auto& item : valuables) {
            if (item.screenPosition.x < -150.0f || item.screenPosition.x > screenWidth + 150.0f ||
                item.screenPosition.y < -50.0f || item.screenPosition.y > screenHeight + 50.0f) {
                continue;
            }
            char buf[128];
            if (config.valuable.showName && config.valuable.showValue && config.valuable.showDistance) {
                snprintf(buf, sizeof(buf), "%s $%.0f [%.0fm]", item.name.c_str(), item.dollarValue, item.distance);
            } else if (config.valuable.showName && config.valuable.showValue) {
                snprintf(buf, sizeof(buf), "%s $%.0f", item.name.c_str(), item.dollarValue);
            } else if (config.valuable.showValue && config.valuable.showDistance) {
                snprintf(buf, sizeof(buf), "$%.0f [%.0fm]", item.dollarValue, item.distance);
            } else if (config.valuable.showName && config.valuable.showDistance) {
                snprintf(buf, sizeof(buf), "%s [%.0fm]", item.name.c_str(), item.distance);
            } else if (config.valuable.showValue) {
                snprintf(buf, sizeof(buf), "$%.0f", item.dollarValue);
            } else {
                snprintf(buf, sizeof(buf), "%s", item.name.c_str());
            }

            DrawTextPill(drawList, ImVec2(item.screenPosition.x, item.screenPosition.y), valColor, buf, bgAlpha);
        }
    } else {
        config.diagnostics.valuablesFound = 0;
    }

    if (config.item.enabled) {
        auto& items = frame.items;
        config.diagnostics.itemsFound = static_cast<int>(items.size());

        std::sort(items.begin(), items.end(), [](const Game::ItemEntity& a, const Game::ItemEntity& b) {
            return a.distance > b.distance;
        });

        ImU32 itemColor = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(config.item.color));

        for (const auto& item : items) {
            if (item.screenPosition.x < -150.0f || item.screenPosition.x > screenWidth + 150.0f ||
                item.screenPosition.y < -50.0f || item.screenPosition.y > screenHeight + 50.0f) {
                continue;
            }
            char buf[128];
            if (item.value > 0 && config.item.showValue && config.item.showDistance) {
                snprintf(buf, sizeof(buf), "%s ($%d) [%.0fm]", item.name.c_str(), item.value, item.distance);
            } else if (config.item.showDistance) {
                snprintf(buf, sizeof(buf), "%s [%.0fm]", item.name.c_str(), item.distance);
            } else {
                snprintf(buf, sizeof(buf), "%s", item.name.c_str());
            }

            DrawTextPill(drawList, ImVec2(item.screenPosition.x, item.screenPosition.y), itemColor, buf, bgAlpha);
        }
    } else {
        config.diagnostics.itemsFound = 0;
    }

    if (++m_FrameCounter % 300 == 0) {
        LOG_INFO("Heartbeat | Cam: %s | Monsters: %d | Players: %d | Valuables: %d | Items: %d",
                 config.diagnostics.cameraSource,
                 config.diagnostics.monstersFound,
                 config.diagnostics.playersFound,
                 config.diagnostics.valuablesFound,
                 config.diagnostics.itemsFound);
    }
}

}
