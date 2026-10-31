#pragma once
#include <windows.h>
#include "imgui/imgui.h"

namespace Features {

class ESP {
public:
    static ESP& Get();

    void Render(ImDrawList* drawList, float screenWidth, float screenHeight);

private:
    ESP() = default;

    void DrawCornerBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 col, float thickness);
    void Draw2DBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 col, float thickness);
    void DrawTextPill(ImDrawList* drawList, const ImVec2& centerPos, ImU32 textColor, const char* text, float bgAlpha);
    void DrawCrosshair(ImDrawList* drawList, float screenW, float screenH);

    int m_FrameCounter = 0;
};

}
