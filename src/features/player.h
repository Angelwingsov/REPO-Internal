#pragma once
#include <windows.h>
#include <cstdint>

namespace Features {

class PlayerMods {
public:
    static PlayerMods& Get();

    void Initialize();
    void Update();

    static const char* GetKeyName(int vkCode);

private:
    PlayerMods() = default;

    bool m_PrevSpeedKeyDown = false;
    bool m_SpeedActiveState = false;

    uintptr_t m_LastPhysGrabber = 0;
    uintptr_t m_LastPlayerCtrl = 0;

    bool m_SpeedSaved = false;
    bool m_SpeedWasActive = false;
    float m_OriginalMoveSpeed = 0.5f;
    float m_OriginalSprintSpeed = 1.0f;
    float m_OriginalCrouchSpeed = 1.0f;

    bool m_GrabRangeSaved = false;
    float m_OriginalGrabRange = 4.0f;
    float m_OriginalGrabReleaseDistance = 8.0f;
    float m_OriginalMaxDistanceFromPlayer = 2.5f;

    bool m_GrabThroughWallsSaved = false;
    int32_t m_OriginalPhysGrabberMask = 0;

    bool m_StrengthSaved = false;
    float m_OriginalGrabStrength = 1.0f;
    float m_OriginalThrowStrength = 0.0f;
    float m_OriginalForceMax = 4.0f;
    float m_OriginalClimbForceMultiplier = 1.0f;
    uintptr_t m_LastGrabbedObject = 0;

    bool m_JumpForceSaved = false;
    float m_OriginalJumpForce = 8.0f;

    uintptr_t m_LastVoiceChat = 0;
    bool m_GlobalVoiceApplied = false;
    uint64_t m_LastVoiceApplyMs = 0;
};

}
