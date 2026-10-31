#include "player.h"
#include "core/config.h"
#include "core/logger.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "mono/mono_bridge.h"
#include "hook/input.h"

namespace Features {

PlayerMods& PlayerMods::Get() {
    static PlayerMods instance;
    return instance;
}

const char* PlayerMods::GetKeyName(int vkCode) {
    switch (vkCode) {
    case 0: return "NONE";
    case VK_LBUTTON: return "M1";
    case VK_RBUTTON: return "M2";
    case VK_MBUTTON: return "M3 (Wheel)";
    case VK_XBUTTON1: return "M4";
    case VK_XBUTTON2: return "M5";
    case VK_MWHEELUP: return "MWheelUp";
    case VK_MWHEELDOWN: return "MWheelDown";
    case VK_SHIFT:
    case VK_LSHIFT: return "LSHIFT";
    case VK_RSHIFT: return "RSHIFT";
    case VK_CONTROL:
    case VK_LCONTROL: return "LCTRL";
    case VK_RCONTROL: return "RCTRL";
    case VK_MENU:
    case VK_LMENU: return "LALT";
    case VK_RMENU: return "RALT";
    case VK_SPACE: return "SPACE";
    case VK_TAB: return "TAB";
    case VK_CAPITAL: return "CAPS";
    case VK_ESCAPE: return "ESC";
    case VK_DELETE: return "DELETE";
    case VK_INSERT: return "INSERT";
    case VK_HOME: return "HOME";
    case VK_END: return "END";
    case VK_PRIOR: return "PGUP";
    case VK_NEXT: return "PGDN";
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    default: {
        static char bufs[4][16];
        static int bufIdx = 0;
        char* cur = bufs[bufIdx++ % 4];
        if ((vkCode >= 'A' && vkCode <= 'Z') || (vkCode >= '0' && vkCode <= '9')) {
            cur[0] = static_cast<char>(vkCode);
            cur[1] = '\0';
            return cur;
        }
        sprintf_s(cur, 16, "Key %d", vkCode);
        return cur;
    }
    }
}

void PlayerMods::Update() {
    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        m_LastPhysGrabber = 0;
        m_LastPlayerCtrl = 0;
        m_LastGrabbedObject = 0;
        m_GrabRangeSaved = false;
        m_StrengthSaved = false;
        m_JumpForceSaved = false;
        m_SpeedSaved = false;
        m_SpeedWasActive = false;
        return;
    }

    auto& cfg = Core::Config::Get();

    bool shouldBeActive = false;
    if (cfg.movement.speedEnabled) {
        if (cfg.movement.speedAlwaysOn) {
            shouldBeActive = true;
        } else if (cfg.movement.speedKey > 0) {
            bool isDown = Hook::InputHook::IsKeyHeld(cfg.movement.speedKey);
            if (cfg.movement.speedKeyMode == 0) {
                shouldBeActive = isDown;
            } else {
                if (isDown && !m_PrevSpeedKeyDown) {
                    m_SpeedActiveState = !m_SpeedActiveState;
                }
                shouldBeActive = m_SpeedActiveState;
            }
            m_PrevSpeedKeyDown = isDown;
        } else {
            shouldBeActive = true;
        }
    } else {
        shouldBeActive = false;
        m_SpeedActiveState = false;
        m_PrevSpeedKeyDown = false;
    }

    cfg.movement.speedActive = shouldBeActive;

    if (gameState.klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance"));
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            if (m_LastPlayerCtrl != playerCtrl) {
                m_LastPlayerCtrl = playerCtrl;
                m_JumpForceSaved = false;
                m_SpeedSaved = false;
                m_SpeedWasActive = false;
            }

            if (cfg.movement.infiniteStamina) {
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_EnergyCurrent, 100.0f);
                Memory::WriteByte(playerCtrl + gameState.offsets.PlayerController_DebugEnergy, 1);
            } else {
                uint8_t debugEnergy = 0;
                if (Memory::ReadByte(playerCtrl + gameState.offsets.PlayerController_DebugEnergy, debugEnergy) && debugEnergy != 0) {
                    Memory::WriteByte(playerCtrl + gameState.offsets.PlayerController_DebugEnergy, 0);
                }
            }

            if (shouldBeActive) {
                if (!m_SpeedSaved) {
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_MoveSpeed, m_OriginalMoveSpeed);
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_SprintSpeed, m_OriginalSprintSpeed);
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_CrouchSpeed, m_OriginalCrouchSpeed);

                    float origMove = 0.0f, origSprint = 0.0f, origCrouch = 0.0f;
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_playerOriginalMoveSpeed, origMove);
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_playerOriginalSprintSpeed, origSprint);
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_playerOriginalCrouchSpeed, origCrouch);

                    if (origMove > 0.01f) m_OriginalMoveSpeed = origMove;
                    if (origSprint > 0.01f) m_OriginalSprintSpeed = origSprint;
                    if (origCrouch > 0.01f) m_OriginalCrouchSpeed = origCrouch;

                    m_SpeedSaved = true;
                }

                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_overrideSpeedMultiplier, cfg.movement.speedMultiplier);
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_overrideSpeedTimer, 0.25f);
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_overrideSpeedSprintCap, -1.0f);

                float targetMove = m_OriginalMoveSpeed * cfg.movement.speedMultiplier;
                float targetSprint = m_OriginalSprintSpeed * cfg.movement.speedMultiplier;
                float targetCrouch = m_OriginalCrouchSpeed * cfg.movement.speedMultiplier;
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_MoveSpeed, targetMove);
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_SprintSpeed, targetSprint);
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_CrouchSpeed, targetCrouch);

                m_SpeedWasActive = true;
            } else {
                if (m_SpeedWasActive) {
                    float restoreMove = m_OriginalMoveSpeed;
                    float restoreSprint = m_OriginalSprintSpeed;
                    float restoreCrouch = m_OriginalCrouchSpeed;

                    float origMove = 0.0f, origSprint = 0.0f, origCrouch = 0.0f;
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_playerOriginalMoveSpeed, origMove);
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_playerOriginalSprintSpeed, origSprint);
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_playerOriginalCrouchSpeed, origCrouch);

                    if (origMove > 0.01f) restoreMove = origMove;
                    if (origSprint > 0.01f) restoreSprint = origSprint;
                    if (origCrouch > 0.01f) restoreCrouch = origCrouch;

                    if (restoreMove > 0.01f) Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_MoveSpeed, restoreMove);
                    if (restoreSprint > 0.01f) Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_SprintSpeed, restoreSprint);
                    if (restoreCrouch > 0.01f) Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_CrouchSpeed, restoreCrouch);

                    Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_overrideSpeedTimer, 0.0f);
                    Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_overrideSpeedMultiplier, 1.0f);
                    Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_overrideSpeedSprintCap, -1.0f);

                    m_SpeedWasActive = false;
                    m_SpeedSaved = false;
                }
            }

            if (cfg.movement.superJump) {
                if (!m_JumpForceSaved) {
                    Memory::ReadFloat(playerCtrl + gameState.offsets.PlayerController_JumpForce, m_OriginalJumpForce);
                    if (m_OriginalJumpForce <= 0.0f || m_OriginalJumpForce > 100.0f) {
                        m_OriginalJumpForce = 8.0f;
                    }
                    m_JumpForceSaved = true;
                }
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_JumpForce, m_OriginalJumpForce * cfg.movement.jumpMultiplier);
            } else {
                if (m_JumpForceSaved) {
                    Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_JumpForce, m_OriginalJumpForce);
                    m_JumpForceSaved = false;
                }
            }

            if (cfg.movement.noTumble) {
                Memory::WriteByte(playerCtrl + gameState.offsets.PlayerController_DebugNoTumble, 1);
            } else {
                uint8_t debugNoTumble = 0;
                if (Memory::ReadByte(playerCtrl + gameState.offsets.PlayerController_DebugNoTumble, debugNoTumble) && debugNoTumble != 0) {
                    Memory::WriteByte(playerCtrl + gameState.offsets.PlayerController_DebugNoTumble, 0);
                }
            }
        }
    }

    if (gameState.klassPhysGrabber) {
        uintptr_t physGrabber = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPhysGrabber, "instance"));
        if (physGrabber && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(physGrabber))) {
            if (m_LastPhysGrabber != physGrabber) {
                m_LastPhysGrabber = physGrabber;
                m_GrabRangeSaved = false;
                m_StrengthSaved = false;
                m_LastGrabbedObject = 0;
            }

            if (cfg.movement.infiniteStrength) {
                if (!m_StrengthSaved) {
                    Memory::ReadFloat(physGrabber + gameState.offsets.PhysGrabber_grabStrength, m_OriginalGrabStrength);
                    Memory::ReadFloat(physGrabber + gameState.offsets.PhysGrabber_throwStrength, m_OriginalThrowStrength);
                    Memory::ReadFloat(physGrabber + gameState.offsets.PhysGrabber_forceMax, m_OriginalForceMax);
                    Memory::ReadFloat(physGrabber + gameState.offsets.PhysGrabber_climbForceMultiplier, m_OriginalClimbForceMultiplier);

                    if (m_OriginalGrabStrength <= 0.0f || m_OriginalGrabStrength > 100.0f) m_OriginalGrabStrength = 1.0f;
                    if (m_OriginalForceMax <= 0.0f || m_OriginalForceMax > 10.0f) m_OriginalForceMax = 4.0f;
                    if (m_OriginalClimbForceMultiplier <= 0.0f || m_OriginalClimbForceMultiplier > 50.0f) m_OriginalClimbForceMultiplier = 1.0f;

                    m_StrengthSaved = true;
                }

                float str = cfg.movement.strengthMultiplier;
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_grabStrength, str);

                float targetForceMax = m_OriginalForceMax * (1.0f + str * 0.15f);
                if (targetForceMax > 50.0f) targetForceMax = 50.0f;
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_forceMax, targetForceMax);

                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_climbForceMultiplier, 5.0f);
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_overrideGrabStrength, str);
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_overrideGrabStrengthTimer, 0.25f);
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_throwStrength, m_OriginalThrowStrength + str * 0.1f);

                uintptr_t grabbedObj = 0;
                if (Memory::Read<uintptr_t>(physGrabber + gameState.offsets.PhysGrabber_grabbedPhysGrabObject, grabbedObj) && grabbedObj) {
                    Memory::WriteFloat(grabbedObj + gameState.offsets.PhysGrabObject_overrideGrabStrength, str);
                    Memory::WriteFloat(grabbedObj + gameState.offsets.PhysGrabObject_overrideGrabStrengthTimer, 0.25f);
                    Memory::WriteFloat(grabbedObj + gameState.offsets.PhysGrabObject_alterMassValue, 0.2f);
                    Memory::WriteFloat(grabbedObj + gameState.offsets.PhysGrabObject_timerAlterMass, 0.25f);
                }

                if (m_LastGrabbedObject && m_LastGrabbedObject != grabbedObj) {
                    Memory::WriteFloat(m_LastGrabbedObject + gameState.offsets.PhysGrabObject_timerAlterMass, 0.0f);
                    Memory::WriteFloat(m_LastGrabbedObject + gameState.offsets.PhysGrabObject_overrideGrabStrength, -1.0f);
                    Memory::WriteFloat(m_LastGrabbedObject + gameState.offsets.PhysGrabObject_overrideGrabStrengthTimer, 0.0f);
                }
                m_LastGrabbedObject = grabbedObj;
            } else {
                if (m_StrengthSaved) {
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_grabStrength, m_OriginalGrabStrength);
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_throwStrength, m_OriginalThrowStrength);
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_forceMax, m_OriginalForceMax);
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_climbForceMultiplier, m_OriginalClimbForceMultiplier);
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_overrideGrabStrength, -1.0f);
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_overrideGrabStrengthTimer, 0.0f);

                    if (m_LastGrabbedObject) {
                        Memory::WriteFloat(m_LastGrabbedObject + gameState.offsets.PhysGrabObject_timerAlterMass, 0.0f);
                        Memory::WriteFloat(m_LastGrabbedObject + gameState.offsets.PhysGrabObject_overrideGrabStrength, -1.0f);
                        Memory::WriteFloat(m_LastGrabbedObject + gameState.offsets.PhysGrabObject_overrideGrabStrengthTimer, 0.0f);
                        m_LastGrabbedObject = 0;
                    }
                    m_StrengthSaved = false;
                }
            }

            if (cfg.movement.extendGrabRange) {
                if (!m_GrabRangeSaved) {
                    Memory::ReadFloat(physGrabber + gameState.offsets.PhysGrabber_grabRange, m_OriginalGrabRange);
                    Memory::ReadFloat(physGrabber + gameState.offsets.PhysGrabber_grabReleaseDistance, m_OriginalGrabReleaseDistance);
                    Memory::ReadFloat(physGrabber + gameState.offsets.PhysGrabber_maxDistanceFromPlayer, m_OriginalMaxDistanceFromPlayer);
                    if (m_OriginalGrabRange <= 0.0f || m_OriginalGrabRange > 100.0f) m_OriginalGrabRange = 4.0f;
                    if (m_OriginalGrabReleaseDistance <= 0.0f || m_OriginalGrabReleaseDistance > 200.0f) m_OriginalGrabReleaseDistance = 8.0f;
                    if (m_OriginalMaxDistanceFromPlayer <= 0.0f || m_OriginalMaxDistanceFromPlayer > 100.0f) m_OriginalMaxDistanceFromPlayer = 2.5f;
                    m_GrabRangeSaved = true;
                }
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_grabRange, cfg.movement.grabRange);
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_grabReleaseDistance, cfg.movement.grabRange * 2.5f + 10.0f);
                Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_maxDistanceFromPlayer, cfg.movement.grabRange);
            } else {
                if (m_GrabRangeSaved) {
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_grabRange, m_OriginalGrabRange);
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_grabReleaseDistance, m_OriginalGrabReleaseDistance);
                    Memory::WriteFloat(physGrabber + gameState.offsets.PhysGrabber_maxDistanceFromPlayer, m_OriginalMaxDistanceFromPlayer);
                    m_GrabRangeSaved = false;
                }
            }
        }
    }

    if (gameState.klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance"));
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            if (cfg.troll.zeroGravity) {
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_antiGravityTimer, 0.25f);
            }

            if (cfg.troll.featherFall) {
                Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_featherTimer, 0.25f);
            }

            uintptr_t avatar = 0;
            if (Memory::Read<uintptr_t>(playerCtrl + gameState.offsets.PlayerController_playerAvatarScript, avatar) && avatar) {
                if (cfg.troll.globalVoice) {
                    uintptr_t voiceChat = 0;
                    if (Memory::Read<uintptr_t>(avatar + gameState.offsets.PlayerAvatar_voiceChat, voiceChat) && voiceChat) {
                        Memory::WriteFloat(voiceChat + gameState.offsets.PlayerVoiceChat_SpatialDisableTimer, 0.5f);
                    }
                }

                if (cfg.troll.overrideAnimSpeed) {
                    gameState.SetAnimationSpeed(cfg.troll.animSpeed);
                }
            }
        }
    }
}

}
