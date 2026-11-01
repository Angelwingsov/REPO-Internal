#pragma once
#include <windows.h>
#include <atomic>
#include "unity/unity_math.h"
#include "mono/mono_bridge.h"

namespace Features {

class AntiAim {
public:
    static AntiAim& Get();

    bool Initialize();
    bool RestoreBeforeUnload();

    void Update(float dt);
    void Reset();

    bool IsActive() const { return m_Active; }
    Unity::Quaternion GetCurrentRotation() const { return m_CurrentRotation; }
    float GetCurrentPitch() const { return m_CurrentPitch; }
    float GetCurrentYaw() const { return m_CurrentYaw; }
    float GetCurrentRoll() const { return m_CurrentRoll; }

    bool IsLocalAvatar(void* avatarPtr) const;
    bool IsLocalCamera(void* cameraPtr) const;
    MonoObject* GetCameraAimInstance() const;

private:
    AntiAim() = default;

    static void __fastcall HookOnPhotonSerializeView(void* this_ptr, void* stream, void* info);
    using OnPhotonSerializeViewFn = void(__fastcall*)(void*, void*, void*);
    static inline OnPhotonSerializeViewFn s_OriginalOnPhotonSerializeView = nullptr;
    static inline void* s_HookTargetAvatar = nullptr;
    static inline std::atomic<bool> s_HookAvatarInstalled{false};

    static void __fastcall HookCameraOnPhotonSerializeView(void* this_ptr, void* stream, void* info);
    static inline OnPhotonSerializeViewFn s_OriginalCameraOnPhotonSerializeView = nullptr;
    static inline void* s_HookTargetCamera = nullptr;
    static inline std::atomic<bool> s_HookCameraInstalled{false};

    bool m_PrevKeyDown = false;
    bool m_ActiveState = false;
    bool m_WasActive = false;
    bool m_Active = false;

    float m_SpinYaw = 0.0f;
    float m_PitchSpin = 0.0f;
    float m_JitterTimer = 0.0f;
    bool  m_JitterFlip = false;

    float m_CurrentPitch = 0.0f;
    float m_CurrentYaw = 0.0f;
    float m_CurrentRoll = 0.0f;

    Unity::Quaternion m_CurrentRotation{0.0f, 0.0f, 0.0f, 1.0f};
};

}
