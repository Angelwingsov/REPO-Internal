#include "animbreaker.h"
#include "core/config.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include <algorithm>
#include <cmath>

namespace Features {

AnimBreaker& AnimBreaker::Get() {
    static AnimBreaker instance;
    return instance;
}

void AnimBreaker::Reset() {
    m_PoseTimer = 0.0f;
    m_PoseIndex = 0;
    m_HeadAngle = 0.0f;
}

void AnimBreaker::Update(float dt) {
    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        Reset();
        return;
    }

    auto& cfg = Core::Config::Get();
    if (!cfg.animBreaker.enabled) return;

    if (!gameState.klassPlayerController) return;

    uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(
        Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance")
    );
    if (!playerCtrl || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) return;

    uintptr_t avatar = 0;
    if (!Memory::ReadPtr(playerCtrl + gameState.offsets.PlayerController_playerAvatarScript, avatar) ||
        !avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) return;

    uint8_t isDead = 0;
    if (Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_deadSet, isDead) && isDead != 0) return;

    if (cfg.animBreaker.slideWalk) {
        if (gameState.offsets.PlayerAvatar_isSliding > 0) {
            Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isSliding, 1);
        }
    }

    if (cfg.animBreaker.forceCrawl) {
        if (gameState.offsets.PlayerAvatar_isCrawling > 0) {
            Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrawling, 1);
        }
    }

    if (cfg.animBreaker.staticLegs) {
        if (gameState.offsets.PlayerAvatar_isMoving > 0) {
            Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isMoving, 0);
        }
        if (gameState.offsets.PlayerAvatar_isSprinting > 0) {
            Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isSprinting, 0);
        }
    }

    if (cfg.animBreaker.poseSpammer) {
        float speed = (std::clamp)(cfg.animBreaker.poseSpamSpeed, 1.0f, 60.0f);
        float interval = 1.0f / speed;
        m_PoseTimer += dt;
        if (m_PoseTimer >= interval) {
            m_PoseTimer = 0.0f;
            m_PoseIndex = (m_PoseIndex + 1) % 4;
        }

        if (gameState.offsets.PlayerAvatar_isCrouching > 0 &&
            gameState.offsets.PlayerAvatar_isCrawling > 0 &&
            gameState.offsets.PlayerAvatar_isSliding > 0) {
            switch (m_PoseIndex) {
            case 0:
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrouching, 0);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrawling, 0);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isSliding, 0);
                break;
            case 1:
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrouching, 1);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrawling, 0);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isSliding, 0);
                break;
            case 2:
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrouching, 1);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrawling, 1);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isSliding, 0);
                break;
            case 3:
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrouching, 1);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isCrawling, 0);
                Memory::WriteByte(avatar + gameState.offsets.PlayerAvatar_isSliding, 1);
                break;
            }
        }
    }

    uintptr_t visuals = 0;
    if (Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerAvatarVisuals, visuals) && visuals &&
        Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(visuals))) {

        if (cfg.animBreaker.spineBreaker) {
            if (gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideDegrees > 0 &&
                gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideTimer > 0) {
                Memory::WriteFloat(visuals + gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideDegrees, cfg.animBreaker.spineTwitchAmount);
                Memory::WriteFloat(visuals + gameState.offsets.PlayerAvatarVisuals_leanTwitchOverrideTimer, 0.25f);
            }
        }

        if (cfg.animBreaker.headBreaker) {
            m_HeadAngle += cfg.animBreaker.headSpinSpeed * dt;
            if (m_HeadAngle >= 360.0f) m_HeadAngle = std::fmod(m_HeadAngle, 360.0f);

            uintptr_t headSideTransform = 0;
            if (gameState.offsets.PlayerAvatarVisuals_headSideTransform > 0 &&
                Memory::ReadPtr(visuals + gameState.offsets.PlayerAvatarVisuals_headSideTransform, headSideTransform) &&
                headSideTransform && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(headSideTransform))) {
                Unity::Engine::Get().SetTransformLocalEulerAngles(
                    reinterpret_cast<MonoObject*>(headSideTransform),
                    Unity::Vector3(0.0f, m_HeadAngle, 0.0f)
                );
            }
        }
    }
}

}
