#include "state.h"
#include "core/logger.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "minhook/include/MinHook.h"
#include "features/removals.h"
#include "features/player.h"
#include "features/antiaim.h"

#include "runtime.h"
#include "core/hook_activity.h"
#include <mutex>
#include <utility>

namespace Game {

namespace {
    struct AppearanceChange {
        uintptr_t target = 0;
        bool cosmetics = false, colors = false;
    };
    std::mutex appearanceMutex;
    AppearanceChange pendingAppearance;
    bool appearancePending = false, appearanceReady = false, appearanceSuccess = false;
    std::string appearanceMessage;
    using PlayerUpdate = void (*)(void*);
    PlayerUpdate originalPlayerUpdate = nullptr;
    bool appearanceHookReady = false;

    void PlayerUpdateHook(void* controller) {
        Core::HookActivity activity;
        originalPlayerUpdate(controller);
        Runtime::Get().Tick();
        if (!GameState::Get().IsReady() || Runtime::Get().IsStopping()) return;
        if (GameState::Get().IsMapLoadingOrRestarting()) {
            std::lock_guard<std::mutex> lock(appearanceMutex);
            if (appearancePending) {
                appearancePending = false;
                appearanceReady = true;
                appearanceSuccess = false;
                appearanceMessage = "[-] Scene is changing. Select a player after loading.";
            }
            return;
        }
        AppearanceChange request;
        {
            std::lock_guard<std::mutex> lock(appearanceMutex);
            if (!appearancePending) return;
            request = pendingAppearance;
            appearancePending = false;
        }
        auto& gs = GameState::Get();
        bool targetPresent = false;
        for (const auto& player : gs.GetServerPlayers()) {
            if (player.address == request.target && !player.isLocal) targetPresent = true;
        }
        bool cosmetics = !request.cosmetics, colors = !request.colors;
        if (targetPresent) {
            if (request.cosmetics) cosmetics = gs.CopyPlayerCosmetics(request.target);
            if (request.colors) colors = gs.CopyPlayerColors(request.target);
        }
        std::string message;
        if (!targetPresent) message = "[-] Target left or is unavailable. Select another player.";
        else {
            if (request.cosmetics) message += cosmetics ? "Cosmetics: OK. " : "Cosmetics: FAILED. ";
            if (request.colors) message += colors ? "Colors: OK." : "Colors: FAILED.";
        }
        std::lock_guard<std::mutex> lock(appearanceMutex);
        appearanceSuccess = targetPresent && cosmetics && colors;
        appearanceMessage = std::move(message);
        appearanceReady = true;
    }
}

bool GameState::QueueCosmeticChange(uintptr_t target, bool cosmetics, bool colors) {
    if (!appearanceHookReady || Runtime::Get().IsStopping() || !target || (!cosmetics && !colors)) return false;
    std::lock_guard<std::mutex> lock(appearanceMutex);
    pendingAppearance = { target, cosmetics, colors };
    appearancePending = true;
    appearanceReady = false;
    return true;
}

bool GameState::TakeCosmeticResult(bool& success, std::string& message) {
    std::lock_guard<std::mutex> lock(appearanceMutex);
    if (!appearanceReady) return false;
    success = appearanceSuccess;
    message = appearanceMessage;
    appearanceReady = false;
    return true;
}

GameState& GameState::Get() {
    static GameState instance;
    return instance;
}

bool GameState::ResolveOffsets() {
    auto& bridge = Mono::Bridge::Get();
    bool valid = true;
    LOG_INFO("Dynamically resolving runtime field offsets via Mono metadata...");

    #define RESOLVE_FIELD(klass, fieldName, targetVar, fallback) \
        targetVar = bridge.GetFieldOffset(klass, fieldName, -1); \
        if (targetVar <= 0) { valid = false; LOG_ERR("Required field missing: %s::%s; gameplay features disabled", #klass, fieldName); } \
        else LOG_OK("Field [%s::%s] -> 0x%X (+%d)", #klass, fieldName, targetVar, targetVar);

    RESOLVE_FIELD(klassEnemyDirector, "enemiesSpawned", offsets.EnemyDirector_enemiesSpawned, 0x0108);
    RESOLVE_FIELD(klassEnemyParent, "enemyName", offsets.EnemyParent_enemyName, 0x0010);
    RESOLVE_FIELD(klassEnemyParent, "Spawned", offsets.EnemyParent_Spawned, 0x0021);
    RESOLVE_FIELD(klassEnemyParent, "Enemy", offsets.EnemyParent_Enemy, 0x0028);
    RESOLVE_FIELD(klassEnemy, "CenterTransform", offsets.Enemy_CenterTransform, 0x0038);
    RESOLVE_FIELD(klassEnemy, "CurrentState", offsets.Enemy_CurrentState, 0x0028);
    RESOLVE_FIELD(klassEnemy, "Stunned", offsets.Enemy_Stunned, 0x0139);

    RESOLVE_FIELD(klassValuableDirector, "valuableList", offsets.ValuableDirector_valuableList, 0x01D8);
    RESOLVE_FIELD(klassValuableObject, "dollarValueCurrent", offsets.ValuableObject_dollarValueCurrent, 0x005C);
    RESOLVE_FIELD(klassValuableObject, "dollarValueOriginal", offsets.ValuableObject_dollarValueOriginal, 0x0058);
    RESOLVE_FIELD(klassValuableObject, "committedToExtraction", offsets.ValuableObject_committedToExtraction, 0x0090);

    RESOLVE_FIELD(klassItemManager, "spawnedItems", offsets.ItemManager_spawnedItems, 0x0038);
    RESOLVE_FIELD(klassItemAttributes, "itemName", offsets.ItemAttributes_itemName, 0x00A8);
    RESOLVE_FIELD(klassItemAttributes, "instanceName", offsets.ItemAttributes_instanceName, 0x00B0);
    RESOLVE_FIELD(klassItemAttributes, "value", offsets.ItemAttributes_value, 0x0068);

    RESOLVE_FIELD(klassCameraUtils, "MainCamera", offsets.CameraUtils_MainCamera, 0x0020);
    RESOLVE_FIELD(klassGameDirector, "MainCamera", offsets.GameDirector_MainCamera, 0x0080);
    RESOLVE_FIELD(klassGameDirector, "currentState", offsets.GameDirector_currentState, 0x0020);
    RESOLVE_FIELD(klassGameDirector, "PlayerList", offsets.GameDirector_PlayerList, 0x00C8);
    RESOLVE_FIELD(klassRunManager, "restarting", offsets.RunManager_restarting, 0x0070);
    RESOLVE_FIELD(klassRunManager, "sceneHandoffNativePending", offsets.RunManager_sceneHandoffNativePending, 0x0144);

    RESOLVE_FIELD(klassPlayerAvatar, "playerName", offsets.PlayerAvatar_playerName, 0x00B0);
    RESOLVE_FIELD(klassPlayerAvatar, "playerCosmetics", offsets.PlayerAvatar_playerCosmetics, 0x00A8);
    RESOLVE_FIELD(klassPlayerAvatar, "playerTransform", offsets.PlayerAvatar_playerTransform, 0x0018);
    RESOLVE_FIELD(klassPlayerAvatar, "isLocal", offsets.PlayerAvatar_isLocal, 0x00C0);
    RESOLVE_FIELD(klassPlayerAvatar, "deadSet", offsets.PlayerAvatar_deadSet, 0x0188);
    RESOLVE_FIELD(klassPlayerAvatar, "playerHealth", offsets.PlayerAvatar_playerHealth, 0x0058);
    RESOLVE_FIELD(klassPlayerAvatar, "spectating", offsets.PlayerAvatar_spectating, 0x0180);
    RESOLVE_FIELD(klassPlayerAvatar, "isDisabled", offsets.PlayerAvatar_isDisabled, 0x00C1);
    RESOLVE_FIELD(klassPlayerAvatar, "clientCameraPosition", offsets.PlayerAvatar_clientCameraPosition, 0x0130);
    RESOLVE_FIELD(klassPlayerAvatar, "clientCameraStateValid", offsets.PlayerAvatar_clientCameraStateValid, 0x0158);
    RESOLVE_FIELD(klassPlayerAvatar, "playerCollider", offsets.PlayerAvatar_playerCollider, 0x00C0);
    RESOLVE_FIELD(klassPlayerAvatar, "rb", offsets.PlayerAvatar_rb, 0x00A0);
    RESOLVE_FIELD(klassPlayerHealth, "health", offsets.PlayerHealth_health, 0x0028);
    RESOLVE_FIELD(klassPlayerHealth, "maxHealth", offsets.PlayerHealth_maxHealth, 0x0030);
    RESOLVE_FIELD(klassPlayerHealth, "godMode", offsets.PlayerHealth_godMode, 0x00C0);
    RESOLVE_FIELD(klassPlayerHealth, "invincibleTimer", offsets.PlayerHealth_invincibleTimer, 0x0188);
    RESOLVE_FIELD(klassPlayerHealth, "hurtFreeze", offsets.PlayerHealth_hurtFreeze, 0x00A0);
    RESOLVE_FIELD(klassPlayerHealth, "hurtFreezeTimer", offsets.PlayerHealth_hurtFreezeTimer, 0x00A4);
    RESOLVE_FIELD(klassPlayerAvatar, "fallDamageResetState", offsets.PlayerAvatar_fallDamageResetState, 0x0280);
    RESOLVE_FIELD(klassPlayerAvatar, "fallDamageResetTimer", offsets.PlayerAvatar_fallDamageResetTimer, 0x0288);
    RESOLVE_FIELD(klassPlayerAvatar, "isTumbling", offsets.PlayerAvatar_isTumbling, 0x00C2);
    RESOLVE_FIELD(klassPlayerTumble, "isTumbling", offsets.PlayerTumble_isTumbling, 0x00A0);
    RESOLVE_FIELD(klassPlayerTumble, "tumbleSetTimer", offsets.PlayerTumble_tumbleSetTimer, 0x00A8);
    RESOLVE_FIELD(klassPlayerTumble, "impactHurtTimer", offsets.PlayerTumble_impactHurtTimer, 0x0098);
    RESOLVE_FIELD(klassPlayerTumble, "impactHurtDamage", offsets.PlayerTumble_impactHurtDamage, 0x009C);
    RESOLVE_FIELD(klassDebugCommandHandler, "godMode", offsets.DebugCommandHandler_godMode, 0x0090);

    RESOLVE_FIELD(klassPlayerCosmetics, "cosmeticEquippedRaw", offsets.PlayerCosmetics_cosmeticEquippedRaw, 0x0048);
    RESOLVE_FIELD(klassPlayerCosmetics, "colorsEquipped", offsets.PlayerCosmetics_colorsEquipped, 0x0050);
    RESOLVE_FIELD(klassPlayerCosmetics, "photonView", offsets.PlayerCosmetics_photonView, 0x0018);

    RESOLVE_FIELD(klassSpectateCamera, "player", offsets.SpectateCamera_player, 0x0028);
    RESOLVE_FIELD(klassSpectateCamera, "currentState", offsets.SpectateCamera_currentState, 0x0018);

    RESOLVE_FIELD(klassPlayerAvatar, "voiceChat", offsets.PlayerAvatar_voiceChat, 0x0060);
    RESOLVE_FIELD(klassPlayerAvatar, "tumble", offsets.PlayerAvatar_tumble, 0x0068);
    RESOLVE_FIELD(klassPlayerVoiceChat, "SpatialDisableTimer", offsets.PlayerVoiceChat_SpatialDisableTimer, 0x0090);

    RESOLVE_FIELD(klassPlayerController, "MoveSpeed", offsets.PlayerController_MoveSpeed, 0x0058);
    RESOLVE_FIELD(klassPlayerController, "SprintSpeed", offsets.PlayerController_SprintSpeed, 0x0078);
    RESOLVE_FIELD(klassPlayerController, "CrouchSpeed", offsets.PlayerController_CrouchSpeed, 0x0080);
    RESOLVE_FIELD(klassPlayerController, "EnergyCurrent", offsets.PlayerController_EnergyCurrent, 0x00E8);
    RESOLVE_FIELD(klassPlayerController, "EnergyStart", offsets.PlayerController_EnergyStart, 0x00E4);
    RESOLVE_FIELD(klassPlayerController, "DebugEnergy", offsets.PlayerController_DebugEnergy, 0x00E0);
    RESOLVE_FIELD(klassPlayerController, "overrideSpeedMultiplier", offsets.PlayerController_overrideSpeedMultiplier, 0x0154);
    RESOLVE_FIELD(klassPlayerController, "overrideSpeedTimer", offsets.PlayerController_overrideSpeedTimer, 0x0150);
    RESOLVE_FIELD(klassPlayerController, "overrideSpeedSprintCap", offsets.PlayerController_overrideSpeedSprintCap, 0x0158);
    RESOLVE_FIELD(klassPlayerController, "playerOriginalMoveSpeed", offsets.PlayerController_playerOriginalMoveSpeed, 0x0188);
    RESOLVE_FIELD(klassPlayerController, "playerOriginalSprintSpeed", offsets.PlayerController_playerOriginalSprintSpeed, 0x0190);
    RESOLVE_FIELD(klassPlayerController, "playerOriginalCrouchSpeed", offsets.PlayerController_playerOriginalCrouchSpeed, 0x0194);
    RESOLVE_FIELD(klassPlayerController, "JumpForce", offsets.PlayerController_JumpForce, 0x00A8);
    RESOLVE_FIELD(klassPlayerController, "DebugNoTumble", offsets.PlayerController_DebugNoTumble, 0x00DE);
    RESOLVE_FIELD(klassPlayerController, "overrideVoicePitchMultiplier", offsets.PlayerController_overrideVoicePitchMultiplier, 0x0164);
    RESOLVE_FIELD(klassPlayerController, "overrideVoicePitchTimer", offsets.PlayerController_overrideVoicePitchTimer, 0x0160);
    RESOLVE_FIELD(klassPlayerController, "antiGravityTimer", offsets.PlayerController_antiGravityTimer, 0x0180);
    RESOLVE_FIELD(klassPlayerController, "featherTimer", offsets.PlayerController_featherTimer, 0x0184);
    RESOLVE_FIELD(klassPlayerController, "playerAvatarScript", offsets.PlayerController_playerAvatarScript, 0x0178);
    RESOLVE_FIELD(klassPlayerController, "rb", offsets.PlayerController_rb, 0x00C8);
    RESOLVE_FIELD(klassPlayerController, "col", offsets.PlayerController_col, 0x02A8);
    RESOLVE_FIELD(klassPlayerController, "kinematicTimer", offsets.PlayerController_kinematicTimer, 0x02C0);
    RESOLVE_FIELD(klassPlayerController, "Velocity", offsets.PlayerController_Velocity, 0x00DC);
    RESOLVE_FIELD(klassPlayerController, "VelocityImpulse", offsets.PlayerController_VelocityImpulse, 0x00EC);
    RESOLVE_FIELD(klassPlayerController, "positionPrevious", offsets.PlayerController_positionPrevious, 0x01F8);

    RESOLVE_FIELD(klassPhysGrabber, "grabStrength", offsets.PhysGrabber_grabStrength, 0x01C8);
    RESOLVE_FIELD(klassPhysGrabber, "throwStrength", offsets.PhysGrabber_throwStrength, 0x01CC);
    RESOLVE_FIELD(klassPhysGrabber, "forceMax", offsets.PhysGrabber_forceMax, 0x00BC);
    RESOLVE_FIELD(klassPhysGrabber, "grabRange", offsets.PhysGrabber_grabRange, 0x005C);
    RESOLVE_FIELD(klassPhysGrabber, "grabReleaseDistance", offsets.PhysGrabber_grabReleaseDistance, 0x0060);
    RESOLVE_FIELD(klassPhysGrabber, "maxDistanceFromPlayer", offsets.PhysGrabber_maxDistanceFromPlayer, 0x0068);
    RESOLVE_FIELD(klassPhysGrabber, "overrideAlwaysGrabbableTimer", offsets.PhysGrabber_overrideAlwaysGrabbableTimer, 0x0144);
    RESOLVE_FIELD(klassPhysGrabber, "mask", offsets.PhysGrabber_mask, 0x0158);
    RESOLVE_FIELD(klassPhysGrabber, "overrideGrabStrength", offsets.PhysGrabber_overrideGrabStrength, 0x0218);
    RESOLVE_FIELD(klassPhysGrabber, "overrideGrabStrengthTimer", offsets.PhysGrabber_overrideGrabStrengthTimer, 0x021C);
    RESOLVE_FIELD(klassPhysGrabber, "climbForceMultiplier", offsets.PhysGrabber_climbForceMultiplier, 0x0018);
    RESOLVE_FIELD(klassPhysGrabber, "grabbedPhysGrabObject", offsets.PhysGrabber_grabbedPhysGrabObject, 0x00A0);

    RESOLVE_FIELD(klassPhysGrabObject, "overrideGrabStrength", offsets.PhysGrabObject_overrideGrabStrength, 0x0218);
    RESOLVE_FIELD(klassPhysGrabObject, "overrideGrabStrengthTimer", offsets.PhysGrabObject_overrideGrabStrengthTimer, 0x021C);
    RESOLVE_FIELD(klassPhysGrabObject, "timerAlterMass", offsets.PhysGrabObject_timerAlterMass, 0x01B0);
    RESOLVE_FIELD(klassPhysGrabObject, "alterMassValue", offsets.PhysGrabObject_alterMassValue, 0x01B4);

    RESOLVE_FIELD(klassMetaManager, "cosmeticUnlocks", offsets.MetaManager_cosmeticUnlocks, 0x0030);
    RESOLVE_FIELD(klassMetaManager, "cosmeticAssets", offsets.MetaManager_cosmeticAssets, 0x0070);

    RESOLVE_FIELD(klassEnvironmentDirector, "ambientColorOverride", offsets.EnvironmentDirector_ambientColorOverride, 0x0110);
    RESOLVE_FIELD(klassEnvironmentDirector, "ambientColorOverrideTimer", offsets.EnvironmentDirector_ambientColorOverrideTimer, 0x0120);
    RESOLVE_FIELD(klassEnvironmentDirector, "fogColorOverride", offsets.EnvironmentDirector_fogColorOverride, 0x0124);
    RESOLVE_FIELD(klassEnvironmentDirector, "fogColorOverrideTimer", offsets.EnvironmentDirector_fogColorOverrideTimer, 0x0134);
    RESOLVE_FIELD(klassEnvironmentDirector, "fogDistanceOverride", offsets.EnvironmentDirector_fogDistanceOverride, 0x0138);
    RESOLVE_FIELD(klassEnvironmentDirector, "fogDistanceOverrideTimer", offsets.EnvironmentDirector_fogDistanceOverrideTimer, 0x0140);

    RESOLVE_FIELD(klassFlashlightController, "spotlight", offsets.FlashlightController_spotlight, 0x0048);
    RESOLVE_FIELD(klassFlashlightController, "baseIntensity", offsets.FlashlightController_baseIntensity, 0x0108);
    RESOLVE_FIELD(klassFlashlightController, "flickerMultiplier", offsets.FlashlightController_flickerMultiplier, 0x0164);
    RESOLVE_FIELD(klassFlashlightController, "flickerTimer", offsets.FlashlightController_flickerTimer, 0x0170);
    RESOLVE_FIELD(klassFlashlightController, "active", offsets.FlashlightController_active, 0x0070);
    RESOLVE_FIELD(klassFlashlightController, "LightActive", offsets.FlashlightController_LightActive, 0x007D);

    RESOLVE_FIELD(klassLightManager, "turnOffLights", offsets.LightManager_turnOffLights, 0x0080);
    RESOLVE_FIELD(klassLightManager, "turningOffLights", offsets.LightManager_turningOffLights, 0x0081);
    RESOLVE_FIELD(klassLightManager, "turningOffEmissions", offsets.LightManager_turningOffEmissions, 0x0082);

    RESOLVE_FIELD(klassPlayerAvatar, "playerAvatarVisuals", offsets.PlayerAvatar_playerAvatarVisuals, 0x0040);
    offsets.PlayerAvatarVisuals_meshParent = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "meshParent", -1);
    offsets.PlayerAvatarVisuals_headTop = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "attachPointTopHeadMiddle", -1);
    RESOLVE_FIELD(klassPlayerAvatar, "PlayerVisionTarget", offsets.PlayerAvatar_PlayerVisionTarget, 0x0068);
    RESOLVE_FIELD(klassPlayerVisionTarget, "VisionTransform", offsets.PlayerVisionTarget_VisionTransform, 0x0020);
    RESOLVE_FIELD(klassCameraPosition, "playerTransform", offsets.CameraPosition_playerTransform, 0x0018);
    RESOLVE_FIELD(klassCameraPosition, "playerOffset", offsets.CameraPosition_playerOffset, 0x0020);
    RESOLVE_FIELD(klassCameraPosition, "overridePositionActive", offsets.CameraPosition_overridePositionActive, 0x0034);
    RESOLVE_FIELD(klassCameraPosition, "overridePositionTimer", offsets.CameraPosition_overridePositionTimer, 0x0038);
    RESOLVE_FIELD(klassCameraPosition, "overridePositionTarget", offsets.CameraPosition_overridePositionTarget, 0x003C);

    offsets.PlayerController_overrideTurnRotationTimer = bridge.GetFieldOffset(klassPlayerController, "overrideTurnRotationTimer", -1);
    offsets.PlayerController_overrideTurnRotation = bridge.GetFieldOffset(klassPlayerController, "overrideTurnRotation", -1);
    offsets.PlayerController_cameraGameObject = bridge.GetFieldOffset(klassPlayerController, "cameraGameObject", -1);
    offsets.PlayerAvatar_localCamera = bridge.GetFieldOffset(klassPlayerAvatar, "localCamera", -1);
    offsets.PlayerLocalCamera_playerAvatar = bridge.GetFieldOffset(klassPlayerLocalCamera, "playerAvatar", -1);

    offsets.PlayerAvatar_rotationOverrideActive = bridge.GetFieldOffset(klassPlayerAvatar, "rotationOverrideActive", -1);
    offsets.PlayerAvatar_rotationDisabled = bridge.GetFieldOffset(klassPlayerAvatar, "rotationDisabled", -1);
    offsets.PlayerAvatar_isCrouching = bridge.GetFieldOffset(klassPlayerAvatar, "isCrouching", -1);
    offsets.PlayerAvatar_isSprinting = bridge.GetFieldOffset(klassPlayerAvatar, "isSprinting", -1);
    offsets.PlayerAvatar_isCrawling = bridge.GetFieldOffset(klassPlayerAvatar, "isCrawling", -1);
    offsets.PlayerAvatar_isSliding = bridge.GetFieldOffset(klassPlayerAvatar, "isSliding", -1);
    offsets.PlayerAvatar_isMoving = bridge.GetFieldOffset(klassPlayerAvatar, "isMoving", -1);

    offsets.PlayerAvatarVisuals_headUpTransform = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "headUpTransform", -1);
    offsets.PlayerAvatarVisuals_headSideTransform = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "headSideTransform", -1);
    offsets.PlayerAvatarVisuals_bodyTopUpTransform = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "bodyTopUpTransform", -1);
    offsets.PlayerAvatarVisuals_bodyTopSideTransform = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "bodyTopSideTransform", -1);
    offsets.PlayerAvatarVisuals_leanTransform = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "leanTransform", -1);
    offsets.PlayerAvatarVisuals_tiltTransform = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "tiltTransform", -1);
    offsets.PlayerAvatarVisuals_legTwistTransform = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "legTwistTransform", -1);
    offsets.PlayerAvatarVisuals_headTiltOverrideAmount = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "headTiltOverrideAmount", -1);
    offsets.PlayerAvatarVisuals_headTiltOverrideTimer = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "headTiltOverrideTimer", -1);
    offsets.PlayerAvatarVisuals_leanTwitchOverrideDegrees = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "leanTwitchOverrideDegrees", -1);
    offsets.PlayerAvatarVisuals_leanTwitchOverrideTimer = bridge.GetFieldOffset(klassPlayerAvatarVisuals, "leanTwitchOverrideTimer", -1);

    offsets.ValuableObject_physGrabObject = bridge.GetFieldOffset(klassValuableObject, "physGrabObject", -1);
    if (offsets.ValuableObject_physGrabObject > 0) LOG_OK("Field [ValuableObject::physGrabObject] -> 0x%X", offsets.ValuableObject_physGrabObject);
    offsets.ItemAttributes_physGrabObject = bridge.GetFieldOffset(klassItemAttributes, "physGrabObject", -1);
    if (offsets.ItemAttributes_physGrabObject > 0) LOG_OK("Field [ItemAttributes::physGrabObject] -> 0x%X", offsets.ItemAttributes_physGrabObject);
    offsets.PhysGrabObject_midPoint = bridge.GetFieldOffset(klassPhysGrabObject, "midPoint", -1);
    if (offsets.PhysGrabObject_midPoint > 0) LOG_OK("Field [PhysGrabObject::midPoint] -> 0x%X", offsets.PhysGrabObject_midPoint);
    offsets.PhysGrabObject_centerPoint = bridge.GetFieldOffset(klassPhysGrabObject, "centerPoint", -1);
    if (offsets.PhysGrabObject_centerPoint > 0) LOG_OK("Field [PhysGrabObject::centerPoint] -> 0x%X", offsets.PhysGrabObject_centerPoint);
    offsets.PhysGrabObject_grabbed = bridge.GetFieldOffset(klassPhysGrabObject, "grabbed", -1);
    if (offsets.PhysGrabObject_grabbed > 0) LOG_OK("Field [PhysGrabObject::grabbed] -> 0x%X", offsets.PhysGrabObject_grabbed);

    #undef RESOLVE_FIELD
    return valid;
}

bool GameState::Initialize() {
    auto& bridge = Mono::Bridge::Get();
    if (!bridge.AttachCurrentThread()) return false;
    MonoImage* appImage = bridge.GetAppImage();
    if (!appImage) return false;

    LOG_INFO("Resolving game classes in Assembly-CSharp...");

    #define GET_APP_CLASS(target, name) \
        target = bridge.FindClass(appImage, "", name); \
        if (target) { LOG_OK("Game Class [%s] -> %p", name, target); } \
        else { LOG_WARN("Game Class [%s] not found!", name); }

    GET_APP_CLASS(klassEnemyDirector, "EnemyDirector");
    GET_APP_CLASS(klassEnemyParent, "EnemyParent");
    GET_APP_CLASS(klassEnemy, "Enemy");
    GET_APP_CLASS(klassValuableDirector, "ValuableDirector");
    GET_APP_CLASS(klassValuableObject, "ValuableObject");
    GET_APP_CLASS(klassItemManager, "ItemManager");
    GET_APP_CLASS(klassItemAttributes, "ItemAttributes");
    GET_APP_CLASS(klassSemiFunc, "SemiFunc");
    GET_APP_CLASS(klassCameraUtils, "CameraUtils");
    GET_APP_CLASS(klassGameDirector, "GameDirector");
    GET_APP_CLASS(klassRunManager, "RunManager");
    GET_APP_CLASS(klassPlayerAvatar, "PlayerAvatar");
    GET_APP_CLASS(klassPlayerController, "PlayerController");
    GET_APP_CLASS(klassPhysGrabber, "PhysGrabber");
    GET_APP_CLASS(klassPhysGrabObject, "PhysGrabObject");
    GET_APP_CLASS(klassPlayerHealth, "PlayerHealth");
    GET_APP_CLASS(klassPlayerVoiceChat, "PlayerVoiceChat");
    GET_APP_CLASS(klassMetaManager, "MetaManager");
    GET_APP_CLASS(klassEnvironmentDirector, "EnvironmentDirector");
    GET_APP_CLASS(klassFlashlightController, "FlashlightController");
    GET_APP_CLASS(klassLightManager, "LightManager");
    GET_APP_CLASS(klassPlayerTumble, "PlayerTumble");
    GET_APP_CLASS(klassCameraPosition, "CameraPosition");
    GET_APP_CLASS(klassSpectateCamera, "SpectateCamera");
    GET_APP_CLASS(klassPlayerAvatarVisuals, "PlayerAvatarVisuals");
    GET_APP_CLASS(klassPlayerVisionTarget, "PlayerVisionTarget");
    GET_APP_CLASS(klassDebugCommandHandler, "DebugCommandHandler");
    GET_APP_CLASS(klassPlayerCosmetics, "PlayerCosmetics");
    GET_APP_CLASS(klassPlayerLocalCamera, "PlayerLocalCamera");
    GET_APP_CLASS(klassCameraAim, "CameraAim");
    if (klassCameraPosition) {
        methodCameraPositionOverride = bridge.FindMethod(klassCameraPosition, "OverridePosition", 2);
        if (methodCameraPositionOverride) {
            LOG_OK("Method [CameraPosition::OverridePosition] -> %p", methodCameraPositionOverride);
        } else {
            LOG_WARN("Method [CameraPosition::OverridePosition] not found!");
        }
    }

    if (klassPlayerAvatarVisuals) {
        methodShowSelfOverride = bridge.FindMethod(klassPlayerAvatarVisuals, "ShowSelfOverride", 1);
        if (methodShowSelfOverride) {
            LOG_OK("Method [PlayerAvatarVisuals::ShowSelfOverride] -> %p", methodShowSelfOverride);
        } else {
            LOG_WARN("Method [PlayerAvatarVisuals::ShowSelfOverride] not found!");
        }
    }

    if (klassMetaManager) {
        methodCosmeticUnlockAll = bridge.FindMethod(klassMetaManager, "CosmeticUnlockAll", 0);
        if (methodCosmeticUnlockAll) {
            LOG_OK("Method [MetaManager::CosmeticUnlockAll] -> %p", methodCosmeticUnlockAll);
        } else {
            LOG_WARN("Method [MetaManager::CosmeticUnlockAll] not found!");
        }
    }

    if (klassPlayerHealth) {
        methodPlayerHealthHeal = bridge.FindMethod(klassPlayerHealth, "Heal", 2);
        if (methodPlayerHealthHeal) {
            LOG_OK("Method [PlayerHealth::Heal] -> %p", methodPlayerHealthHeal);
        }
        methodPlayerHealthInvincibleSet = bridge.FindMethod(klassPlayerHealth, "InvincibleSet", 1);
        if (methodPlayerHealthInvincibleSet) {
            LOG_OK("Method [PlayerHealth::InvincibleSet] -> %p", methodPlayerHealthInvincibleSet);
        }
    }

    if (klassPlayerVoiceChat) {
        methodPlayerVoiceChatSetSpatialAudio = bridge.FindMethod(klassPlayerVoiceChat, "SetSpatialAudio", 1);
        if (methodPlayerVoiceChatSetSpatialAudio) {
            LOG_OK("Method [PlayerVoiceChat::SetSpatialAudio] -> %p", methodPlayerVoiceChatSetSpatialAudio);
        } else {
            LOG_WARN("Method [PlayerVoiceChat::SetSpatialAudio] not found!");
        }
    }

    if (klassPlayerAvatar) {
        methodOverrideAnimationSpeed = bridge.FindMethod(klassPlayerAvatar, "OverrideAnimationSpeed", 4);
        methodPlayerAvatarRevive = bridge.FindMethod(klassPlayerAvatar, "Revive", 1);
        if (methodPlayerAvatarRevive) {
            LOG_OK("Method [PlayerAvatar::Revive] -> %p", methodPlayerAvatarRevive);
        }

    }

    if (klassPlayerCosmetics) {
        methodPlayerCosmeticsSetupCosmetics = bridge.FindMethod(klassPlayerCosmetics, "SetupCosmetics", 3);
        if (methodPlayerCosmeticsSetupCosmetics) {
            LOG_OK("Method [PlayerCosmetics::SetupCosmetics] -> %p", methodPlayerCosmeticsSetupCosmetics);
        }
        methodPlayerCosmeticsSetupColors = bridge.FindMethod(klassPlayerCosmetics, "SetupColors", 2);
        if (methodPlayerCosmeticsSetupColors) {
            LOG_OK("Method [PlayerCosmetics::SetupColors] -> %p", methodPlayerCosmeticsSetupColors);
        }
    }

    if (bridge.GetPhotonImage()) {
        klassPhotonNetwork = bridge.FindClass(bridge.GetPhotonImage(), "Photon.Pun", "PhotonNetwork");
        if (klassPhotonNetwork) {
            methodPhotonInstantiate = bridge.FindMethod(klassPhotonNetwork, "Instantiate", 5);
            methodPhotonInstantiateRoom = bridge.FindMethod(klassPhotonNetwork, "InstantiateRoomObject", 5);
            if (methodPhotonInstantiate) {
                LOG_OK("Method [PhotonNetwork::Instantiate] -> %p", methodPhotonInstantiate);
            }
            if (methodPhotonInstantiateRoom) {
                LOG_OK("Method [PhotonNetwork::InstantiateRoomObject] -> %p", methodPhotonInstantiateRoom);
            }
        }
    }

    if (bridge.GetCoreImage()) {
        klassResources = bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "Resources");
        if (klassResources) {
            methodResourcesLoad = bridge.FindMethod(klassResources, "Load", 1);
            if (methodResourcesLoad) {
                LOG_OK("Method [Resources::Load] -> %p", methodResourcesLoad);
            }
        }
        klassObject = bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "Object");
        if (klassObject) {
            methodObjectInstantiate = bridge.FindMethod(klassObject, "Instantiate", 3);
            if (methodObjectInstantiate) {
                LOG_OK("Method [Object::Instantiate] -> %p", methodObjectInstantiate);
            }
        }
    }

    if (klassSemiFunc) {
        methodSemiFuncIsMultiplayer = bridge.FindMethod(klassSemiFunc, "IsMultiplayer", 0);
        methodSemiFuncIsMasterClient = bridge.FindMethod(klassSemiFunc, "IsMasterClient", 0);
        methodSemiFuncIsMasterOrSingle = bridge.FindMethod(klassSemiFunc, "IsMasterClientOrSingleplayer", 0);
    }

    #undef GET_APP_CLASS

    if (!ResolveOffsets()) return false;

    auto compileMethod = reinterpret_cast<void* (*)(MonoMethod*)>(GetProcAddress(
        GetModuleHandleA("mono-2.0-bdwgc.dll") ? GetModuleHandleA("mono-2.0-bdwgc.dll")
            : GetModuleHandleA("mono.dll"), "mono_compile_method"));
    if (!compileMethod) LOG_ERR("mono_compile_method export not found");
    MonoMethod* update = bridge.FindMethod(klassGameDirector, "Update", 0);
    if (!update) LOG_ERR("Method [GameDirector::Update] not found");
    void* updateCode = compileMethod && update ? compileMethod(update) : nullptr;
    if (update && compileMethod && !updateCode) LOG_ERR("mono_compile_method returned null for GameDirector::Update");

    for (int attempt = 0; !appearanceHookReady && attempt < 10; ++attempt) {
        if (attempt > 0) {
            if (updateCode) MH_RemoveHook(updateCode);
            originalPlayerUpdate = nullptr;
            Sleep(300);
            updateCode = compileMethod && update ? compileMethod(update) : nullptr;
        }
        if (!updateCode) continue;

        MH_STATUS createStatus = MH_CreateHook(updateCode, &PlayerUpdateHook,
            reinterpret_cast<void**>(&originalPlayerUpdate));
        if (createStatus != MH_OK) {
            LOG_WARN("MH_CreateHook(GameDirector::Update) attempt %d failed: %s", attempt + 1, MH_StatusToString(createStatus));
            continue;
        }

        MH_STATUS enableStatus = MH_EnableHook(updateCode);
        if (enableStatus != MH_OK) {
            LOG_WARN("MH_EnableHook(GameDirector::Update) attempt %d failed: %s", attempt + 1, MH_StatusToString(enableStatus));
            MH_RemoveHook(updateCode);
            originalPlayerUpdate = nullptr;
            continue;
        }
        appearanceHookReady = true;
    }
    if (!appearanceHookReady) {
        LOG_ERR("Game-thread Update hook failed; gameplay features disabled");
        return false;
    }

    Features::Removals::Get().Initialize();
    Features::PlayerMods::Get().Initialize();
    Features::AntiAim::Get().Initialize();

    m_Initialized = (klassEnemyDirector != nullptr || klassValuableDirector != nullptr);
    return m_Initialized;
}

bool GameState::IsMapLoadingOrRestarting() {
    if (klassRunManager) {
        uintptr_t runMgr = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(klassRunManager, "instance"));
        if (runMgr && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(runMgr))) {
            uint8_t restarting = 0;
            uint8_t handoffPending = 0;
            Memory::ReadByte(runMgr + offsets.RunManager_restarting, restarting);
            Memory::ReadByte(runMgr + offsets.RunManager_sceneHandoffNativePending, handoffPending);
            if (restarting || handoffPending) {
                return true;
            }
        }
    }

    if (klassGameDirector) {
        uintptr_t gameDir = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(klassGameDirector, "instance"));
        if (gameDir && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(gameDir))) {
            int32_t state = 0;
            Memory::ReadInt(gameDir + offsets.GameDirector_currentState, state);
            if (state == 0 || state == 3 || state == 4 || state == 5) {
                return true;
            }
        }
    }

    return false;
}

bool GameState::UnlockAllCosmetics(int* outUnlockedCount, int* outTotalCount) {
    if (outUnlockedCount) *outUnlockedCount = 0;
    if (outTotalCount) *outTotalCount = 0;

    auto& bridge = Mono::Bridge::Get();
    if (!klassMetaManager || !methodCosmeticUnlockAll) {
        LOG_WARN("UnlockAllCosmetics: MetaManager or CosmeticUnlockAll method not found");
        return false;
    }

    uintptr_t metaMgr = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassMetaManager, "instance"));
    if (!metaMgr || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(metaMgr))) {
        LOG_WARN("UnlockAllCosmetics: MetaManager.instance is null or invalid");
        return false;
    }

    bridge.AttachCurrentThread();
    MonoObject* exc = nullptr;
    MonoObject* res = bridge.InvokeMethod(methodCosmeticUnlockAll, reinterpret_cast<void*>(metaMgr), nullptr, &exc);
    if (exc) {
        LOG_ERR("UnlockAllCosmetics: CosmeticUnlockAll threw an exception");
        return false;
    }

    MonoClass* klassMenuPage = bridge.FindClass(bridge.GetAppImage(), "", "MenuPageCosmetics");
    if (klassMenuPage) {
        MonoMethod* methodRefresh = bridge.FindMethod(klassMenuPage, "RefreshScrollContent", 0);
        MonoClass* klassMenuMgr = bridge.FindClass(bridge.GetAppImage(), "", "MenuManager");
        if (methodRefresh && klassMenuMgr) {
            uintptr_t menuMgr = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassMenuMgr, "instance"));
            if (menuMgr && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(menuMgr))) {
                int offsetPage = bridge.GetFieldOffset(klassMenuMgr, "currentMenuPage", -1);
                if (offsetPage > 0) {
                    uintptr_t curPage = 0;
                    if (Memory::Read<uintptr_t>(menuMgr + offsetPage, curPage) && curPage) {
                        MonoObject* excRefresh = nullptr;
                        bridge.InvokeMethod(methodRefresh, reinterpret_cast<void*>(curPage), nullptr, &excRefresh);
                    }
                }
            }
        }
    }

    int u = 0, t = 0;
    GetCosmeticStats(u, t);
    if (outUnlockedCount) *outUnlockedCount = u;
    if (outTotalCount) *outTotalCount = t;

    LOG_OK("UnlockAllCosmetics: Successfully unlocked %d / %d cosmetics!", u, t);
    return true;
}

bool GameState::GetCosmeticStats(int& unlockedCount, int& totalCount) {
    unlockedCount = 0;
    totalCount = 0;

    if (!klassMetaManager) return false;

    uintptr_t metaMgr = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(klassMetaManager, "instance"));
    if (!metaMgr || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(metaMgr))) {
        return false;
    }

    uintptr_t unlocksList = 0;
    uintptr_t assetsList = 0;

    if (offsets.MetaManager_cosmeticUnlocks > 0) {
        Memory::ReadPtr(metaMgr + offsets.MetaManager_cosmeticUnlocks, unlocksList);
    }
    if (offsets.MetaManager_cosmeticAssets > 0) {
        Memory::ReadPtr(metaMgr + offsets.MetaManager_cosmeticAssets, assetsList);
    }

    if (unlocksList) {
        int count = 0;
        if (Memory::ReadMonoListCount(unlocksList, count)) {
            unlockedCount = count;
        }
    }

    if (assetsList) {
        int count = 0;
        if (Memory::ReadMonoListCount(assetsList, count)) {
            totalCount = count;
        }
    }
    return (totalCount > 0);
}

void GameState::SetAnimationSpeed(float targetSpeed) {
    if (!klassPlayerController || !methodOverrideAnimationSpeed) return;
    uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(klassPlayerController, "instance"));
    if (!playerCtrl || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) return;

    uintptr_t avatar = 0;
    if (Memory::ReadPtr(playerCtrl + offsets.PlayerController_playerAvatarScript, avatar) && avatar) {
        Mono::Bridge::Get().AttachCurrentThread();
        float inRate = 12.0f;
        float outRate = 12.0f;
        float duration = 0.5f;
        void* args[4] = { &targetSpeed, &inRate, &outRate, &duration };
        MonoObject* exc = nullptr;
        Mono::Bridge::Get().InvokeMethod(methodOverrideAnimationSpeed, reinterpret_cast<void*>(avatar), args, &exc);
    }
}

bool GameState::ReviveLocalPlayer() {
    auto& bridge = Mono::Bridge::Get();
    if (!klassPlayerAvatar || !methodPlayerAvatarRevive) return false;

    uintptr_t avatar = GetLocalPlayerAvatar();
    if (!avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) return false;

    bridge.AttachCurrentThread();
    bool revivedByTruck = false;
    void* args[1] = { &revivedByTruck };
    MonoObject* exc = nullptr;
    bridge.InvokeMethod(methodPlayerAvatarRevive, reinterpret_cast<void*>(avatar), args, &exc);
    if (exc) {
        LOG_ERR("GameState::ReviveLocalPlayer: Exception thrown in PlayerAvatar.Revive");
        return false;
    }

    LOG_OK("GameState::ReviveLocalPlayer: Revive invoked successfully!");
    return true;
}

bool GameState::HealLocalPlayer(int healAmount) {
    auto& bridge = Mono::Bridge::Get();
    if (!klassPlayerAvatar) return false;

    uintptr_t avatar = GetLocalPlayerAvatar();
    if (!avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) return false;

    uintptr_t healthObj = 0;
    if (!Memory::ReadPtr(avatar + offsets.PlayerAvatar_playerHealth, healthObj) || !healthObj ||
        !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(healthObj))) {
        return false;
    }

    int curHp = 0, maxHp = 100;
    Memory::ReadInt(healthObj + offsets.PlayerHealth_health, curHp);
    Memory::ReadInt(healthObj + offsets.PlayerHealth_maxHealth, maxHp);
    if (maxHp <= 0) maxHp = 100;

    if (healAmount <= 0) {
        healAmount = maxHp - curHp;
    }
    if (healAmount <= 0) return true;

    if (methodPlayerHealthHeal) {
        bridge.AttachCurrentThread();
        bool effect = false;
        void* args[2] = { &healAmount, &effect };
        MonoObject* exc = nullptr;
        bridge.InvokeMethod(methodPlayerHealthHeal, reinterpret_cast<void*>(healthObj), args, &exc);
    }

    Memory::WriteInt(healthObj + offsets.PlayerHealth_health, maxHp);
    return true;
}

uintptr_t GameState::GetLocalPlayerAvatar() {
    auto& bridge = Mono::Bridge::Get();
    uintptr_t avatar = 0;
    if (klassPlayerAvatar) {
        avatar = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassPlayerAvatar, "instance"));
    }
    if (!avatar && klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassPlayerController, "instance"));
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            Memory::ReadPtr(playerCtrl + offsets.PlayerController_playerAvatarScript, avatar);
        }
    }
    if (!avatar && klassGameDirector) {
        uintptr_t gameDir = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassGameDirector, "instance"));
        if (gameDir && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(gameDir))) {
            uintptr_t playerList = 0;
            if (Memory::ReadPtr(gameDir + offsets.GameDirector_PlayerList, playerList)) {
                std::vector<uintptr_t> avatars;
                if (Memory::ReadMonoList(playerList, avatars)) {
                    for (uintptr_t a : avatars) {
                        if (!a || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(a))) continue;
                        uint8_t isLoc = 0;
                        Memory::ReadByte(a + offsets.PlayerAvatar_isLocal, isLoc);
                        if (isLoc) {
                            avatar = a;
                            break;
                        }
                    }
                }
            }
        }
    }
    return (avatar && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) ? avatar : 0;
}

bool GameState::CopyPlayerCosmetics(uintptr_t targetAvatarAddress) {
    if (!targetAvatarAddress || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(targetAvatarAddress))) return false;
    auto& bridge = Mono::Bridge::Get();
    bridge.AttachCurrentThread();

    uintptr_t localAvatar = GetLocalPlayerAvatar();
    if (!localAvatar) return false;

    uintptr_t targetCosmetics = 0;
    uintptr_t localCosmetics = 0;
    Memory::ReadPtr(targetAvatarAddress + offsets.PlayerAvatar_playerCosmetics, targetCosmetics);
    Memory::ReadPtr(localAvatar + offsets.PlayerAvatar_playerCosmetics, localCosmetics);

    if (!Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(targetCosmetics)) ||
        !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(localCosmetics)) ||
        !methodPlayerCosmeticsSetupCosmetics) return false;

    uintptr_t targetEquippedRaw = 0;
    if (offsets.PlayerCosmetics_cosmeticEquippedRaw > 0) {
        Memory::ReadPtr(targetCosmetics + offsets.PlayerCosmetics_cosmeticEquippedRaw, targetEquippedRaw);
    }

    if (targetEquippedRaw) {
        uint8_t synced = 0;
        uint8_t forced = 1;
        void* args[3] = { &synced, &forced, reinterpret_cast<void*>(targetEquippedRaw) };

        if (!bridge.TryInvokeMethod(methodPlayerCosmeticsSetupCosmetics, reinterpret_cast<void*>(localCosmetics), args)) {
            LOG_WARN("CopyPlayerCosmetics: SetupCosmetics threw an exception");
            return false;
        }
        synced = 1;
        if (!bridge.TryInvokeMethod(methodPlayerCosmeticsSetupCosmetics, reinterpret_cast<void*>(localCosmetics), args)) return false;
        LOG_OK("CopyPlayerCosmetics: Applied and synced cosmetics to network");
        return true;
    }
    return false;
}

bool GameState::CopyPlayerColors(uintptr_t targetAvatarAddress) {
    if (!targetAvatarAddress || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(targetAvatarAddress))) return false;
    auto& bridge = Mono::Bridge::Get();
    bridge.AttachCurrentThread();

    uintptr_t localAvatar = GetLocalPlayerAvatar();
    if (!localAvatar) return false;

    uintptr_t targetCosmetics = 0;
    uintptr_t localCosmetics = 0;
    Memory::ReadPtr(targetAvatarAddress + offsets.PlayerAvatar_playerCosmetics, targetCosmetics);
    Memory::ReadPtr(localAvatar + offsets.PlayerAvatar_playerCosmetics, localCosmetics);

    if (!Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(targetCosmetics)) ||
        !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(localCosmetics)) ||
        !methodPlayerCosmeticsSetupColors) return false;

    uintptr_t targetColorsEquipped = 0;
    if (offsets.PlayerCosmetics_colorsEquipped > 0) {
        Memory::ReadPtr(targetCosmetics + offsets.PlayerCosmetics_colorsEquipped, targetColorsEquipped);
    }

    if (targetColorsEquipped) {
        uint8_t synced = 0;
        void* args[2] = { &synced, reinterpret_cast<void*>(targetColorsEquipped) };
        if (!bridge.TryInvokeMethod(methodPlayerCosmeticsSetupColors, reinterpret_cast<void*>(localCosmetics), args)) {
            LOG_WARN("CopyPlayerColors: SetupColors threw an exception");
            return false;
        }
        synced = 1;
        if (!bridge.TryInvokeMethod(methodPlayerCosmeticsSetupColors, reinterpret_cast<void*>(localCosmetics), args)) return false;
        LOG_OK("CopyPlayerColors: Applied and synced colors to network");
        return true;
    }
    return false;
}

std::vector<ServerPlayerInfo> GameState::GetServerPlayers() {
    std::vector<ServerPlayerInfo> list;
    auto& bridge = Mono::Bridge::Get();
    if (!klassGameDirector) return list;

    uintptr_t gameDir = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassGameDirector, "instance"));
    if (!gameDir || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(gameDir))) return list;

    uintptr_t playerList = 0;
    if (!Memory::ReadPtr(gameDir + offsets.GameDirector_PlayerList, playerList)) return list;

    std::vector<uintptr_t> avatars;
    if (!Memory::ReadMonoList(playerList, avatars)) return list;

    for (uintptr_t avatar : avatars) {
        if (!avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) continue;

        ServerPlayerInfo info;
        info.address = avatar;

        uint8_t isLoc = 0;
        Memory::ReadByte(avatar + offsets.PlayerAvatar_isLocal, isLoc);
        info.isLocal = (isLoc != 0);

        uint8_t dead = 0;
        Memory::ReadByte(avatar + offsets.PlayerAvatar_deadSet, dead);
        info.isDead = (dead != 0);

        uintptr_t namePtr = 0;
        if (Memory::ReadPtr(avatar + offsets.PlayerAvatar_playerName, namePtr) && namePtr) {
            info.name = bridge.ReadMonoString(reinterpret_cast<MonoString*>(namePtr));
        }
        if (info.name.empty()) {
            info.name = info.isLocal ? "Local Player" : "Player";
        }

        uintptr_t healthObj = 0;
        if (Memory::ReadPtr(avatar + offsets.PlayerAvatar_playerHealth, healthObj) && healthObj &&
            Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(healthObj))) {
            Memory::ReadInt(healthObj + offsets.PlayerHealth_health, info.health);
        } else {
            info.health = info.isDead ? 0 : 100;
        }

        list.push_back(info);
    }

    return list;
}


bool GameState::IsMultiplayer() {
    if (!methodSemiFuncIsMultiplayer) return false;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(methodSemiFuncIsMultiplayer, nullptr, nullptr, nullptr);
    if (!res) return false;
    bool isMulti = false;
    Mono::Bridge::Get().Unbox<bool>(res, isMulti);
    return isMulti;
}

bool GameState::IsMasterClient() {
    if (!methodSemiFuncIsMasterClient) return false;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(methodSemiFuncIsMasterClient, nullptr, nullptr, nullptr);
    if (!res) return false;
    bool isMaster = false;
    Mono::Bridge::Get().Unbox<bool>(res, isMaster);
    return isMaster;
}

bool GameState::SpawnItemInternal(const std::string& resourcePath, const Unity::Vector3& position, const Unity::Quaternion& rotation, int mode) {
    auto& bridge = Mono::Bridge::Get();
    if (!bridge.IsReady() || resourcePath.empty()) return false;
    bridge.AttachCurrentThread();

    bool isMulti = IsMultiplayer();
    bool isMaster = IsMasterClient();

    bool useClientNetwork = false;
    bool useRoomNetwork = false;

    if (mode == 1) {
        useClientNetwork = true;
    } else if (mode == 2) {
        useRoomNetwork = true;
    } else if (mode == 3) {
        useClientNetwork = false;
        useRoomNetwork = false;
    } else {
        if (isMulti) {
            if (isMaster) {
                useRoomNetwork = true;
            } else {
                useClientNetwork = true;
            }
        }
    }

    MonoObject* exc = nullptr;
    if (useClientNetwork || useRoomNetwork) {
        if (!klassPhotonNetwork && bridge.GetPhotonImage()) {
            klassPhotonNetwork = bridge.FindClass(bridge.GetPhotonImage(), "Photon.Pun", "PhotonNetwork");
        }
        if (!methodPhotonInstantiate && klassPhotonNetwork) {
            methodPhotonInstantiate = bridge.FindMethod(klassPhotonNetwork, "Instantiate", 5);
        }
        if (!methodPhotonInstantiateRoom && klassPhotonNetwork) {
            methodPhotonInstantiateRoom = bridge.FindMethod(klassPhotonNetwork, "InstantiateRoomObject", 5);
        }

        MonoMethod* targetMethod = useRoomNetwork ? methodPhotonInstantiateRoom : methodPhotonInstantiate;
        if (!targetMethod && useRoomNetwork) {
            targetMethod = methodPhotonInstantiate;
        }

        if (targetMethod) {
            MonoString* monoStr = bridge.CreateString(resourcePath.c_str());
            if (!monoStr) return false;

            Unity::Vector3 pos = position;
            Unity::Quaternion rot = rotation;
            uint8_t group = 0;

            void* args[5] = {
                monoStr,
                &pos,
                &rot,
                &group,
                nullptr
            };

            MonoObject* spawnedObj = bridge.InvokeMethod(targetMethod, nullptr, args, &exc);
            if (exc) {
                LOG_ERR("SpawnItemInternal network exception: %p", exc);
                if (useRoomNetwork && methodPhotonInstantiate) {
                    exc = nullptr;
                    spawnedObj = bridge.InvokeMethod(methodPhotonInstantiate, nullptr, args, &exc);
                }
            }
            if (spawnedObj && !exc) {
                LOG_OK("SpawnItemInternal spawned %s (Network %s)", resourcePath.c_str(), (targetMethod == methodPhotonInstantiateRoom && !exc) ? "Room" : "Client");
                return true;
            }
        }
    }

    if (!klassResources && bridge.GetCoreImage()) {
        klassResources = bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "Resources");
    }
    if (!methodResourcesLoad && klassResources) {
        methodResourcesLoad = bridge.FindMethod(klassResources, "Load", 1);
    }
    if (!klassObject && bridge.GetCoreImage()) {
        klassObject = bridge.FindClass(bridge.GetCoreImage(), "UnityEngine", "Object");
    }
    if (!methodObjectInstantiate && klassObject) {
        methodObjectInstantiate = bridge.FindMethod(klassObject, "Instantiate", 3);
    }

    if (methodResourcesLoad && methodObjectInstantiate) {
        MonoString* monoStr = bridge.CreateString(resourcePath.c_str());
        if (!monoStr) return false;

        void* loadArgs[1] = { monoStr };
        MonoObject* prefab = bridge.InvokeMethod(methodResourcesLoad, nullptr, loadArgs, &exc);
        if (!prefab || exc) {
            LOG_ERR("Resources.Load failed for %s", resourcePath.c_str());
            return false;
        }

        Unity::Vector3 pos = position;
        Unity::Quaternion rot = rotation;
        void* instArgs[3] = {
            prefab,
            &pos,
            &rot
        };

        MonoObject* spawnedObj = bridge.InvokeMethod(methodObjectInstantiate, nullptr, instArgs, &exc);
        if (exc) {
            LOG_ERR("Object.Instantiate exception: %p", exc);
            return false;
        }
        if (spawnedObj) {
            LOG_OK("SpawnItemInternal spawned %s (Local)", resourcePath.c_str());
            return true;
        }
    }

    return false;
}

}
