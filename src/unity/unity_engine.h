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
    Quaternion GetTransformRotation(MonoObject* transformObj);
    void SetTransformRotation(MonoObject* transformObj, const Quaternion& rot);
    Vector3 GetTransformForward(MonoObject* transformObj);
    Vector3 GetTransformRight(MonoObject* transformObj);
    Vector3 GetTransformUp(MonoObject* transformObj);
    MonoObject* GetTransform(MonoObject* componentObj);
    void SetTransformLocalRotation(MonoObject* transformObj, const Quaternion& rot);
    void SetTransformLocalEulerAngles(MonoObject* transformObj, const Vector3& angles);
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
    bool TryGetColliderBounds(MonoObject* collider, Bounds& bounds);
    bool TryGetVisualBounds(MonoObject* meshParent, Bounds& bounds, bool& hasSkinned);

    bool Linecast(const Vector3& start, const Vector3& end, Vector3& outHitPoint);
    bool CheckLineOfSight(const Vector3& start, const Vector3& end, int layerMask = (1 << 0));
    Vector3 GetCameraPosition(MonoObject* camera);

    MonoObject* FindShader(const char* name);
    MonoObject* CreateMaterial(MonoObject* shader);
    void SetMaterialInt(MonoObject* material, const char* name, int value);
    void SetMaterialColor(MonoObject* material, const char* name, const Color& color);
    void SetHideFlags(MonoObject* obj, int flags);
    MonoObject* GetRendererSharedMaterial(MonoObject* renderer);
    void SetRendererSharedMaterial(MonoObject* renderer, MonoObject* material);
    MonoObject* GetRendererSharedMaterials(MonoObject* renderer);
    void SetRendererSharedMaterials(MonoObject* renderer, MonoObject* materialsArray);
    bool GetRenderers(MonoObject* parent, std::vector<MonoObject*>& outRenderers);

    MonoObject* FindActiveCamera(const char** outSourceName = nullptr);
    Vector3 GetPlayerOrCameraPosition(MonoObject* camera);

    void UpdateFrameCameraMetrics(MonoObject* camera, float fallbackWidth, float fallbackHeight);

    bool WorldToScreen(MonoObject* camera, const Vector3& world, Vector3& screen, float screenWidth, float screenHeight);

private:
    Engine() = default;

    int GetCameraPixelWidth(MonoObject* cam);
    int GetCameraPixelHeight(MonoObject* cam);

    bool m_Initialized = false;

    MonoClass* m_klassMeshRenderer = nullptr;
    MonoClass* m_klassSkinnedMeshRenderer = nullptr;
    MonoMethod* m_methodGetComponentsInternal = nullptr;
    MonoMethod* m_methodRendererGetBounds = nullptr;
    MonoMethod* m_methodRendererGetEnabled = nullptr;
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
    MonoClass* m_klassShader = nullptr;
    MonoClass* m_klassMaterial = nullptr;
    MonoClass* m_klassRenderer = nullptr;

    MonoMethod* m_methodGetMainCamera = nullptr;
    MonoMethod* m_methodWorldToScreen = nullptr;
    MonoMethod* m_methodGetPixelWidth = nullptr;
    MonoMethod* m_methodGetPixelHeight = nullptr;
    MonoMethod* m_methodGetTransform = nullptr;
    MonoMethod* m_methodGetPosition = nullptr;
    MonoMethod* m_methodSetPosition = nullptr;
    MonoMethod* m_methodGetRotation = nullptr;
    MonoMethod* m_methodSetRotation = nullptr;
    MonoMethod* m_methodSetLocalEulerAngles = nullptr;
    MonoMethod* m_methodSetLocalRotation = nullptr;
    MonoMethod* m_methodGetForward = nullptr;
    MonoMethod* m_methodGetRight = nullptr;
    MonoMethod* m_methodGetUp = nullptr;
    MonoMethod* m_methodLinecast = nullptr;
    MonoMethod* m_methodLinecast4 = nullptr;
    MonoMethod* m_methodLinecast2 = nullptr;
    MonoMethod* m_methodSetHideFlags = nullptr;
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
    MonoMethod* m_methodColliderGetBounds = nullptr;
    MonoMethod* m_methodShaderFind = nullptr;
    MonoMethod* m_methodMaterialCtor = nullptr;
    MonoMethod* m_methodMaterialSetInt = nullptr;
    MonoMethod* m_methodMaterialSetColor = nullptr;
    MonoMethod* m_methodRendererGetSharedMaterial = nullptr;
    MonoMethod* m_methodRendererSetSharedMaterial = nullptr;
    MonoMethod* m_methodRendererGetSharedMaterials = nullptr;
    MonoMethod* m_methodRendererSetSharedMaterials = nullptr;

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
