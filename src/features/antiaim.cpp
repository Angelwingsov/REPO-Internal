#include "antiaim.h"
#include "animbreaker.h"
#include "core/config.h"
#include "core/keybind.h"
#include "core/logger.h"
#include "core/hook_activity.h"
#include "hook/input.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "minhook/include/MinHook.h"
#include <cmath>
#include <cstdlib>

namespace Features {

AntiAim& AntiAim::Get() {
    static AntiAim instance;
    return instance;
}

bool AntiAim::Initialize() {
    auto& bridge = Mono::Bridge::Get();
    if (!bridge.AttachCurrentThread()) return false;

    auto& gameState = Game::GameState::Get();
    HMODULE mono = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!mono) mono = GetModuleHandleA("mono.dll");
    if (!mono) return false;

    auto compileMethod = reinterpret_cast<void* (*)(MonoMethod*)>(GetProcAddress(mono, "mono_compile_method"));
    if (!compileMethod) return false;

    if (!s_HookAvatarInstalled.load() && gameState.klassPlayerAvatar) {
        MonoMethod* methodAvatar = bridge.FindMethod(gameState.klassPlayerAvatar, "OnPhotonSerializeView", 2);
        if (methodAvatar) {
            void* codeAvatar = compileMethod(methodAvatar);
            if (codeAvatar) {
                if (MH_CreateHook(codeAvatar, &HookOnPhotonSerializeView, reinterpret_cast<void**>(&s_OriginalOnPhotonSerializeView)) == MH_OK) {
                    if (MH_EnableHook(codeAvatar) == MH_OK) {
                        s_HookTargetAvatar = codeAvatar;
                        s_HookAvatarInstalled.store(true);
                        LOG_OK("PlayerAvatar::OnPhotonSerializeView hooked successfully!");
                    } else {
                        MH_RemoveHook(codeAvatar);
                    }
                }
            }
        }
    }

    if (!s_HookCameraInstalled.load() && gameState.klassPlayerLocalCamera) {
        MonoMethod* methodCamera = bridge.FindMethod(gameState.klassPlayerLocalCamera, "OnPhotonSerializeView", 2);
        if (methodCamera) {
            void* codeCamera = compileMethod(methodCamera);
            if (codeCamera) {
                if (MH_CreateHook(codeCamera, &HookCameraOnPhotonSerializeView, reinterpret_cast<void**>(&s_OriginalCameraOnPhotonSerializeView)) == MH_OK) {
                    if (MH_EnableHook(codeCamera) == MH_OK) {
                        s_HookTargetCamera = codeCamera;
                        s_HookCameraInstalled.store(true);
                        LOG_OK("PlayerLocalCamera::OnPhotonSerializeView hooked successfully!");
                    } else {
                        MH_RemoveHook(codeCamera);
                    }
                }
            }
        }
    }

    return s_HookAvatarInstalled.load() || s_HookCameraInstalled.load();
}

bool AntiAim::RestoreBeforeUnload() {
    if (s_HookAvatarInstalled.load() && s_HookTargetAvatar) {
        MH_DisableHook(s_HookTargetAvatar);
        MH_RemoveHook(s_HookTargetAvatar);
        s_OriginalOnPhotonSerializeView = nullptr;
        s_HookTargetAvatar = nullptr;
        s_HookAvatarInstalled.store(false);
    }
    if (s_HookCameraInstalled.load() && s_HookTargetCamera) {
        MH_DisableHook(s_HookTargetCamera);
        MH_RemoveHook(s_HookTargetCamera);
        s_OriginalCameraOnPhotonSerializeView = nullptr;
        s_HookTargetCamera = nullptr;
        s_HookCameraInstalled.store(false);
    }
    return true;
}

bool AntiAim::IsLocalAvatar(void* avatarPtr) const {
    if (!avatarPtr) return false;
    auto& gs = Game::GameState::Get();
    uintptr_t target = reinterpret_cast<uintptr_t>(avatarPtr);
    uintptr_t localAvatar = gs.GetLocalPlayerAvatar();
    if (localAvatar && target == localAvatar) return true;
    if (gs.offsets.PlayerAvatar_isLocal > 0) {
        uint8_t loc = 0;
        if (Memory::ReadByte(target + gs.offsets.PlayerAvatar_isLocal, loc) && loc != 0) {
            return true;
        }
    }
    return false;
}

bool AntiAim::IsLocalCamera(void* cameraPtr) const {
    if (!cameraPtr) return false;
    auto& gs = Game::GameState::Get();
    uintptr_t target = reinterpret_cast<uintptr_t>(cameraPtr);

    uintptr_t localAvatar = gs.GetLocalPlayerAvatar();
    if (localAvatar && gs.offsets.PlayerAvatar_localCamera > 0) {
        uintptr_t localCam = 0;
        if (Memory::ReadPtr(localAvatar + gs.offsets.PlayerAvatar_localCamera, localCam) && localCam == target) {
            return true;
        }
    }

    if (gs.offsets.PlayerLocalCamera_playerAvatar > 0) {
        uintptr_t av = 0;
        if (Memory::ReadPtr(target + gs.offsets.PlayerLocalCamera_playerAvatar, av) && av) {
            if (localAvatar && av == localAvatar) return true;
            if (gs.offsets.PlayerAvatar_isLocal > 0) {
                uint8_t loc = 0;
                if (Memory::ReadByte(av + gs.offsets.PlayerAvatar_isLocal, loc) && loc != 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

MonoObject* AntiAim::GetCameraAimInstance() const {
    auto& gs = Game::GameState::Get();
    if (!gs.klassCameraAim) return nullptr;
    return reinterpret_cast<MonoObject*>(
        Mono::Bridge::Get().GetStaticFieldValue(gs.klassCameraAim, "Instance")
    );
}

void __fastcall AntiAim::HookOnPhotonSerializeView(void* this_ptr, void* stream, void* info) {
    Core::HookActivity activity;

    auto& aa = AntiAim::Get();
    auto& cfg = Core::Config::Get();

    bool shouldAA = cfg.antiAim.enabled && aa.IsActive() && (cfg.antiAim.yawMode > 0 || cfg.antiAim.rollMode > 0);
    bool shouldSpine = cfg.animBreaker.enabled && cfg.animBreaker.spineBreaker;

    auto& gameState = Game::GameState::Get();

    MonoObject* playerCtrlTransform = nullptr;
    Unity::Quaternion savedRot{};
    bool rotSwapped = false;

    if ((shouldAA || shouldSpine) && aa.IsLocalAvatar(this_ptr) && gameState.klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(
            Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance")
        );
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            playerCtrlTransform = Unity::Engine::Get().GetTransform(reinterpret_cast<MonoObject*>(playerCtrl));
            if (playerCtrlTransform) {
                savedRot = Unity::Engine::Get().GetTransformRotation(playerCtrlTransform);

                float yaw = aa.GetCurrentYaw();
                float roll = aa.GetCurrentRoll();

                if (shouldSpine) {
                    roll += (aa.m_JitterFlip ? cfg.animBreaker.spineTwitchAmount * 0.5f : -cfg.animBreaker.spineTwitchAmount * 0.5f);
                }

                Unity::Quaternion aaRot = Unity::Quaternion::Euler(0.0f, yaw, roll);
                if (aaRot.IsValid()) {
                    Unity::Engine::Get().SetTransformRotation(playerCtrlTransform, aaRot);
                    rotSwapped = true;
                }
            }
        }
    }

    if (s_OriginalOnPhotonSerializeView) {
        s_OriginalOnPhotonSerializeView(this_ptr, stream, info);
    }

    if (rotSwapped && playerCtrlTransform) {
        Unity::Engine::Get().SetTransformRotation(playerCtrlTransform, savedRot);
    }
}

void __fastcall AntiAim::HookCameraOnPhotonSerializeView(void* this_ptr, void* stream, void* info) {
    Core::HookActivity activity;

    auto& aa = AntiAim::Get();
    auto& cfg = Core::Config::Get();

    bool shouldAA = cfg.antiAim.enabled && aa.IsActive();
    bool shouldHeadBreaker = cfg.animBreaker.enabled && cfg.animBreaker.headBreaker;

    MonoObject* camAimTransform = nullptr;
    Unity::Quaternion savedCamRot{};
    bool camSwapped = false;

    if ((shouldAA || shouldHeadBreaker) && aa.IsLocalCamera(this_ptr)) {
        MonoObject* camAim = aa.GetCameraAimInstance();
        if (camAim && Unity::Engine::IsValidUnityObject(camAim)) {
            camAimTransform = Unity::Engine::Get().GetTransform(camAim);
            if (camAimTransform) {
                savedCamRot = Unity::Engine::Get().GetTransformRotation(camAimTransform);

                Unity::Vector3 camEuler = savedCamRot.ToEuler();

                float pitch = 0.0f;
                if (shouldAA && cfg.antiAim.pitchMode > 0) {
                    pitch = aa.GetCurrentPitch();
                } else {
                    pitch = camEuler.x;
                    if (pitch > 180.0f) pitch -= 360.0f;
                }

                float yaw = camEuler.y;
                if (shouldHeadBreaker) {
                    yaw += Features::AnimBreaker::Get().GetHeadAngle();
                } else if (shouldAA && cfg.antiAim.yawMode > 0) {
                    yaw = aa.GetCurrentYaw();
                }

                float roll = 0.0f;
                if (shouldAA && cfg.antiAim.rollMode > 0) {
                    roll = aa.GetCurrentRoll();
                }

                Unity::Quaternion aaCamRot = Unity::Quaternion::Euler(pitch, yaw, roll);
                if (aaCamRot.IsValid()) {
                    Unity::Engine::Get().SetTransformRotation(camAimTransform, aaCamRot);
                    camSwapped = true;
                }
            }
        }
    }

    if (s_OriginalCameraOnPhotonSerializeView) {
        s_OriginalCameraOnPhotonSerializeView(this_ptr, stream, info);
    }

    if (camSwapped && camAimTransform) {
        Unity::Engine::Get().SetTransformRotation(camAimTransform, savedCamRot);
    }
}

void AntiAim::Reset() {
    m_PrevKeyDown = false;
    m_ActiveState = false;
    m_Active = false;
    m_SpinYaw = 0.0f;
    m_PitchSpin = 0.0f;
    m_JitterTimer = 0.0f;
    m_JitterFlip = false;
    m_WasActive = false;
    m_CurrentPitch = 0.0f;
    m_CurrentYaw = 0.0f;
    m_CurrentRoll = 0.0f;
    m_CurrentRotation = Unity::Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
}

void AntiAim::Update(float dt) {
    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        Reset();
        return;
    }

    if (!s_HookAvatarInstalled.load() || !s_HookCameraInstalled.load()) {
        Initialize();
    }

    auto& cfg = Core::Config::Get();

    bool shouldBeActive = false;
    if (cfg.antiAim.enabled) {
        if (cfg.antiAim.toggleKey > 0) {
            bool isDown = Hook::InputHook::IsKeyHeld(cfg.antiAim.toggleKey);
            shouldBeActive = Core::EvaluateKeybind(cfg.antiAim.keyMode, isDown, m_PrevKeyDown, m_ActiveState);
            m_PrevKeyDown = isDown;
        } else {
            shouldBeActive = true;
        }
    } else {
        shouldBeActive = false;
        m_ActiveState = false;
        m_PrevKeyDown = false;
    }

    cfg.antiAim.active = shouldBeActive;
    m_Active = shouldBeActive;

    if (!gameState.klassPlayerController) return;

    uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(
        Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance")
    );
    if (!playerCtrl || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
        m_WasActive = false;
        return;
    }

    uintptr_t avatar = 0;
    if (!Memory::ReadPtr(playerCtrl + gameState.offsets.PlayerController_playerAvatarScript, avatar) ||
        !avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
        m_WasActive = false;
        return;
    }

    uint8_t isDead = 0;
    if (Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_deadSet, isDead) && isDead != 0) {
        shouldBeActive = false;
        m_Active = false;
        cfg.antiAim.active = false;
    }

    MonoObject* avatarObj = reinterpret_cast<MonoObject*>(avatar);
    MonoObject* avatarTransform = Unity::Engine::Get().GetTransform(avatarObj);

    if (!shouldBeActive) {
        if (m_WasActive) {
            MonoObject* ctrlTransform = Unity::Engine::Get().GetTransform(reinterpret_cast<MonoObject*>(playerCtrl));
            if (ctrlTransform && avatarTransform) {
                Unity::Quaternion normalRot = Unity::Engine::Get().GetTransformRotation(ctrlTransform);
                if (normalRot.IsValid()) {
                    Unity::Engine::Get().SetTransformRotation(avatarTransform, normalRot);
                }
            }
            if (gameState.offsets.PlayerAvatar_rotationOverrideActive > 0) {
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_rotationOverrideActive, 0);
            }
            m_WasActive = false;
        }
        return;
    }

    m_WasActive = true;

    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    MonoObject* activeCam = Unity::Engine::Get().FindActiveCamera();
    if (activeCam) {
        MonoObject* camTransform = Unity::Engine::Get().GetTransform(activeCam);
        if (camTransform) {
            Unity::Vector3 fwd = Unity::Engine::Get().GetTransformForward(camTransform);
            if (fwd.IsValid() && (fwd.x != 0.0f || fwd.z != 0.0f)) {
                cameraYaw = std::atan2(fwd.x, fwd.z) * 57.2957795f;
            }
            Unity::Quaternion camRot = Unity::Engine::Get().GetTransformRotation(camTransform);
            if (camRot.IsValid()) {
                cameraPitch = camRot.ToEuler().x;
                if (cameraPitch > 180.0f) cameraPitch -= 360.0f;
            }
        }
    } else {
        MonoObject* ctrlTransform = Unity::Engine::Get().GetTransform(reinterpret_cast<MonoObject*>(playerCtrl));
        if (ctrlTransform) {
            Unity::Vector3 fwd = Unity::Engine::Get().GetTransformForward(ctrlTransform);
            if (fwd.IsValid() && (fwd.x != 0.0f || fwd.z != 0.0f)) {
                cameraYaw = std::atan2(fwd.x, fwd.z) * 57.2957795f;
            }
        }
    }

    float targetYaw = cameraYaw;
    switch (cfg.antiAim.yawMode) {
    case 1:
        m_SpinYaw += cfg.antiAim.spinSpeed * dt;
        if (m_SpinYaw >= 360.0f) m_SpinYaw = std::fmod(m_SpinYaw, 360.0f);
        if (m_SpinYaw < 0.0f) m_SpinYaw += 360.0f;
        targetYaw = m_SpinYaw;
        break;
    case 2:
        m_JitterTimer += dt;
        if (m_JitterTimer >= 0.06f) {
            m_JitterFlip = !m_JitterFlip;
            m_JitterTimer = 0.0f;
        }
        targetYaw = cameraYaw + (m_JitterFlip ? cfg.antiAim.jitterRange : -cfg.antiAim.jitterRange);
        break;
    case 3:
        targetYaw = cameraYaw + 180.0f;
        break;
    case 4:
        targetYaw = cameraYaw + 90.0f;
        break;
    case 5:
        targetYaw = static_cast<float>(rand() % 360);
        break;
    default:
        break;
    }

    while (targetYaw < 0.0f) targetYaw += 360.0f;
    while (targetYaw >= 360.0f) targetYaw -= 360.0f;

    float targetRoll = 0.0f;
    switch (cfg.antiAim.rollMode) {
    case 1:
        targetRoll = cfg.antiAim.leanAngle;
        break;
    case 2:
        targetRoll = m_JitterFlip ? cfg.antiAim.leanAngle : -cfg.antiAim.leanAngle;
        break;
    case 3:
        targetRoll = m_JitterFlip ? 45.0f : -45.0f;
        break;
    default:
        break;
    }

    float targetPitch = cameraPitch;
    if (cfg.antiAim.pitchMode > 0) {
        switch (cfg.antiAim.pitchMode) {
        case 1:
            targetPitch = 85.0f;
            break;
        case 2:
            targetPitch = -85.0f;
            break;
        case 3:
            targetPitch = m_JitterFlip ? 85.0f : -85.0f;
            break;
        case 4:
            targetPitch = 0.0f;
            break;
        case 5:
            m_PitchSpin += 360.0f * dt;
            if (m_PitchSpin >= 360.0f) m_PitchSpin = std::fmod(m_PitchSpin, 360.0f);
            targetPitch = std::sin(m_PitchSpin * 0.0174532925f) * 85.0f;
            break;
        }
    }

    m_CurrentPitch = targetPitch;
    m_CurrentYaw = targetYaw;
    m_CurrentRoll = targetRoll;

    m_CurrentRotation = Unity::Quaternion::Euler(0.0f, targetYaw, targetRoll);

    if (avatarTransform && (cfg.antiAim.yawMode > 0 || cfg.antiAim.rollMode > 0)) {
        Unity::Engine::Get().SetTransformRotation(avatarTransform, m_CurrentRotation);
    }

    uintptr_t visuals = 0;
    if (Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerAvatarVisuals, visuals) && visuals &&
        Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(visuals))) {

        if (cfg.antiAim.rollMode == 3) {
            if (gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideDegrees > 0 &&
                gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideTimer > 0) {
                Memory::WriteFloat(visuals + gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideDegrees, 75.0f);
                Memory::WriteFloat(visuals + gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideTimer, 0.25f);
            }
        }

        if (cfg.antiAim.pitchMode > 0) {
            if (gameState.offsets.PlayerAvatarVisuals_headTiltOverrideAmount > 0 &&
                gameState.offsets.PlayerAvatarVisuals_headTiltOverrideTimer > 0) {
                Memory::WriteFloat(visuals + gameState.offsets.PlayerAvatarVisuals_headTiltOverrideAmount, targetPitch);
                Memory::WriteFloat(visuals + gameState.offsets.PlayerAvatarVisuals_headTiltOverrideTimer, 0.25f);
            }
        }
    }
}

}
