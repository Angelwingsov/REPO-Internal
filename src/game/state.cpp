#include "state.h"
#include "core/logger.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"

namespace Game {

GameState& GameState::Get() {
    static GameState instance;
    return instance;
}

void GameState::ResolveOffsets() {
    auto& bridge = Mono::Bridge::Get();
    LOG_INFO("Dynamically resolving runtime field offsets via Mono metadata...");

    #define RESOLVE_FIELD(klass, fieldName, targetVar, fallback) \
        targetVar = bridge.GetFieldOffset(klass, fieldName, fallback); \
        LOG_OK("Field [%s::%s] -> 0x%X (+%d)", #klass, fieldName, targetVar, targetVar);

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
    RESOLVE_FIELD(klassPlayerAvatar, "PlayerVisionTarget", offsets.PlayerAvatar_PlayerVisionTarget, 0x0068);
    RESOLVE_FIELD(klassPlayerVisionTarget, "VisionTransform", offsets.PlayerVisionTarget_VisionTransform, 0x0020);
    RESOLVE_FIELD(klassCameraPosition, "playerTransform", offsets.CameraPosition_playerTransform, 0x0018);
    RESOLVE_FIELD(klassCameraPosition, "playerOffset", offsets.CameraPosition_playerOffset, 0x0020);
    RESOLVE_FIELD(klassCameraPosition, "overridePositionActive", offsets.CameraPosition_overridePositionActive, 0x0034);
    RESOLVE_FIELD(klassCameraPosition, "overridePositionTimer", offsets.CameraPosition_overridePositionTimer, 0x0038);
    RESOLVE_FIELD(klassCameraPosition, "overridePositionTarget", offsets.CameraPosition_overridePositionTarget, 0x003C);

    #undef RESOLVE_FIELD
}

bool GameState::Initialize() {
    auto& bridge = Mono::Bridge::Get();
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

    if (klassPlayerAvatar) {
        methodOverrideAnimationSpeed = bridge.FindMethod(klassPlayerAvatar, "OverrideAnimationSpeed", 4);
        methodPlayerAvatarRevive = bridge.FindMethod(klassPlayerAvatar, "Revive", 1);
        if (methodPlayerAvatarRevive) {
            LOG_OK("Method [PlayerAvatar::Revive] -> %p", methodPlayerAvatarRevive);
        }
    }

    #undef GET_APP_CLASS

    ResolveOffsets();

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

    if (outUnlockedCount || outTotalCount) {
        int unlocked = 0, total = 0;
        if (GetCosmeticStats(unlocked, total)) {
            if (outUnlockedCount) *outUnlockedCount = unlocked;
            if (outTotalCount) *outTotalCount = total;
        }
    }

    LOG_OK("UnlockAllCosmetics: All cosmetics unlocked and saved successfully!");
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
    Memory::Read<uintptr_t>(metaMgr + offsets.MetaManager_cosmeticUnlocks, unlocksList);
    Memory::Read<uintptr_t>(metaMgr + offsets.MetaManager_cosmeticAssets, assetsList);

    if (unlocksList) {
        Memory::ReadInt(unlocksList + 0x18, unlockedCount);
    }
    if (assetsList) {
        Memory::ReadInt(assetsList + 0x18, totalCount);
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

    uintptr_t avatar = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassPlayerAvatar, "instance"));
    if (!avatar && klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassPlayerController, "instance"));
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            Memory::ReadPtr(playerCtrl + offsets.PlayerController_playerAvatarScript, avatar);
        }
    }
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

    uintptr_t avatar = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassPlayerAvatar, "instance"));
    if (!avatar && klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassPlayerController, "instance"));
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            Memory::ReadPtr(playerCtrl + offsets.PlayerController_playerAvatarScript, avatar);
        }
    }
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

}
