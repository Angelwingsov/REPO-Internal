#pragma once
#include <windows.h>
#include <cstdint>
#include <mutex>

#ifndef VK_MWHEELUP
#define VK_MWHEELUP   0x0E
#endif
#ifndef VK_MWHEELDOWN
#define VK_MWHEELDOWN 0x0F
#endif

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

struct ChamsConfig {
    bool enabled = false;
    float colorVisible[4] = { 0.20f, 0.95f, 0.30f, 1.0f };
    float colorInvisible[4] = { 0.95f, 0.20f, 0.20f, 1.0f };
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

struct GodModeConfig {
    bool enabled = false;
    bool healToMax = true;
    bool preventDeath = true;
    bool noFallDamage = true;
    bool autoRevive = false;
    int toggleKey = 'G';
    int keyMode = 1;
    bool active = false;
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
    bool grabThroughWalls = false;

    bool superJump = false;
    float jumpMultiplier = 1.8f;
    bool noTumble = true;
};

struct NoClipConfig {
    bool enabled = false;
    int toggleKey = 'N';
    int keyMode = 1;
    bool active = false;

    float speed = 15.0f;
    float sprintMultiplier = 2.5f;
    bool freezeWhenIdle = true;
    bool disableColliders = true;
    bool verticalFollowCam = true;
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

struct AntiAimConfig {
    bool enabled = false;
    int toggleKey = 0;
    int keyMode = 1;
    bool active = false;

    int yawMode = 1;
    float spinSpeed = 360.0f;
    float jitterRange = 90.0f;

    int pitchMode = 1;

    int rollMode = 0;
    float leanAngle = 35.0f;
};

struct AnimBreakerConfig {
    bool enabled = false;

    bool slideWalk = false;
    bool staticLegs = false;
    bool forceCrawl = false;

    bool poseSpammer = false;
    float poseSpamSpeed = 10.0f;

    bool headBreaker = false;
    float headSpinSpeed = 360.0f;

    bool spineBreaker = false;
    float spineTwitchAmount = 60.0f;
};

struct RemovalsConfig {
    bool enabled = false;
    bool noFog = true;
    bool noGrain = true;
    bool noVignette = true;
    bool noBloom = false;
    bool noMotionBlur = true;
    bool noLensDistortion = true;
    bool noChromaticAberration = true;
    bool noCameraShake = true;
    bool noCameraNoise = true;
    bool noCameraGlitch = true;
    bool noHurtOverlay = true;
    bool noPostProcessing = false;
};

struct FullbrightConfig {
    bool enabled = false;
    float brightness = 1.0f;
};

struct CosmeticParserConfig {
    int selectedPlayerIdx = 0;
    uintptr_t selectedPlayerAddress = 0;
    char statusMessage[128] = "";
    bool actionSuccess = false;
    bool hasStatus = false;
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
    int color[4] = {163, 212, 31, 255};
};

struct SpawnerConfig {
    int selectedCategory = 0;
    int selectedItemIndex = 0;
    int spawnAmount = 1;
    float spawnDistance = 2.0f;
    bool spawnAtCrosshair = false;
    int spawnMode = 0;
    int hotkey = 0;
    int hotkeyMode = 1;
    char searchFilter[64] = {};
    bool hasStatus = false;
    char statusMessage[128] = "Ready";
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
    static Config& Global();
    static std::recursive_mutex& Mutex();
    static Config Copy();
    class Scope;

    MonsterConfig monster;
    PlayerESPConfig player;
    ChamsConfig chams;
    ValuableConfig valuable;
    ItemConfig item;
    MovementConfig movement;
    NoClipConfig noclip;
    GodModeConfig godMode;
    ThirdPersonConfig thirdPerson;
    SpectatorListConfig spectatorList;
    VisualConfig visual;
    MenuConfig menu;
    Diagnostics diagnostics;
    TrollConfig troll;
    CosmeticParserConfig cosmeticParser;
    RemovalsConfig removals;
    FullbrightConfig fullbright;
    SpawnerConfig spawner;
    AntiAimConfig antiAim;
    AnimBreakerConfig animBreaker;

    void ResetToDefaults();

private:
    static inline thread_local Config* s_ThreadConfig = nullptr;
    Config() = default;
};

class Config::Scope {
public:
    explicit Scope(Config& config) : m_Previous(s_ThreadConfig) { s_ThreadConfig = &config; }
    ~Scope() { s_ThreadConfig = m_Previous; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
private:
    Config* m_Previous;
};

}
