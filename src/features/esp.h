#pragma once
#include <windows.h>
#include "imgui/imgui.h"
#include "core/snapshot.h"
#include "game/entities.h"

namespace Features {

class ESP {
public:
    static ESP& Get();

    void Render(ImDrawList* drawList, float screenWidth, float screenHeight);
    void Update(float screenWidth, float screenHeight);

private:
    friend struct ESPRenderTestAccess;
    ESP() = default;
    struct Frame {
        std::vector<Game::MonsterEntity> monsters;
        std::vector<Game::PlayerEntity> players;
        std::vector<Game::ValuableEntity> valuables;
        std::vector<Game::ItemEntity> items;
        float width = 0, height = 0;
        ULONGLONG timestamp = 0;
        bool available = false;
    };
    Frame Collect(float width, float height);
    Core::Snapshot<Frame> m_Frame;

    void DrawCornerBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 col, float thickness);
    void Draw2DBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 col, float thickness);
    void DrawTextPill(ImDrawList* drawList, const ImVec2& centerPos, ImU32 textColor, const char* text, float bgAlpha);
    void DrawCrosshair(ImDrawList* drawList, float screenW, float screenH);

    int m_FrameCounter = 0;
};

}
