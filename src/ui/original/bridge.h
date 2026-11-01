#pragma once
#include "framework.h"
#include "core/config.h"

class CMenu {
public:
    static CMenu* get() { static CMenu instance; return &instance; }
    ImColor GetMenuColor() const {
        const auto& color = Core::Config::Get().menu.color;
        return ImColor(color[0],color[1],color[2],std::min(255,IdaLovesMe::Globals::Gui_Ctx->MenuAlpha));
    }
};
