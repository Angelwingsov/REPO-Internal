#include "config.h"

namespace Core {

Config& Config::Get() {
    if (s_ThreadConfig) return *s_ThreadConfig;
    return Global();
}

Config& Config::Global() {
    static Config instance;
    return instance;
}

std::recursive_mutex& Config::Mutex() {
    static std::recursive_mutex mutex;
    return mutex;
}

Config Config::Copy() {
    std::lock_guard<std::recursive_mutex> lock(Mutex());
    return Get();
}

void Config::ResetToDefaults() {
    monster = MonsterConfig();
    player = PlayerESPConfig();
    chams = ChamsConfig();
    valuable = ValuableConfig();
    item = ItemConfig();
    movement = MovementConfig();
    noclip = NoClipConfig();
    godMode = GodModeConfig();
    thirdPerson = ThirdPersonConfig();
    spectatorList = SpectatorListConfig();
    troll = TrollConfig();
    cosmeticParser = CosmeticParserConfig();
    removals = RemovalsConfig();
    fullbright = FullbrightConfig();
    visual = VisualConfig();
    menu = MenuConfig();
}

}
