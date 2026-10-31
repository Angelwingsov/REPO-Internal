#include "theme.h"
#include "imgui/imgui.h"

namespace UI {

void Theme::Apply() {
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImGui::StyleColorsDark(&style);
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
}

}
