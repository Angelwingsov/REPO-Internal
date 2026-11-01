#include "unity_engine.h"
#include "game/player_bounds.h"
#include "core/logger.h"
#include "game/state.h"

namespace Unity {

Engine& Engine::Get() {
    static Engine instance;
    return instance;
}

bool Engine::IsValidMonoObject(void* obj) {
    if (!obj || !Memory::IsValidPtr(obj)) return false;
    uintptr_t vtable = 0;
    if (!Memory::ReadPtr(reinterpret_cast<uintptr_t>(obj), vtable)) return false;
    return Memory::IsValidAddress(vtable);
}

bool Engine::IsValidUnityObject(void* obj) {
    if (!IsValidMonoObject(obj)) return false;

    constexpr size_t cachedPtrOffset = sizeof(void*) * 2;
    uintptr_t cachedNativePtr = 0;
    return Memory::ReadPtr(reinterpret_cast<uintptr_t>(obj) + cachedPtrOffset, cachedNativePtr) &&
        Memory::IsValidAddress(cachedNativePtr);
}

bool Engine::Initialize() {
    auto& bridge = Mono::Bridge::Get();
    MonoImage* coreImage = bridge.GetCoreImage();
    if (!coreImage) return false;

    LOG_INFO("Resolving UnityEngine classes and methods...");

    m_klassCamera = bridge.FindClass(coreImage, "UnityEngine", "Camera");
    if (m_klassCamera) {
        m_methodGetMainCamera = bridge.FindMethod(m_klassCamera, "get_main", 0);
        m_methodWorldToScreen = bridge.FindMethod(m_klassCamera, "WorldToScreenPoint", 1);
        m_methodGetPixelWidth = bridge.FindMethod(m_klassCamera, "get_pixelWidth", 0);
        m_methodGetPixelHeight = bridge.FindMethod(m_klassCamera, "get_pixelHeight", 0);
        LOG_OK("UnityEngine.Camera resolved (W2S: %p)", m_methodWorldToScreen);
    }

    m_klassMeshRenderer = bridge.FindClass(coreImage, "UnityEngine", "MeshRenderer");
    m_klassSkinnedMeshRenderer = bridge.FindClass(coreImage, "UnityEngine", "SkinnedMeshRenderer");
    if (auto* renderer = bridge.FindClass(coreImage, "UnityEngine", "Renderer")) {
        m_methodRendererGetBounds = bridge.FindMethod(renderer, "get_bounds", 0);
        m_methodRendererGetEnabled = bridge.FindMethod(renderer, "get_enabled", 0);
    }
    if (auto* gameObject = bridge.FindClass(coreImage, "UnityEngine", "GameObject")) {

        m_methodGetComponentsInternal = bridge.FindMethod(gameObject, "GetComponentsInternal", 6);
    }

    m_klassComponent = bridge.FindClass(coreImage, "UnityEngine", "Component");
    if (m_klassComponent) {
        m_methodGetTransform = bridge.FindMethod(m_klassComponent, "get_transform", 0);
        LOG_OK("UnityEngine.Component resolved (get_transform: %p)", m_methodGetTransform);
    }

    m_klassTransform = bridge.FindClass(coreImage, "UnityEngine", "Transform");
    if (m_klassTransform) {
        m_methodGetPosition = bridge.FindMethod(m_klassTransform, "get_position", 0);
        m_methodSetPosition = bridge.FindMethod(m_klassTransform, "set_position", 1);
        m_methodGetForward = bridge.FindMethod(m_klassTransform, "get_forward", 0);
        m_methodGetRight = bridge.FindMethod(m_klassTransform, "get_right", 0);
        m_methodGetUp = bridge.FindMethod(m_klassTransform, "get_up", 0);
        LOG_OK("UnityEngine.Transform resolved (get_position: %p, set_position: %p, get_forward: %p)",
               m_methodGetPosition, m_methodSetPosition, m_methodGetForward);
    }

    m_klassObject = bridge.FindClass(coreImage, "UnityEngine", "Object");
    if (m_klassObject) {
        m_methodGetName = bridge.FindMethod(m_klassObject, "get_name", 0);
        LOG_OK("UnityEngine.Object resolved (get_name: %p)", m_methodGetName);
    }

    m_klassCursor = bridge.FindClass(coreImage, "UnityEngine", "Cursor");
    if (!m_klassCursor && bridge.GetAppImage()) {
        m_klassCursor = bridge.FindClass(bridge.GetAppImage(), "UnityEngine", "Cursor");
    }
    if (m_klassCursor) {
        m_methodSetCursorVisible = bridge.FindMethod(m_klassCursor, "set_visible", 1);
        m_methodGetCursorVisible = bridge.FindMethod(m_klassCursor, "get_visible", 0);
        m_methodSetCursorLockState = bridge.FindMethod(m_klassCursor, "set_lockState", 1);
        m_methodGetCursorLockState = bridge.FindMethod(m_klassCursor, "get_lockState", 0);
        LOG_OK("UnityEngine.Cursor resolved (set_visible: %p, set_lockState: %p)",
               m_methodSetCursorVisible, m_methodSetCursorLockState);
    } else {
        LOG_WARN("UnityEngine.Cursor class not found!");
    }

    m_klassRenderSettings = bridge.FindClass(coreImage, "UnityEngine", "RenderSettings");
    if (m_klassRenderSettings) {
        m_methodSetAmbientLight = bridge.FindMethod(m_klassRenderSettings, "set_ambientLight", 1);
        m_methodGetAmbientLight = bridge.FindMethod(m_klassRenderSettings, "get_ambientLight", 0);
        m_methodSetAmbientIntensity = bridge.FindMethod(m_klassRenderSettings, "set_ambientIntensity", 1);
        m_methodGetAmbientIntensity = bridge.FindMethod(m_klassRenderSettings, "get_ambientIntensity", 0);
        m_methodSetFog = bridge.FindMethod(m_klassRenderSettings, "set_fog", 1);
        m_methodGetFog = bridge.FindMethod(m_klassRenderSettings, "get_fog", 0);
        m_methodSetFogDensity = bridge.FindMethod(m_klassRenderSettings, "set_fogDensity", 1);
        LOG_OK("UnityEngine.RenderSettings resolved (set_ambientLight: %p)", m_methodSetAmbientLight);
    }

    m_klassLight = bridge.FindClass(coreImage, "UnityEngine", "Light");
    if (m_klassLight) {
        m_methodLightSetIntensity = bridge.FindMethod(m_klassLight, "set_intensity", 1);
        m_methodLightGetIntensity = bridge.FindMethod(m_klassLight, "get_intensity", 0);
        m_methodLightSetRange = bridge.FindMethod(m_klassLight, "set_range", 1);
        m_methodLightGetRange = bridge.FindMethod(m_klassLight, "get_range", 0);
        m_methodLightSetSpotAngle = bridge.FindMethod(m_klassLight, "set_spotAngle", 1);
        m_methodLightGetSpotAngle = bridge.FindMethod(m_klassLight, "get_spotAngle", 0);
        LOG_OK("UnityEngine.Light resolved (set_intensity: %p)", m_methodLightSetIntensity);
    }

    m_klassBehaviour = bridge.FindClass(coreImage, "UnityEngine", "Behaviour");
    if (m_klassBehaviour) {
        m_methodBehaviourSetEnabled = bridge.FindMethod(m_klassBehaviour, "set_enabled", 1);
        m_methodBehaviourGetEnabled = bridge.FindMethod(m_klassBehaviour, "get_enabled", 0);
        LOG_OK("UnityEngine.Behaviour resolved (set_enabled: %p)", m_methodBehaviourSetEnabled);
    }

    MonoImage* physImage = bridge.GetPhysicsImage();
    if (physImage) {
        m_klassPhysics = bridge.FindClass(physImage, "UnityEngine", "Physics");
        if (m_klassPhysics) {

        m_methodLinecast = bridge.FindMethod(m_klassPhysics, "Linecast", 5);
            if (!m_methodLinecast) {
                m_methodLinecast = bridge.FindMethod(m_klassPhysics, "Linecast", 2);
            }
            LOG_OK("UnityEngine.Physics resolved (Linecast: %p)", m_methodLinecast);
        }

        m_klassRigidbody = bridge.FindClass(physImage, "UnityEngine", "Rigidbody");
        if (m_klassRigidbody) {
            m_methodRigidbodySetPosition = bridge.FindMethod(m_klassRigidbody, "set_position", 1);
            m_methodRigidbodySetVelocity = bridge.FindMethod(m_klassRigidbody, "set_velocity", 1);
            if (!m_methodRigidbodySetVelocity) {
                m_methodRigidbodySetVelocity = bridge.FindMethod(m_klassRigidbody, "set_linearVelocity", 1);
            }
            m_methodRigidbodySetIsKinematic = bridge.FindMethod(m_klassRigidbody, "set_isKinematic", 1);
            m_methodRigidbodyGetIsKinematic = bridge.FindMethod(m_klassRigidbody, "get_isKinematic", 0);
            m_methodRigidbodySetDetectCollisions = bridge.FindMethod(m_klassRigidbody, "set_detectCollisions", 1);
            LOG_OK("UnityEngine.Rigidbody resolved (set_position: %p, set_isKinematic: %p, get_isKinematic: %p)",
                   m_methodRigidbodySetPosition, m_methodRigidbodySetIsKinematic, m_methodRigidbodyGetIsKinematic);
        }

        m_klassCollider = bridge.FindClass(physImage, "UnityEngine", "Collider");
        if (m_klassCollider) {
            m_methodColliderSetEnabled = bridge.FindMethod(m_klassCollider, "set_enabled", 1);
            m_methodColliderGetEnabled = bridge.FindMethod(m_klassCollider, "get_enabled", 0);
            m_methodColliderGetBounds = bridge.FindMethod(m_klassCollider, "get_bounds", 0);
            m_methodColliderSetIsTrigger = bridge.FindMethod(m_klassCollider, "set_isTrigger", 1);
            m_methodColliderGetIsTrigger = bridge.FindMethod(m_klassCollider, "get_isTrigger", 0);
            LOG_OK("UnityEngine.Collider resolved (set_enabled: %p, set_isTrigger: %p)",
                   m_methodColliderSetEnabled, m_methodColliderSetIsTrigger);
        }
    }

    m_Initialized = (m_klassCamera != nullptr && m_methodWorldToScreen != nullptr && m_klassTransform != nullptr);
    return m_Initialized;
}

int Engine::GetCameraPixelWidth(MonoObject* cam) {
    if (!cam || !m_methodGetPixelWidth) return 0;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetPixelWidth, cam, nullptr, &exc);
    if (!res || exc) return 0;

    int width = 0;
    if (Mono::Bridge::Get().Unbox(res, width)) {
        return width;
    }
    return 0;
}

int Engine::GetCameraPixelHeight(MonoObject* cam) {
    if (!cam || !m_methodGetPixelHeight) return 0;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetPixelHeight, cam, nullptr, &exc);
    if (!res || exc) return 0;

    int height = 0;
    if (Mono::Bridge::Get().Unbox(res, height)) {
        return height;
    }
    return 0;
}

void Engine::UpdateFrameCameraMetrics(MonoObject* camera, float fallbackWidth, float fallbackHeight) {
    if (!camera || !IsValidUnityObject(camera)) {
        m_LastCachedCamera = nullptr;
        m_CachedCameraWidth = static_cast<int>(fallbackWidth);
        m_CachedCameraHeight = static_cast<int>(fallbackHeight);
        return;
    }

    m_LastCachedCamera = camera;
    int w = GetCameraPixelWidth(camera);
    int h = GetCameraPixelHeight(camera);

    m_CachedCameraWidth = (w > 0) ? w : static_cast<int>(fallbackWidth);
    m_CachedCameraHeight = (h > 0) ? h : static_cast<int>(fallbackHeight);
}

Vector3 Engine::GetTransformPosition(MonoObject* transformObj) {
    if (!transformObj || !m_methodGetPosition || !IsValidUnityObject(transformObj)) {
        return Vector3{};
    }

    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetPosition, transformObj, nullptr, &exc);
    if (!res || exc) return Vector3{};

    Vector3 pos;
    if (Mono::Bridge::Get().Unbox(res, pos) && pos.IsValid()) {
        return pos;
    }
    return Vector3{};
}

void Engine::SetTransformPosition(MonoObject* transformObj, const Vector3& pos) {
    if (!transformObj || !m_methodSetPosition || !IsValidUnityObject(transformObj)) {
        return;
    }
    Vector3 p = pos;
    void* args[1] = { &p };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodSetPosition, transformObj, args, &exc);
}

void Engine::SetRigidbodyPosition(MonoObject* rbObj, const Vector3& pos) {
    if (!rbObj || !m_methodRigidbodySetPosition || !IsValidUnityObject(rbObj)) {
        return;
    }
    Vector3 p = pos;
    void* args[1] = { &p };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodRigidbodySetPosition, rbObj, args, &exc);
}

void Engine::SetRigidbodyVelocity(MonoObject* rbObj, const Vector3& vel) {
    if (!rbObj || !m_methodRigidbodySetVelocity || !IsValidUnityObject(rbObj)) {
        return;
    }
    if (GetRigidbodyIsKinematic(rbObj)) {
        return;
    }
    Vector3 v = vel;
    void* args[1] = { &v };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodRigidbodySetVelocity, rbObj, args, &exc);
}

void Engine::SetRigidbodyIsKinematic(MonoObject* rbObj, bool kinematic) {
    if (!rbObj || !m_methodRigidbodySetIsKinematic || !IsValidUnityObject(rbObj)) {
        return;
    }
    int val = kinematic ? 1 : 0;
    void* args[1] = { &val };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodRigidbodySetIsKinematic, rbObj, args, &exc);
}

bool Engine::GetRigidbodyIsKinematic(MonoObject* rbObj) {
    if (!rbObj || !m_methodRigidbodyGetIsKinematic || !IsValidUnityObject(rbObj)) {
        return false;
    }
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodRigidbodyGetIsKinematic, rbObj, nullptr, &exc);
    if (!res || exc) return false;
    bool val = false;
    if (Mono::Bridge::Get().Unbox(res, val)) {
        return val;
    }
    return false;
}

void Engine::SetRigidbodyDetectCollisions(MonoObject* rbObj, bool detect) {
    if (!rbObj || !m_methodRigidbodySetDetectCollisions || !IsValidUnityObject(rbObj)) {
        return;
    }
    int val = detect ? 1 : 0;
    void* args[1] = { &val };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodRigidbodySetDetectCollisions, rbObj, args, &exc);
}

void Engine::SetColliderEnabled(MonoObject* colObj, bool enabled) {
    if (!colObj || !m_methodColliderSetEnabled || !IsValidUnityObject(colObj)) {
        return;
    }
    int val = enabled ? 1 : 0;
    void* args[1] = { &val };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodColliderSetEnabled, colObj, args, &exc);
}

bool Engine::TryGetVisualBounds(MonoObject* meshParent, Bounds& bounds, bool& hasSkinned) {
    hasSkinned=false;
    if (!IsValidUnityObject(meshParent) || !m_klassMeshRenderer || !m_methodGetComponentsInternal ||
        !m_methodRendererGetBounds || !m_methodRendererGetEnabled) return false;
    auto& bridge = Mono::Bridge::Get();
    Game::PlayerBoundsAccumulator accumulator;
    for (auto* rendererClass : {m_klassMeshRenderer,m_klassSkinnedMeshRenderer}) {
        if (!rendererClass) continue;
        MonoObject* type = bridge.GetReflectionType(rendererClass);
        Mono::PinnedObject typePin(type);
        if (!typePin) return false;
        int typedArray=1, recursive=1, inactive=0, reverse=0;
        void* args[] = {type, &typedArray, &recursive, &inactive, &reverse, nullptr};
        MonoObject* exception = nullptr;
        MonoObject* result = bridge.InvokeMethod(m_methodGetComponentsInternal, meshParent, args, &exception);
        if (!result || exception) return false;
        Mono::PinnedObject arrayPin(result);
        if (!arrayPin) return false;
        uintptr_t length=0;
        uintptr_t array=reinterpret_cast<uintptr_t>(result);
        if (!Memory::ReadPtr(array+sizeof(void*)*3,length) || length>128) return false;
        for (uintptr_t i=0;i<length;++i) {
            uintptr_t reference=0;
            if (!Memory::ReadPtr(array+sizeof(void*)*4+i*sizeof(void*),reference)) return false;
            auto* renderer=reinterpret_cast<MonoObject*>(reference);
            if (!IsValidUnityObject(renderer)) continue;
            exception=nullptr;
            auto* enabledValue=bridge.InvokeMethod(m_methodRendererGetEnabled,renderer,nullptr,&exception);
            bool enabled=false;
            if (!enabledValue || exception || !bridge.Unbox(enabledValue,enabled) || !enabled) continue;
            exception=nullptr;
            auto* value=bridge.InvokeMethod(m_methodRendererGetBounds,renderer,nullptr,&exception);
            Bounds part;
            if (value && !exception && bridge.Unbox(value,part) && part.IsValid()) {
                accumulator.Add(part);
                if (rendererClass==m_klassSkinnedMeshRenderer) hasSkinned=true;
            }
        }
    }
    return accumulator.Get(bounds);
}

bool Engine::TryGetColliderBounds(MonoObject* collider, Bounds& bounds) {
    if (!m_methodColliderGetBounds || !IsValidUnityObject(collider)) return false;
    MonoObject* exception = nullptr;
    MonoObject* result = Mono::Bridge::Get().InvokeMethod(m_methodColliderGetBounds, collider, nullptr, &exception);
    Bounds value;
    if (!result || exception || !Mono::Bridge::Get().Unbox(result, value) || !value.IsValid()) return false;
    bounds = value;
    return true;
}

bool Engine::GetColliderEnabled(MonoObject* colObj) {
    if (!colObj || !m_methodColliderGetEnabled || !IsValidUnityObject(colObj)) {
        return false;
    }
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodColliderGetEnabled, colObj, nullptr, &exc);
    if (!res || exc) return false;
    bool enabled = false;
    if (Mono::Bridge::Get().Unbox(res, enabled)) {
        return enabled;
    }
    return false;
}

void Engine::SetColliderIsTrigger(MonoObject* colObj, bool isTrigger) {
    if (!colObj || !m_methodColliderSetIsTrigger || !IsValidUnityObject(colObj)) {
        return;
    }
    int val = isTrigger ? 1 : 0;
    void* args[1] = { &val };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodColliderSetIsTrigger, colObj, args, &exc);
}

bool Engine::GetColliderIsTrigger(MonoObject* colObj) {
    if (!colObj || !m_methodColliderGetIsTrigger || !IsValidUnityObject(colObj)) {
        return false;
    }
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodColliderGetIsTrigger, colObj, nullptr, &exc);
    if (!res || exc) return false;
    bool val = false;
    if (Mono::Bridge::Get().Unbox(res, val)) {
        return val;
    }
    return false;
}

Vector3 Engine::GetTransformForward(MonoObject* transformObj) {
    if (!transformObj || !m_methodGetForward || !IsValidUnityObject(transformObj)) {
        return Vector3{ 0.0f, 0.0f, 1.0f };
    }

    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetForward, transformObj, nullptr, &exc);
    if (!res || exc) return Vector3{ 0.0f, 0.0f, 1.0f };

    Vector3 vec;
    if (Mono::Bridge::Get().Unbox(res, vec) && vec.IsValid()) {
        return vec;
    }
    return Vector3{ 0.0f, 0.0f, 1.0f };
}

Vector3 Engine::GetTransformRight(MonoObject* transformObj) {
    if (!transformObj || !m_methodGetRight || !IsValidUnityObject(transformObj)) {
        return Vector3{ 1.0f, 0.0f, 0.0f };
    }

    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetRight, transformObj, nullptr, &exc);
    if (!res || exc) return Vector3{ 1.0f, 0.0f, 0.0f };

    Vector3 vec;
    if (Mono::Bridge::Get().Unbox(res, vec) && vec.IsValid()) {
        return vec;
    }
    return Vector3{ 1.0f, 0.0f, 0.0f };
}

Vector3 Engine::GetTransformUp(MonoObject* transformObj) {
    if (!transformObj || !m_methodGetUp || !IsValidUnityObject(transformObj)) {
        return Vector3{ 0.0f, 1.0f, 0.0f };
    }

    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetUp, transformObj, nullptr, &exc);
    if (!res || exc) return Vector3{ 0.0f, 1.0f, 0.0f };

    Vector3 vec;
    if (Mono::Bridge::Get().Unbox(res, vec) && vec.IsValid()) {
        return vec;
    }
    return Vector3{ 0.0f, 1.0f, 0.0f };
}

MonoObject* Engine::GetTransform(MonoObject* componentObj) {
    if (!componentObj || !m_methodGetTransform || !IsValidUnityObject(componentObj)) {
        return nullptr;
    }

    MonoObject* exc = nullptr;
    MonoObject* transform = Mono::Bridge::Get().InvokeMethod(m_methodGetTransform, componentObj, nullptr, &exc);
    if (!transform || exc || !IsValidUnityObject(transform)) return nullptr;

    return transform;
}

#pragma pack(push, 1)
struct InternalRaycastHit {
    Vector3 point;
    Vector3 normal;
    uint32_t faceID;
    float distance;
    Vector2 uv;
    int collider;
    char pad[32];
};
#pragma pack(pop)

bool Engine::Linecast(const Vector3& start, const Vector3& end, Vector3& outHitPoint) {
    if (!m_methodLinecast) return false;

    Vector3 s = start;
    Vector3 e = end;
    InternalRaycastHit hit{};

    int layerMask = -1;
    int triggerInteraction = 1;
    void* params[5];
    params[0] = &s;
    params[1] = &e;
    params[2] = &hit;
    params[3] = &layerMask;
    params[4] = &triggerInteraction;

    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodLinecast, nullptr, params, &exc);
    if (!res || exc) return false;

    bool didHit = false;
    if (Mono::Bridge::Get().Unbox(res, didHit) && didHit) {
        if (hit.point.IsValid()) {
            outHitPoint = hit.point;
            return true;
        }
    }
    return false;
}

Vector3 Engine::GetComponentPosition(MonoObject* componentObj) {
    if (!componentObj || !m_methodGetTransform || !IsValidUnityObject(componentObj)) {
        return Vector3{};
    }

    MonoObject* exc = nullptr;
    MonoObject* transform = Mono::Bridge::Get().InvokeMethod(m_methodGetTransform, componentObj, nullptr, &exc);
    if (!transform || exc) return Vector3{};

    return GetTransformPosition(transform);
}

std::string Engine::GetObjectName(MonoObject* obj) {
    if (!obj || !m_methodGetName || !IsValidUnityObject(obj)) {
        return "";
    }

    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetName, obj, nullptr, &exc);
    if (!res || exc) return "";

    std::string s = Mono::Bridge::Get().ReadMonoString(reinterpret_cast<MonoString*>(res));
    if (s.empty()) return "";

    size_t clonePos = s.find("(Clone)");
    if (clonePos != std::string::npos) {
        s.erase(clonePos);
    }

    if (s.rfind("Valuable", 0) == 0 && s.length() > 8) {
        s = s.substr(8);
    }

    if (s.length() > 8 && s.compare(s.length() - 8, 8, "Valuable") == 0) {
        s.erase(s.length() - 8);
    }

    return s;
}

MonoObject* Engine::FindActiveCamera(const char** outSourceName) {
    auto& gameState = Game::GameState::Get();

    if (gameState.klassSemiFunc) {
        bool valid = Mono::Bridge::Get().GetStaticBool(gameState.klassSemiFunc, "playerCameraLocalStateValid", false);
        if (valid) {
            auto* cam = static_cast<MonoObject*>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassSemiFunc, "playerCameraLocal"));
            if (IsValidUnityObject(cam)) {
                if (outSourceName) *outSourceName = "SemiFunc.playerCameraLocal";
                return cam;
            }
        }
    }

    if (gameState.klassCameraUtils) {
        uintptr_t camUtils = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassCameraUtils, "Instance"));
        if (camUtils && IsValidUnityObject(reinterpret_cast<void*>(camUtils))) {
            uintptr_t cam = 0;
            if (Memory::ReadPtr(camUtils + gameState.offsets.CameraUtils_MainCamera, cam) && cam && IsValidUnityObject(reinterpret_cast<void*>(cam))) {
                if (outSourceName) *outSourceName = "CameraUtils.Instance.MainCamera";
                return reinterpret_cast<MonoObject*>(cam);
            }
        }
    }

    if (gameState.klassGameDirector) {
        uintptr_t gameDir = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassGameDirector, "instance"));
        if (gameDir && IsValidUnityObject(reinterpret_cast<void*>(gameDir))) {
            uintptr_t cam = 0;
            if (Memory::ReadPtr(gameDir + gameState.offsets.GameDirector_MainCamera, cam) && cam && IsValidUnityObject(reinterpret_cast<void*>(cam))) {
                if (outSourceName) *outSourceName = "GameDirector.instance.MainCamera";
                return reinterpret_cast<MonoObject*>(cam);
            }
        }
    }

    if (m_methodGetMainCamera) {
        MonoObject* exc = nullptr;
        MonoObject* cam = Mono::Bridge::Get().InvokeMethod(m_methodGetMainCamera, nullptr, nullptr, &exc);
        if (cam && !exc && IsValidUnityObject(cam)) {
            if (outSourceName) *outSourceName = "Camera.main";
            return cam;
        }
    }

    if (outSourceName) *outSourceName = "None";
    return nullptr;
}

Vector3 Engine::GetPlayerOrCameraPosition(MonoObject* camera) {
    auto& gameState = Game::GameState::Get();

    if (gameState.klassPlayerAvatar) {
        uintptr_t player = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerAvatar, "instance"));
        if (player && IsValidUnityObject(reinterpret_cast<void*>(player))) {
            Vector3 p = GetComponentPosition(reinterpret_cast<MonoObject*>(player));
            if (p.IsValid() && !p.IsZero()) {
                return p;
            }
        }
    }

    if (camera && IsValidUnityObject(camera)) {
        Vector3 p = GetComponentPosition(camera);
        if (p.IsValid()) return p;
    }

    return Vector3{};
}

bool Engine::WorldToScreen(MonoObject* camera, const Vector3& world, Vector3& screen, float screenWidth, float screenHeight) {
    if (!camera || !m_methodWorldToScreen || !IsValidUnityObject(camera) || !world.IsValid()) {
        return false;
    }

    void* params[1] = { const_cast<Vector3*>(&world) };
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodWorldToScreen, camera, params, &exc);
    if (!res || exc) return false;

    Vector3 unboxed;
    if (!Mono::Bridge::Get().Unbox(res, unboxed) || !unboxed.IsValid()) {
        return false;
    }

    if (unboxed.z <= 0.1f) {
        return false;
    }

    float normX = 0.0f;
    float normY = 0.0f;

    if (m_CachedCameraWidth > 0 && m_CachedCameraHeight > 0) {
        normX = unboxed.x / static_cast<float>(m_CachedCameraWidth);
        normY = unboxed.y / static_cast<float>(m_CachedCameraHeight);
    } else {
        normX = unboxed.x / screenWidth;
        normY = unboxed.y / screenHeight;
    }

    screen.x = normX * screenWidth;
    screen.y = (1.0f - normY) * screenHeight;
    screen.z = unboxed.z;

    if (std::isnan(screen.x) || std::isnan(screen.y)) return false;
    if (screen.x < -4000.0f || screen.x > screenWidth + 4000.0f) return false;
    if (screen.y < -4000.0f || screen.y > screenHeight + 4000.0f) return false;

    return true;
}

void Engine::SetCursorVisible(bool visible) {
    if (!m_methodSetCursorVisible) return;
    void* params[1] = { &visible };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodSetCursorVisible, nullptr, params, &exc);
}

bool Engine::GetCursorVisible() {
    if (!m_methodGetCursorVisible) return false;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetCursorVisible, nullptr, nullptr, &exc);
    if (!res || exc) return false;

    bool val = false;
    if (Mono::Bridge::Get().Unbox(res, val)) {
        return val;
    }
    return false;
}

void Engine::SetCursorLockState(int lockState) {
    if (!m_methodSetCursorLockState) return;
    void* params[1] = { &lockState };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodSetCursorLockState, nullptr, params, &exc);
}

int Engine::GetCursorLockState() {
    if (!m_methodGetCursorLockState) return 0;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetCursorLockState, nullptr, nullptr, &exc);
    if (!res || exc) return 0;

    int val = 0;
    if (Mono::Bridge::Get().Unbox(res, val)) {
        return val;
    }
    return 0;
}

void Engine::UpdateCursorState(bool menuVisible) {
    if (!m_klassCursor) return;

    if (menuVisible) {
        if (!m_MenuWasVisible) {
            m_SavedLockState = GetCursorLockState();
            m_SavedCursorVisible = GetCursorVisible();
            m_MenuWasVisible = true;
            LOG_INFO("Menu opened: saved cursor state (lockState=%d, visible=%d)", m_SavedLockState, m_SavedCursorVisible);
        }

        SetCursorLockState(0);
        SetCursorVisible(true);
        ::ClipCursor(nullptr);
    } else {
        if (m_MenuWasVisible) {
            m_MenuWasVisible = false;
            LOG_INFO("Menu closed: restoring cursor state (lockState=%d, visible=%d)", m_SavedLockState, m_SavedCursorVisible);

            if (m_SavedLockState == 1) {
                SetCursorLockState(1);
                SetCursorVisible(false);
            } else if (m_SavedLockState == 0) {
                SetCursorLockState(0);
                SetCursorVisible(m_SavedCursorVisible);
            } else {
                bool inGame = (FindActiveCamera() != nullptr);
                SetCursorLockState(inGame ? 1 : 0);
                SetCursorVisible(!inGame);
            }
        }
    }
}

void Engine::SetAmbientLight(const Color& color) {
    if (!m_methodSetAmbientLight) return;
    void* args[1] = { const_cast<Color*>(&color) };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodSetAmbientLight, nullptr, args, &exc);
}

Color Engine::GetAmbientLight() {
    if (!m_methodGetAmbientLight) return Color{ 0.0f, 0.0f, 0.0f, 1.0f };
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetAmbientLight, nullptr, nullptr, &exc);
    if (!res || exc) return Color{ 0.0f, 0.0f, 0.0f, 1.0f };
    Color c;
    if (Mono::Bridge::Get().Unbox(res, c)) return c;
    return Color{ 0.0f, 0.0f, 0.0f, 1.0f };
}

void Engine::SetAmbientIntensity(float intensity) {
    if (!m_methodSetAmbientIntensity) return;
    void* args[1] = { &intensity };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodSetAmbientIntensity, nullptr, args, &exc);
}

float Engine::GetAmbientIntensity() {
    if (!m_methodGetAmbientIntensity) return 1.0f;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetAmbientIntensity, nullptr, nullptr, &exc);
    if (!res || exc) return 1.0f;
    float val = 1.0f;
    if (Mono::Bridge::Get().Unbox(res, val)) return val;
    return 1.0f;
}

void Engine::SetFog(bool enabled) {
    if (!m_methodSetFog) return;
    void* args[1] = { &enabled };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodSetFog, nullptr, args, &exc);
}

bool Engine::GetFog() {
    if (!m_methodGetFog) return true;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodGetFog, nullptr, nullptr, &exc);
    if (!res || exc) return true;
    bool val = true;
    if (Mono::Bridge::Get().Unbox(res, val)) return val;
    return true;
}

void Engine::SetFogDensity(float density) {
    if (!m_methodSetFogDensity) return;
    void* args[1] = { &density };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodSetFogDensity, nullptr, args, &exc);
}

void Engine::SetLightIntensity(MonoObject* light, float intensity) {
    if (!light || !m_methodLightSetIntensity || !IsValidUnityObject(light)) return;
    void* args[1] = { &intensity };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodLightSetIntensity, light, args, &exc);
}

float Engine::GetLightIntensity(MonoObject* light) {
    if (!light || !m_methodLightGetIntensity || !IsValidUnityObject(light)) return 1.0f;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodLightGetIntensity, light, nullptr, &exc);
    if (!res || exc) return 1.0f;
    float val = 1.0f;
    if (Mono::Bridge::Get().Unbox(res, val)) return val;
    return 1.0f;
}

void Engine::SetLightRange(MonoObject* light, float range) {
    if (!light || !m_methodLightSetRange || !IsValidUnityObject(light)) return;
    void* args[1] = { &range };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodLightSetRange, light, args, &exc);
}

float Engine::GetLightRange(MonoObject* light) {
    if (!light || !m_methodLightGetRange || !IsValidUnityObject(light)) return 10.0f;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodLightGetRange, light, nullptr, &exc);
    if (!res || exc) return 10.0f;
    float val = 10.0f;
    if (Mono::Bridge::Get().Unbox(res, val)) return val;
    return 10.0f;
}

void Engine::SetLightSpotAngle(MonoObject* light, float spotAngle) {
    if (!light || !m_methodLightSetSpotAngle || !IsValidUnityObject(light)) return;
    void* args[1] = { &spotAngle };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodLightSetSpotAngle, light, args, &exc);
}

float Engine::GetLightSpotAngle(MonoObject* light) {
    if (!light || !m_methodLightGetSpotAngle || !IsValidUnityObject(light)) return 45.0f;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodLightGetSpotAngle, light, nullptr, &exc);
    if (!res || exc) return 45.0f;
    float val = 45.0f;
    if (Mono::Bridge::Get().Unbox(res, val)) return val;
    return 45.0f;
}

void Engine::SetLightEnabled(MonoObject* light, bool enabled) {
    if (!light || !m_methodBehaviourSetEnabled || !IsValidUnityObject(light)) return;
    void* args[1] = { &enabled };
    MonoObject* exc = nullptr;
    Mono::Bridge::Get().InvokeMethod(m_methodBehaviourSetEnabled, light, args, &exc);
}

bool Engine::GetLightEnabled(MonoObject* light) {
    if (!light || !m_methodBehaviourGetEnabled || !IsValidUnityObject(light)) return false;
    MonoObject* exc = nullptr;
    MonoObject* res = Mono::Bridge::Get().InvokeMethod(m_methodBehaviourGetEnabled, light, nullptr, &exc);
    if (!res || exc) return false;
    bool val = false;
    if (Mono::Bridge::Get().Unbox(res, val)) return val;
    return false;
}

}
