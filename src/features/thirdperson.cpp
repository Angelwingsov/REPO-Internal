#include "core/keybind.h"
#include "thirdperson.h"
#include "core/config.h"
#include "core/logger.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include "mono/mono_bridge.h"
#include "hook/input.h"

namespace Features {

ThirdPerson& ThirdPerson::Get() {
    static ThirdPerson instance;
    return instance;
}

void ThirdPerson::Update() {
    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        m_WasActive = false;
        return;
    }

    auto& cfg = Core::Config::Get();

    bool shouldBeActive = false;
    if (cfg.thirdPerson.enabled) {
        if (cfg.thirdPerson.toggleKey > 0) {
            bool isDown = Hook::InputHook::IsKeyHeld(cfg.thirdPerson.toggleKey);
            shouldBeActive = Core::EvaluateKeybind(cfg.thirdPerson.keyMode, isDown, m_PrevKeyDown, m_ActiveState);
            m_PrevKeyDown = isDown;
        } else {
            shouldBeActive = true;
        }
    } else {
        shouldBeActive = false;
        m_ActiveState = false;
        m_PrevKeyDown = false;
    }

    cfg.thirdPerson.active = shouldBeActive;

    if (!gameState.klassCameraPosition || !gameState.methodCameraPositionOverride) {
        return;
    }

    uintptr_t camPos = reinterpret_cast<uintptr_t>(
        Mono::Bridge::Get().GetStaticFieldValue(gameState.klassCameraPosition, "instance")
    );
    if (!camPos || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(camPos))) {
        return;
    }

    uintptr_t avatar = 0;
    if (gameState.klassPlayerController) {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(
            Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance")
        );
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            Memory::Read<uintptr_t>(playerCtrl + gameState.offsets.PlayerController_playerAvatarScript, avatar);
        }
    }
    if (!avatar && gameState.klassPlayerAvatar) {
        avatar = reinterpret_cast<uintptr_t>(
            Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerAvatar, "instance")
        );
    }

    if (avatar && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
        uint8_t isDead = 0;
        if (Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_deadSet, isDead) && isDead != 0) {
            shouldBeActive = false;
        }
    }

    uintptr_t avatarVisuals = 0;
    if (avatar && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
        Memory::Read<uintptr_t>(avatar + gameState.offsets.PlayerAvatar_playerAvatarVisuals, avatarVisuals);
    }

    if (shouldBeActive) {
        Unity::Vector3 headPos{};
        bool headFound = false;

        if (avatar) {
            uintptr_t visionTarget = 0;
            if (Memory::Read<uintptr_t>(avatar + gameState.offsets.PlayerAvatar_PlayerVisionTarget, visionTarget) && visionTarget) {
                uintptr_t visionTransform = 0;
                if (Memory::Read<uintptr_t>(visionTarget + gameState.offsets.PlayerVisionTarget_VisionTransform, visionTransform) && visionTransform) {
                    headPos = Unity::Engine::Get().GetTransformPosition(reinterpret_cast<MonoObject*>(visionTransform));
                    if (headPos.IsValid() && !headPos.IsZero()) {
                        headFound = true;
                    }
                }
            }
        }

        if (!headFound) {
            uintptr_t playerTransform = 0;
            if (Memory::Read<uintptr_t>(camPos + gameState.offsets.CameraPosition_playerTransform, playerTransform) && playerTransform) {
                headPos = Unity::Engine::Get().GetTransformPosition(reinterpret_cast<MonoObject*>(playerTransform));
                Unity::Vector3 pOffset{};
                if (Memory::Read<Unity::Vector3>(camPos + gameState.offsets.CameraPosition_playerOffset, pOffset) && pOffset.IsValid()) {
                    headPos = headPos + pOffset;
                } else {
                    headPos.y += 1.4f;
                }
                if (headPos.IsValid()) {
                    headFound = true;
                }
            }
        }

        if (!headFound || !headPos.IsValid()) {
            return;
        }

        MonoObject* activeCam = Unity::Engine::Get().FindActiveCamera();
        if (!activeCam) {
            return;
        }

        MonoObject* camTransform = Unity::Engine::Get().GetTransform(activeCam);
        if (!camTransform) {
            return;
        }

        Unity::Vector3 fwd = Unity::Engine::Get().GetTransformForward(camTransform);
        Unity::Vector3 right = Unity::Engine::Get().GetTransformRight(camTransform);
        Unity::Vector3 up = Unity::Engine::Get().GetTransformUp(camTransform);

        Unity::Vector3 targetCamPos = headPos - (fwd * cfg.thirdPerson.distance)
                                              + (right * cfg.thirdPerson.shoulderOffset)
                                              + (up * cfg.thirdPerson.heightOffset);

        if (cfg.thirdPerson.collision) {
            Unity::Vector3 hitPoint{};
            if (Unity::Engine::Get().Linecast(headPos, targetCamPos, hitPoint)) {
                Unity::Vector3 toHead = (headPos - hitPoint).Normalized();
                targetCamPos = hitPoint + (toHead * 0.18f);
            }
        }

        void* overrideParams[2];
        overrideParams[0] = &targetCamPos;
        float duration = 0.25f;
        overrideParams[1] = &duration;

        MonoObject* exc = nullptr;
        Mono::Bridge::Get().InvokeMethod(
            gameState.methodCameraPositionOverride,
            reinterpret_cast<void*>(camPos),
            overrideParams,
            &exc
        );

        if (avatarVisuals && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatarVisuals)) && gameState.methodShowSelfOverride) {
            void* visParams[1];
            float visTime = 0.25f;
            visParams[0] = &visTime;
            Mono::Bridge::Get().InvokeMethod(
                gameState.methodShowSelfOverride,
                reinterpret_cast<void*>(avatarVisuals),
                visParams,
                &exc
            );
        }

        m_WasActive = true;
    } else {
        if (m_WasActive) {
            Unity::Vector3 zeroPos{};
            float zeroDuration = 0.0f;
            void* overrideParams[2];
            overrideParams[0] = &zeroPos;
            overrideParams[1] = &zeroDuration;

            MonoObject* exc = nullptr;
            Mono::Bridge::Get().InvokeMethod(
                gameState.methodCameraPositionOverride,
                reinterpret_cast<void*>(camPos),
                overrideParams,
                &exc
            );

            if (avatarVisuals && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatarVisuals)) && gameState.methodShowSelfOverride) {
                float zeroTime = 0.0f;
                void* visParams[1];
                visParams[0] = &zeroTime;
                Mono::Bridge::Get().InvokeMethod(
                    gameState.methodShowSelfOverride,
                    reinterpret_cast<void*>(avatarVisuals),
                    visParams,
                    &exc
                );
            }

            m_WasActive = false;
        }
    }
}

}
