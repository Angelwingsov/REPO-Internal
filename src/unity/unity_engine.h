#pragma once
#include <windows.h>
#include <string>
#include "unity_math.h"
#include "mono/mono_bridge.h"

namespace Unity {

class Engine {
public:
    static Engine& Get();

    bool Initialize();
    bool IsReady() const { return m_Initialized; }

    static bool IsValidMonoObject(void* obj);
    static bool IsValidUnityObject(void* obj);

    void SetCursorVisible(bool visible);
    bool GetCursorVisible();
    void SetCursorLockState(int lockState);
    int  GetCursorLockState();
    void UpdateCursorState(bool menuVisible);

    void SetAmbientLight(const Color& color);
    Color GetAmbientLight();
    void SetAmbientIntensity(float intensity);
    float GetAmbientIntensity();
    void SetFog(bool enabled);
    bool GetFog();
    void SetFogDensity(float density);

    void SetLightIntensity(MonoObject* light, float intensity);
    float GetLightIntensity(MonoObject* light);
    void SetLightRange(MonoObject* light, float range);
    float GetLightRange(MonoObject* light);
    void SetLightSpotAngle(MonoObject* light, float spotAngle);
    float GetLightSpotAngle(MonoObject* light);
    void SetLightEnabled(MonoObject* light, bool enabled);
    bool GetLightEnabled(MonoObject* light);

    Vector3 GetTransformPosition(MonoObject* transformObj);
    void SetTransformPosition(MonoObject* transformObj, const Vector3& pos);
    Vector3 GetTransformForward(MonoObject* transformObj);
    Vector3 GetTransformRight(MonoObject* transformObj);
    Vector3 GetTransformUp(MonoObject* transformObj);
    MonoObject* GetTransform(MonoObject* componentObj);
    Vector3 GetComponentPosition(MonoObject* componentObj);
    std::string GetObjectName(MonoObject* obj);

    void SetRigidbodyPosition(MonoObject* rbObj, const Vector3& pos);
    void SetRigidbodyVelocity(MonoObject* rbObj, const Vector3& vel);
    void SetRigidbodyIsKinematic(MonoObject* rbObj, bool kinematic);
    bool GetRigidbodyIsKinematic(MonoObject* rbObj);
    void SetRigidbodyDetectCollisions(MonoObject* rbObj, bool detect);

    void SetColliderEnabled(MonoObject* colObj, bool enabled);
    bool GetColliderEnabled(MonoObject* colObj);
    void SetColliderIsTrigger(MonoObject* colObj, bool isTrigger);
    bool GetColliderIsTrigger(MonoObject* colObj);

    bool Linecast(const Vector3& start, const Vector3& end, Vector3& outHitPoint);

    MonoObject* FindActiveCamera(const char** outSourceName = nullptr);
    Vector3 GetPlayerOrCameraPosition(MonoObject* camera);

    void UpdateFrameCameraMetrics(MonoObject* camera, float fallbackWidth, float fallbackHeight);

    bool WorldToScreen(MonoObject* camera, const Vector3& world, Vector3& screen, float screenWidth, float screenHeight);

private:
    Engine() = default;

    int GetCameraPixelWidth(MonoObject* cam);
    int GetCameraPixelHeight(MonoObject* cam);

    bool m_Initialized = false;

    MonoClass* m_klassCamera = nullptr;
    MonoClass* m_klassComponent = nullptr;
    MonoClass* m_klassTransform = nullptr;
    MonoClass* m_klassObject = nullptr;
    MonoClass* m_klassCursor = nullptr;
    MonoClass* m_klassRenderSettings = nullptr;
    MonoClass* m_klassLight = nullptr;
    MonoClass* m_klassBehaviour = nullptr;
    MonoClass* m_klassPhysics = nullptr;
    MonoClass* m_klassRigidbody = nullptr;
    MonoClass* m_klassCollider = nullptr;

    MonoMethod* m_methodGetMainCamera = nullptr;
    MonoMethod* m_methodWorldToScreen = nullptr;
    MonoMethod* m_methodGetPixelWidth = nullptr;
    MonoMethod* m_methodGetPixelHeight = nullptr;
    MonoMethod* m_methodGetTransform = nullptr;
    MonoMethod* m_methodGetPosition = nullptr;
    MonoMethod* m_methodSetPosition = nullptr;
    MonoMethod* m_methodGetForward = nullptr;
    MonoMethod* m_methodGetRight = nullptr;
    MonoMethod* m_methodGetUp = nullptr;
    MonoMethod* m_methodLinecast = nullptr;
    MonoMethod* m_methodGetName = nullptr;
    MonoMethod* m_methodSetCursorVisible = nullptr;
    MonoMethod* m_methodGetCursorVisible = nullptr;
    MonoMethod* m_methodSetCursorLockState = nullptr;
    MonoMethod* m_methodGetCursorLockState = nullptr;

    MonoMethod* m_methodRigidbodySetPosition = nullptr;
    MonoMethod* m_methodRigidbodySetVelocity = nullptr;
    MonoMethod* m_methodRigidbodySetIsKinematic = nullptr;
    MonoMethod* m_methodRigidbodyGetIsKinematic = nullptr;
    MonoMethod* m_methodRigidbodySetDetectCollisions = nullptr;
    MonoMethod* m_methodColliderSetEnabled = nullptr;
    MonoMethod* m_methodColliderGetEnabled = nullptr;
    MonoMethod* m_methodColliderSetIsTrigger = nullptr;
    MonoMethod* m_methodColliderGetIsTrigger = nullptr;

    MonoMethod* m_methodSetAmbientLight = nullptr;
    MonoMethod* m_methodGetAmbientLight = nullptr;
    MonoMethod* m_methodSetAmbientIntensity = nullptr;
    MonoMethod* m_methodGetAmbientIntensity = nullptr;
    MonoMethod* m_methodSetFog = nullptr;
    MonoMethod* m_methodGetFog = nullptr;
    MonoMethod* m_methodSetFogDensity = nullptr;

    MonoMethod* m_methodLightSetIntensity = nullptr;
    MonoMethod* m_methodLightGetIntensity = nullptr;
    MonoMethod* m_methodLightSetRange = nullptr;
    MonoMethod* m_methodLightGetRange = nullptr;
    MonoMethod* m_methodLightSetSpotAngle = nullptr;
    MonoMethod* m_methodLightGetSpotAngle = nullptr;

    MonoMethod* m_methodBehaviourSetEnabled = nullptr;
    MonoMethod* m_methodBehaviourGetEnabled = nullptr;

    bool m_MenuWasVisible = false;
    int  m_SavedLockState = 1;
    bool m_SavedCursorVisible = false;

    MonoObject* m_LastCachedCamera = nullptr;
    int m_CachedCameraWidth = 0;
    int m_CachedCameraHeight = 0;
};

}
