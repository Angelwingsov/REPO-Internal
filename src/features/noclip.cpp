#include "noclip.h"
#include "core/config.h"
#include "core/logger.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "mono/mono_bridge.h"
#include "hook/input.h"
#include "imgui/imgui.h"
#include <algorithm>

namespace Features {

NoClip& NoClip::Get() {
    static NoClip instance;
    return instance;
}

void NoClip::RestorePhysics() {
    auto& gameState = Game::GameState::Get();
    auto& cfg = Core::Config::Get();

    if (m_LastPlayerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(m_LastPlayerCtrl))) {
        Memory::WriteFloat(m_LastPlayerCtrl + gameState.offsets.PlayerController_kinematicTimer, 0.0f);
        Memory::Write<Unity::Vector3>(m_LastPlayerCtrl + gameState.offsets.PlayerController_Velocity, Unity::Vector3{});
        Memory::Write<Unity::Vector3>(m_LastPlayerCtrl + gameState.offsets.PlayerController_VelocityImpulse, Unity::Vector3{});
        Memory::WriteByte(m_LastPlayerCtrl + gameState.offsets.PlayerController_DebugNoTumble, cfg.movement.noTumble ? 1 : 0);
    }

    if (m_LastPlayerAvatar && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(m_LastPlayerAvatar))) {
        Memory::WriteFloat(m_LastPlayerAvatar + gameState.offsets.PlayerAvatar_fallDamageResetTimer, 2.0f);
        Memory::WriteByte(m_LastPlayerAvatar + gameState.offsets.PlayerAvatar_fallDamageResetState, 1);
    }

    m_LastPlayerCtrl = 0;
    m_LastPlayerAvatar = 0;
}

void NoClip::Update() {
    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        if (m_WasActive) {
            RestorePhysics();
            m_WasActive = false;
        }
        m_ActiveState = false;
        m_PrevKeyDown = false;
        Core::Config::Get().noclip.active = false;
        return;
    }

    auto& cfg = Core::Config::Get();

    bool shouldBeActive = false;
    if (cfg.noclip.enabled) {
        if (cfg.noclip.toggleKey > 0) {
            bool isDown = Hook::InputHook::IsKeyHeld(cfg.noclip.toggleKey);
            if (cfg.noclip.keyMode == 0) {
                shouldBeActive = isDown;
            } else {
                if (isDown && !m_PrevKeyDown) {
                    m_ActiveState = !m_ActiveState;
                }
                shouldBeActive = m_ActiveState;
            }
            m_PrevKeyDown = isDown;
        } else {
            shouldBeActive = true;
        }
    } else {
        shouldBeActive = false;
        m_ActiveState = false;
        m_PrevKeyDown = false;
    }

    cfg.noclip.active = shouldBeActive;

    if (!gameState.klassPlayerController) {
        if (m_WasActive) {
            RestorePhysics();
            m_WasActive = false;
        }
        return;
    }

    uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(
        Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance")
    );
    if (!playerCtrl || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
        if (m_WasActive) {
            RestorePhysics();
            m_WasActive = false;
        }
        return;
    }

    uintptr_t avatar = 0;
    Memory::Read<uintptr_t>(playerCtrl + gameState.offsets.PlayerController_playerAvatarScript, avatar);

    if (avatar && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
        uint8_t isDead = 0;
        if (Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_deadSet, isDead) && isDead != 0) {
            shouldBeActive = false;
            cfg.noclip.active = false;
            m_ActiveState = false;
        }
    }

    if (!shouldBeActive) {
        if (m_WasActive) {
            RestorePhysics();
            m_WasActive = false;
        }
        return;
    }

    m_WasActive = true;
    m_LastPlayerCtrl = playerCtrl;
    m_LastPlayerAvatar = avatar;

    Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_kinematicTimer, 0.25f);
    Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_antiGravityTimer, 0.25f);
    Memory::WriteFloat(playerCtrl + gameState.offsets.PlayerController_featherTimer, 0.25f);
    Memory::WriteByte(playerCtrl + gameState.offsets.PlayerController_DebugNoTumble, 1);
    Memory::Write<Unity::Vector3>(playerCtrl + gameState.offsets.PlayerController_Velocity, Unity::Vector3{});
    Memory::Write<Unity::Vector3>(playerCtrl + gameState.offsets.PlayerController_VelocityImpulse, Unity::Vector3{});

    if (avatar && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
        Memory::WriteFloat(avatar + gameState.offsets.PlayerAvatar_fallDamageResetTimer, 2.0f);
        Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_fallDamageResetState, 1);
    }

    MonoObject* playerTransform = Unity::Engine::Get().GetTransform(reinterpret_cast<MonoObject*>(playerCtrl));
    if (!playerTransform) return;

    Unity::Vector3 curPos = Unity::Engine::Get().GetTransformPosition(playerTransform);
    if (!curPos.IsValid()) return;

    MonoObject* cam = Unity::Engine::Get().FindActiveCamera();
    MonoObject* camTrans = cam ? Unity::Engine::Get().GetTransform(cam) : nullptr;
    Unity::Vector3 fwd = camTrans ? Unity::Engine::Get().GetTransformForward(camTrans) : Unity::Vector3{ 0.0f, 0.0f, 1.0f };
    Unity::Vector3 right = camTrans ? Unity::Engine::Get().GetTransformRight(camTrans) : Unity::Vector3{ 1.0f, 0.0f, 0.0f };
    Unity::Vector3 worldUp{ 0.0f, 1.0f, 0.0f };

    if (!cfg.noclip.verticalFollowCam) {
        fwd.y = 0.0f;
        fwd = fwd.Normalized();
        right.y = 0.0f;
        right = right.Normalized();
    }

    Unity::Vector3 moveDir{ 0.0f, 0.0f, 0.0f };
    bool isMoving = false;

    if (!cfg.menu.isVisible && (!Hook::InputHook::s_Window || GetForegroundWindow() == Hook::InputHook::s_Window)) {
        if (Hook::InputHook::IsKeyHeld('W') || Hook::InputHook::IsKeyHeld(VK_UP)) {
            moveDir = moveDir + fwd;
            isMoving = true;
        }
        if (Hook::InputHook::IsKeyHeld('S') || Hook::InputHook::IsKeyHeld(VK_DOWN)) {
            moveDir = moveDir - fwd;
            isMoving = true;
        }
        if (Hook::InputHook::IsKeyHeld('D') || Hook::InputHook::IsKeyHeld(VK_RIGHT)) {
            moveDir = moveDir + right;
            isMoving = true;
        }
        if (Hook::InputHook::IsKeyHeld('A') || Hook::InputHook::IsKeyHeld(VK_LEFT)) {
            moveDir = moveDir - right;
            isMoving = true;
        }
        if (Hook::InputHook::IsKeyHeld(VK_SPACE)) {
            moveDir = moveDir + worldUp;
            isMoving = true;
        }
        if (Hook::InputHook::IsKeyHeld(VK_CONTROL) || Hook::InputHook::IsKeyHeld(VK_LCONTROL) || Hook::InputHook::IsKeyHeld('C')) {
            moveDir = moveDir - worldUp;
            isMoving = true;
        }
    }

    float speed = cfg.noclip.speed;
    if (Hook::InputHook::IsKeyHeld(VK_SHIFT) || Hook::InputHook::IsKeyHeld(VK_LSHIFT)) {
        speed *= cfg.noclip.sprintMultiplier;
    }

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;

    if (isMoving && moveDir.LengthSq() > 0.0001f) {
        moveDir = moveDir.Normalized();
        Unity::Vector3 newPos = curPos + (moveDir * (speed * dt));

        if (newPos.y < -80.0f) {
            newPos.y = -80.0f;
        }

        Unity::Engine::Get().SetTransformPosition(playerTransform, newPos);
        Memory::Write<Unity::Vector3>(playerCtrl + gameState.offsets.PlayerController_positionPrevious, newPos);
    } else if (cfg.noclip.freezeWhenIdle) {
        Unity::Engine::Get().SetTransformPosition(playerTransform, curPos);
        Memory::Write<Unity::Vector3>(playerCtrl + gameState.offsets.PlayerController_positionPrevious, curPos);
    }
}

}
