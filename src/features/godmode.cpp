#include "core/keybind.h"
#include "godmode.h"
#include "core/config.h"
#include "core/logger.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "mono/mono_bridge.h"
#include "hook/input.h"

namespace Features {

GodMode& GodMode::Get() {
    static GodMode instance;
    return instance;
}

bool GodMode::ReviveLocalPlayer() {
    return Game::GameState::Get().ReviveLocalPlayer();
}

bool GodMode::HealLocalPlayer(int healAmount) {
    return Game::GameState::Get().HealLocalPlayer(healAmount);
}

void GodMode::Update() {
    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        m_LastPlayerAvatar = 0;
        m_LastPlayerHealth = 0;
        m_GodModeSaved = false;
        return;
    }

    auto& cfg = Core::Config::Get();

    bool shouldBeActive = false;
    if (cfg.godMode.enabled) {
        if (cfg.godMode.toggleKey > 0) {
            bool isDown = Hook::InputHook::IsKeyHeld(cfg.godMode.toggleKey);
            shouldBeActive = Core::EvaluateKeybind(cfg.godMode.keyMode, isDown, m_PrevKeyDown, m_ActiveState);
            m_PrevKeyDown = isDown;
        } else {
            shouldBeActive = true;
        }
    } else {
        shouldBeActive = false;
        m_ActiveState = false;
        m_PrevKeyDown = false;
    }

    cfg.godMode.active = shouldBeActive;

    auto& bridge = Mono::Bridge::Get();
    uintptr_t avatar = 0;
    if (gameState.klassPlayerAvatar) {
        avatar = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(gameState.klassPlayerAvatar, "instance"));
    }
    if (!avatar && gameState.klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(gameState.klassPlayerController, "instance"));
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            Memory::ReadPtr(playerCtrl + gameState.offsets.PlayerController_playerAvatarScript, avatar);
        }
    }

    if (!avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
        return;
    }

    uintptr_t healthObj = 0;
    Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerHealth, healthObj);
    bool validHealth = (healthObj && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(healthObj)));

    if (shouldBeActive) {
        if (validHealth) {
            if (m_LastPlayerHealth != healthObj) {
                m_LastPlayerHealth = healthObj;
                m_GodModeSaved = false;
            }

            int curHp = 0, maxHp = 100;
            Memory::ReadInt(healthObj + gameState.offsets.PlayerHealth_health, curHp);
            Memory::ReadInt(healthObj + gameState.offsets.PlayerHealth_maxHealth, maxHp);
            if (maxHp <= 0) maxHp = 100;

            if (cfg.godMode.healToMax && curHp < maxHp) {
                Memory::WriteInt(healthObj + gameState.offsets.PlayerHealth_health, maxHp);
                if (gameState.methodPlayerHealthHeal) {
                    bridge.AttachCurrentThread();
                    int healAmt = maxHp - curHp;
                    bool effect = false;
                    void* args[2] = { &healAmt, &effect };
                    MonoObject* exc = nullptr;
                    bridge.InvokeMethod(gameState.methodPlayerHealthHeal, reinterpret_cast<void*>(healthObj), args, &exc);
                }
            }

            if (cfg.godMode.preventDeath) {
                Memory::WriteByte(healthObj + gameState.offsets.PlayerHealth_godMode, 1);
                Memory::WriteFloat(healthObj + gameState.offsets.PlayerHealth_invincibleTimer, 5.0f);
                Memory::WriteByte(healthObj + gameState.offsets.PlayerHealth_hurtFreeze, 0);
                Memory::WriteFloat(healthObj + gameState.offsets.PlayerHealth_hurtFreezeTimer, 0.0f);
            }

            m_GodModeSaved = true;
        }

        if (gameState.klassDebugCommandHandler) {
            uintptr_t debugCmd = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(gameState.klassDebugCommandHandler, "instance"));
            if (debugCmd && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(debugCmd))) {
                Memory::WriteByte(debugCmd + gameState.offsets.DebugCommandHandler_godMode, 1);
            }
        }

        if (cfg.godMode.noFallDamage) {
            Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_fallDamageResetState, 1);
            Memory::WriteFloat(avatar + gameState.offsets.PlayerAvatar_fallDamageResetTimer, 2.0f);

            uintptr_t tumble = 0;
            if (Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_tumble, tumble) && tumble &&
                Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(tumble))) {
                Memory::WriteFloat(tumble + gameState.offsets.PlayerTumble_impactHurtTimer, 0.0f);
                Memory::WriteInt(tumble + gameState.offsets.PlayerTumble_impactHurtDamage, 0);
            }
        }

        if (cfg.godMode.autoRevive) {
            uint8_t deadSet = 0;
            if (Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_deadSet, deadSet) && deadSet != 0) {
                ReviveLocalPlayer();
            }
        }
    } else {
        if (m_GodModeSaved) {
            if (validHealth) {
                Memory::WriteByte(healthObj + gameState.offsets.PlayerHealth_godMode, 0);
                Memory::WriteFloat(healthObj + gameState.offsets.PlayerHealth_invincibleTimer, 0.0f);
            }

            if (gameState.klassDebugCommandHandler) {
                uintptr_t debugCmd = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(gameState.klassDebugCommandHandler, "instance"));
                if (debugCmd && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(debugCmd))) {
                    Memory::WriteByte(debugCmd + gameState.offsets.DebugCommandHandler_godMode, 0);
                }
            }

            m_GodModeSaved = false;
        }
    }
}

}
