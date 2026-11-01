#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <atomic>
#include "mono/mono_bridge.h"

namespace Game {

struct Offsets {
    int PlayerAvatarVisuals_meshParent = -1;
    int PlayerAvatarVisuals_headTop = -1;
    int EnemyDirector_enemiesSpawned = 0x0108;
    int EnemyParent_enemyName = 0x0010;
    int EnemyParent_Spawned = 0x0021;
    int EnemyParent_Enemy = 0x0028;
    int Enemy_CenterTransform = 0x0038;
    int Enemy_CurrentState = 0x0028;
    int Enemy_Stunned = 0x0139;

    int ValuableDirector_valuableList = 0x01D8;
    int ValuableObject_dollarValueCurrent = 0x005C;
    int ValuableObject_dollarValueOriginal = 0x0058;
    int ValuableObject_committedToExtraction = 0x0090;

    int ItemManager_spawnedItems = 0x0038;
    int ItemAttributes_itemName = 0x00A8;
    int ItemAttributes_instanceName = 0x00B0;
    int ItemAttributes_value = 0x0068;

    int CameraUtils_MainCamera = 0x0020;
    int GameDirector_MainCamera = 0x0080;
    int GameDirector_currentState = 0x0020;
    int GameDirector_PlayerList = 0x00C8;
    int RunManager_restarting = 0x0070;
    int RunManager_sceneHandoffNativePending = 0x0144;

    int PlayerAvatar_playerName = 0x00B0;
    int PlayerAvatar_playerCosmetics = 0x00A8;
    int PlayerAvatar_playerTransform = 0x0018;
    int PlayerAvatar_isLocal = 0x00C0;
    int PlayerAvatar_deadSet = 0x0188;
    int PlayerAvatar_playerHealth = 0x0058;
    int PlayerAvatar_spectating = 0x0180;
    int PlayerAvatar_isDisabled = 0x00C1;
    int PlayerAvatar_clientCameraPosition = 0x0130;
    int PlayerAvatar_clientCameraStateValid = 0x0158;
    int PlayerAvatar_playerCollider = 0x00C0;
    int PlayerAvatar_rb = 0x00A0;
    int PlayerHealth_health = 0x0028;
    int PlayerHealth_maxHealth = 0x0030;
    int PlayerHealth_godMode = 0x00C0;
    int PlayerHealth_invincibleTimer = 0x0188;
    int PlayerHealth_hurtFreeze = 0x00A0;
    int PlayerHealth_hurtFreezeTimer = 0x00A4;
    int PlayerAvatar_fallDamageResetState = 0x0280;
    int PlayerAvatar_fallDamageResetTimer = 0x0288;
    int PlayerAvatar_isTumbling = 0x00C2;
    int PlayerTumble_isTumbling = 0x00A0;
    int PlayerTumble_tumbleSetTimer = 0x00A8;
    int PlayerTumble_impactHurtTimer = 0x0098;
    int PlayerTumble_impactHurtDamage = 0x009C;
    int DebugCommandHandler_godMode = 0x0090;

    int SpectateCamera_player = 0x0028;
    int SpectateCamera_currentState = 0x0018;

    int PlayerController_MoveSpeed = 0x0058;
    int PlayerController_SprintSpeed = 0x0078;
    int PlayerController_CrouchSpeed = 0x0080;
    int PlayerController_EnergyCurrent = 0x00E8;
    int PlayerController_EnergyStart = 0x00E4;
    int PlayerController_DebugEnergy = 0x00E0;
    int PlayerController_overrideSpeedMultiplier = 0x0154;
    int PlayerController_overrideSpeedTimer = 0x0150;
    int PlayerController_overrideSpeedSprintCap = 0x0158;
    int PlayerController_playerOriginalMoveSpeed = 0x0188;
    int PlayerController_playerOriginalSprintSpeed = 0x0190;
    int PlayerController_playerOriginalCrouchSpeed = 0x0194;
    int PlayerController_JumpForce = 0x00A8;
    int PlayerController_DebugNoTumble = 0x00DE;
    int PlayerController_overrideVoicePitchMultiplier = 0x0164;
    int PlayerController_overrideVoicePitchTimer = 0x0160;
    int PlayerController_antiGravityTimer = 0x0180;
    int PlayerController_featherTimer = 0x0184;
    int PlayerController_playerAvatarScript = 0x0178;
    int PlayerController_rb = 0x00C8;
    int PlayerController_col = 0x02A8;
    int PlayerController_kinematicTimer = 0x02C0;
    int PlayerController_Velocity = 0x00DC;
    int PlayerController_VelocityImpulse = 0x00EC;
    int PlayerController_positionPrevious = 0x01F8;

    int PlayerAvatar_voiceChat = 0x0060;
    int PlayerAvatar_tumble = 0x0068;
    int PlayerVoiceChat_SpatialDisableTimer = 0x0090;

    int PhysGrabber_grabStrength = 0x01C8;
    int PhysGrabber_throwStrength = 0x01CC;
    int PhysGrabber_forceMax = 0x00BC;
    int PhysGrabber_grabRange = 0x005C;
    int PhysGrabber_grabReleaseDistance = 0x0060;
    int PhysGrabber_maxDistanceFromPlayer = 0x0068;
    int PhysGrabber_overrideGrabStrength = 0x0218;
    int PhysGrabber_overrideGrabStrengthTimer = 0x021C;
    int PhysGrabber_climbForceMultiplier = 0x0018;
    int PhysGrabber_grabbedPhysGrabObject = 0x00A0;

    int PhysGrabObject_overrideGrabStrength = 0x0218;
    int PhysGrabObject_overrideGrabStrengthTimer = 0x021C;
    int PhysGrabObject_timerAlterMass = 0x01B0;
    int PhysGrabObject_alterMassValue = 0x01B4;

    int PlayerCosmetics_cosmeticEquippedRaw = 0x0048;
    int PlayerCosmetics_colorsEquipped = 0x0050;
    int PlayerCosmetics_photonView = 0x0018;

    int MetaManager_cosmeticUnlocks = 0x0030;
    int MetaManager_cosmeticAssets = 0x0070;

    int EnvironmentDirector_ambientColorOverride = 0x0110;
    int EnvironmentDirector_ambientColorOverrideTimer = 0x0120;
    int EnvironmentDirector_fogColorOverride = 0x0124;
    int EnvironmentDirector_fogColorOverrideTimer = 0x0134;
    int EnvironmentDirector_fogDistanceOverride = 0x0138;
    int EnvironmentDirector_fogDistanceOverrideTimer = 0x0140;

    int FlashlightController_spotlight = 0x0048;
    int FlashlightController_baseIntensity = 0x0108;
    int FlashlightController_flickerMultiplier = 0x0164;
    int FlashlightController_flickerTimer = 0x0170;
    int FlashlightController_active = 0x0070;
    int FlashlightController_LightActive = 0x007D;

    int LightManager_turnOffLights = 0x0080;
    int LightManager_turningOffLights = 0x0081;
    int LightManager_turningOffEmissions = 0x0082;

    int PlayerAvatar_playerAvatarVisuals = 0x0040;
    int PlayerAvatar_PlayerVisionTarget = 0x0068;
    int PlayerVisionTarget_VisionTransform = 0x0020;
    int CameraPosition_playerTransform = 0x0018;
    int CameraPosition_playerOffset = 0x0020;
    int CameraPosition_overridePositionActive = 0x0034;
    int CameraPosition_overridePositionTimer = 0x0038;
    int CameraPosition_overridePositionTarget = 0x003C;
};

struct ServerPlayerInfo {
    uintptr_t address = 0;
    std::string name;
    bool isLocal = false;
    bool isDead = false;
    int health = 100;
};

class GameState {
public:
    static GameState& Get();

    bool Initialize();
    bool IsReady() const { return m_Initialized; }

    bool IsMapLoadingOrRestarting();

    Offsets offsets;

    MonoClass* klassEnemyDirector = nullptr;
    MonoClass* klassEnemyParent = nullptr;
    MonoClass* klassEnemy = nullptr;
    MonoClass* klassValuableDirector = nullptr;
    MonoClass* klassValuableObject = nullptr;
    MonoClass* klassItemManager = nullptr;
    MonoClass* klassItemAttributes = nullptr;
    MonoClass* klassSemiFunc = nullptr;
    MonoClass* klassCameraUtils = nullptr;
    MonoClass* klassGameDirector = nullptr;
    MonoClass* klassRunManager = nullptr;
    MonoClass* klassPlayerAvatar = nullptr;
    MonoClass* klassPlayerController = nullptr;
    MonoClass* klassPhysGrabber = nullptr;
    MonoClass* klassPhysGrabObject = nullptr;
    MonoClass* klassPlayerHealth = nullptr;
    MonoClass* klassPlayerVoiceChat = nullptr;
    MonoClass* klassMetaManager = nullptr;
    MonoClass* klassEnvironmentDirector = nullptr;
    MonoClass* klassFlashlightController = nullptr;
    MonoClass* klassLightManager = nullptr;
    MonoClass* klassPlayerTumble = nullptr;
    MonoClass* klassCameraPosition = nullptr;
    MonoClass* klassSpectateCamera = nullptr;
    MonoClass* klassPlayerAvatarVisuals = nullptr;
    MonoClass* klassPlayerVisionTarget = nullptr;
    MonoClass* klassDebugCommandHandler = nullptr;
    MonoClass* klassPlayerCosmetics = nullptr;

    MonoMethod* methodCameraPositionOverride = nullptr;
    MonoMethod* methodShowSelfOverride = nullptr;
    MonoMethod* methodCosmeticUnlockAll = nullptr;
    MonoMethod* methodOverrideAnimationSpeed = nullptr;
    MonoMethod* methodPlayerHealthHeal = nullptr;
    MonoMethod* methodPlayerHealthInvincibleSet = nullptr;
    MonoMethod* methodPlayerAvatarRevive = nullptr;
    MonoMethod* methodPlayerCosmeticsSetupCosmetics = nullptr;
    MonoMethod* methodPlayerCosmeticsSetupColors = nullptr;

    bool UnlockAllCosmetics(int* outUnlockedCount = nullptr, int* outTotalCount = nullptr);
    bool GetCosmeticStats(int& unlockedCount, int& totalCount);
    void SetAnimationSpeed(float targetSpeed);
    bool ReviveLocalPlayer();
    bool HealLocalPlayer(int healAmount = 0);

    uintptr_t GetLocalPlayerAvatar();
    bool CopyPlayerCosmetics(uintptr_t targetAvatarAddress);
    bool CopyPlayerColors(uintptr_t targetAvatarAddress);

    bool QueueCosmeticChange(uintptr_t target, bool cosmetics, bool colors);
    bool TakeCosmeticResult(bool& success, std::string& message);
    std::vector<ServerPlayerInfo> GetServerPlayers();

private:
    GameState() = default;
    bool ResolveOffsets();

    std::atomic<bool> m_Initialized{false};
};

}
