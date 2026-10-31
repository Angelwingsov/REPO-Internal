#include "config.h"

namespace Core {

Config& Config::Get() {
    static Config instance;
    return instance;
}

void Config::ResetToDefaults() {
    monster = MonsterConfig();
    player = PlayerESPConfig();
    valuable = ValuableConfig();
    item = ItemConfig();
    movement = MovementConfig();
    noclip = NoClipConfig();
    godMode = GodModeConfig();
    thirdPerson = ThirdPersonConfig();
    spectatorList = SpectatorListConfig();
    troll = TrollConfig();
    visual = VisualConfig();
    menu.isVisible = true;
    menu.toggleKey = VK_DELETE;
    menu.blockInputWhenOpen = true;
    menu.requestUnload = false;
}

}
