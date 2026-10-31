#pragma once
#include <windows.h>
#include <cstdint>

namespace Core {

struct MonsterConfig {
    bool enabled = true;
    bool box2D = true;
    bool cornerBox = false;
    bool snaplines = false;
    bool showName = true;
    bool showDistance = true;
    bool highlightChasing = true;
    bool showStunned = true;
    float maxDistance = 300.0f;
    float boxThickness = 1.5f;

    float colorDefault[4] = { 1.0f, 0.25f, 0.25f, 1.0f };
    float colorChasing[4] = { 1.0f, 0.08f, 0.08f, 1.0f };
    float colorStunned[4] = { 1.0f, 0.85f, 0.20f, 1.0f };
};

struct PlayerESPConfig {
    bool enabled = true;
    bool box2D = true;
    bool cornerBox = false;
    bool snaplines = false;
    bool showName = true;
    bool showDistance = true;
    bool showHealth = true;
    bool hideLocalPlayer = true;
    float maxDistance = 300.0f;
    float boxThickness = 1.5f;

    float color[4] = { 0.20f, 0.75f, 1.0f, 1.0f };
    float colorDead[4] = { 0.65f, 0.65f, 0.65f, 0.8f };
};

struct ValuableConfig {
    bool enabled = true;
    bool showName = true;
    bool showValue = true;
    bool showDistance = true;
    bool hideExtracted = true;
    float minDollarValue = 0.0f;
    float maxDistance = 250.0f;

    float color[4] = { 1.0f, 0.85f, 0.15f, 1.0f };
};

struct ItemConfig {
    bool enabled = true;
    bool showName = true;
    bool showValue = true;
    bool showDistance = true;
    float maxDistance = 200.0f;

    float color[4] = { 0.25f, 0.95f, 0.35f, 1.0f };
};

struct MovementConfig {
    bool speedEnabled = false;
    bool speedAlwaysOn = false;
    float speedMultiplier = 2.5f;
    int speedKey = 'V';
    int speedKeyMode = 0;
    bool speedActive = false;

    bool infiniteStamina = true;

    bool infiniteStrength = true;
    float strengthMultiplier = 50.0f;
    bool extendGrabRange = false;
    float grabRange = 15.0f;

    bool superJump = false;
    float jumpMultiplier = 1.8f;
    bool noTumble = true;
};

struct ThirdPersonConfig {
    bool enabled = true;
    int toggleKey = 'F';
    int keyMode = 1;
    bool active = false;

    float distance = 2.5f;
    float shoulderOffset = 0.45f;
    float heightOffset = 0.20f;
    bool collision = true;
};

struct SpectatorListConfig {
    bool enabled = true;
    bool showTarget = true;
    bool showMode = true;
    bool hideWhenEmpty = false;
    bool onlySpectatingMe = true;
    bool showSpectatorCount = true;

    float colorHeader[4] = { 0.25f, 0.65f, 1.0f, 1.0f };
    float colorSpectator[4] = { 0.95f, 0.95f, 0.95f, 1.0f };
    float colorTargetMe[4] = { 0.20f, 1.0f, 0.35f, 1.0f };
    float colorTargetOther[4] = { 1.0f, 0.85f, 0.25f, 1.0f };
};

struct TrollConfig {
    bool globalVoice = false;

    bool zeroGravity = false;
    bool featherFall = false;

    bool overrideAnimSpeed = false;
    float animSpeed = 3.0f;
};

struct VisualConfig {
    bool textBackground = true;
    float textBgAlpha = 0.65f;
    bool drawCrosshair = false;
    float crosshairSize = 5.0f;
    float crosshairColor[4] = { 1.0f, 1.0f, 1.0f, 0.8f };
    float snaplineThickness = 1.2f;
};

struct MenuConfig {
    bool isVisible = true;
    DWORD toggleKey = VK_DELETE;
    bool blockInputWhenOpen = true;
    bool requestUnload = false;
};

struct Diagnostics {
    int monstersFound = 0;
    int playersFound = 0;
    int valuablesFound = 0;
    int itemsFound = 0;
    bool cameraValid = false;
    bool mapLoading = false;
    const char* cameraSource = "None";
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
};

class Config {
public:
    static Config& Get();

    MonsterConfig monster;
    PlayerESPConfig player;
    ValuableConfig valuable;
    ItemConfig item;
    MovementConfig movement;
    ThirdPersonConfig thirdPerson;
    SpectatorListConfig spectatorList;
    VisualConfig visual;
    MenuConfig menu;
    Diagnostics diagnostics;
    TrollConfig troll;

    void ResetToDefaults();

private:
    Config() = default;
};

}
