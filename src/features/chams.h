#pragma once
#include <windows.h>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "unity/unity_math.h"
#include "mono/mono_types.h"

namespace Features {

class Chams {
public:
    static Chams& Get();

    void Initialize();
    void Update();
    void Restore();

private:
    Chams() = default;

    struct RendererBackup {
        MonoObject* originalMaterials = nullptr;
        bool isArray = false;
        uint32_t pin = 0;
    };

    void UpdateInternal();
    void RestoreInternal();
    bool EnsureMaterials();
    void ApplyChamsToPlayer(uintptr_t avatar, bool isLocal, MonoObject* camera, const Unity::Vector3& camPos);
    void ApplyChamsToMonster(uintptr_t enemyParent, uintptr_t enemy, MonoObject* camera, const Unity::Vector3& camPos);
    void ApplyMaterialToRenderers(MonoObject* parentObj, MonoObject* material);
    void RestoreRenderer(uintptr_t rendererAddress, RendererBackup& backup);

    bool m_Initialized = false;
    MonoObject* m_MatVisible = nullptr;
    MonoObject* m_MatInvisible = nullptr;
    uint32_t m_PinMatVisible = 0;
    uint32_t m_PinMatInvisible = 0;

    Unity::Color m_LastColorVisible{};
    Unity::Color m_LastColorInvisible{};

    std::unordered_map<uintptr_t, RendererBackup> m_TrackedRenderers;
    std::vector<uintptr_t> m_ActiveThisFrame;
};

}
