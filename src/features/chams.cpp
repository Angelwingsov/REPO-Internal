#include "chams.h"
#include "core/config.h"
#include "core/logger.h"
#include "game/state.h"
#include "unity/unity_engine.h"
#include "mono/mono_bridge.h"
#include "memory/memory.h"
#include <algorithm>

namespace Features {

Chams& Chams::Get() {
    static Chams instance;
    return instance;
}

void Chams::Initialize() {
    m_Initialized = true;
    LOG_OK("Chams feature initialized");
}

bool Chams::EnsureMaterials() {
    auto& engine = Unity::Engine::Get();

    if (m_MatVisible && m_MatInvisible &&
        Unity::Engine::IsValidUnityObject(m_MatVisible) &&
        Unity::Engine::IsValidUnityObject(m_MatInvisible)) {
        return true;
    }

    auto& bridge = Mono::Bridge::Get();
    if (m_PinMatVisible) {
        bridge.FreePin(m_PinMatVisible);
        m_PinMatVisible = 0;
    }
    if (m_PinMatInvisible) {
        bridge.FreePin(m_PinMatInvisible);
        m_PinMatInvisible = 0;
    }
    m_MatVisible = nullptr;
    m_MatInvisible = nullptr;

    MonoObject* shader = engine.FindShader("Hidden/Internal-Colored");
    if (!shader) shader = engine.FindShader("GUI/Text Shader");
    if (!shader) shader = engine.FindShader("Unlit/Color");
    if (!shader) shader = engine.FindShader("Sprites/Default");
    if (!shader) shader = engine.FindShader("Standard");

    if (!shader) {
        return false;
    }

    m_MatVisible = engine.CreateMaterial(shader);
    m_MatInvisible = engine.CreateMaterial(shader);

    if (!m_MatVisible || !m_MatInvisible ||
        !Unity::Engine::IsValidUnityObject(m_MatVisible) ||
        !Unity::Engine::IsValidUnityObject(m_MatInvisible)) {
        m_MatVisible = nullptr;
        m_MatInvisible = nullptr;
        return false;
    }

    m_PinMatVisible = bridge.PinObject(m_MatVisible);
    m_PinMatInvisible = bridge.PinObject(m_MatInvisible);

    engine.SetMaterialInt(m_MatVisible, "_ZTest", 8);
    engine.SetMaterialInt(m_MatVisible, "_ZWrite", 0);
    engine.SetMaterialInt(m_MatVisible, "_Cull", 0);

    engine.SetMaterialInt(m_MatInvisible, "_ZTest", 8);
    engine.SetMaterialInt(m_MatInvisible, "_ZWrite", 0);
    engine.SetMaterialInt(m_MatInvisible, "_Cull", 0);

    auto& cfg = Core::Config::Get();
    m_LastColorVisible = Unity::Color(cfg.chams.colorVisible[0], cfg.chams.colorVisible[1], cfg.chams.colorVisible[2], cfg.chams.colorVisible[3]);
    m_LastColorInvisible = Unity::Color(cfg.chams.colorInvisible[0], cfg.chams.colorInvisible[1], cfg.chams.colorInvisible[2], cfg.chams.colorInvisible[3]);

    engine.SetMaterialColor(m_MatVisible, "_Color", m_LastColorVisible);
    engine.SetMaterialColor(m_MatInvisible, "_Color", m_LastColorInvisible);

    return true;
}

void Chams::RestoreRenderer(uintptr_t rendererAddress, RendererBackup& backup) {
    if (Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(rendererAddress))) {
        auto* rend = reinterpret_cast<MonoObject*>(rendererAddress);
        auto& engine = Unity::Engine::Get();
        if (backup.originalMaterials && Unity::Engine::IsValidUnityObject(backup.originalMaterials)) {
            if (backup.isArray) {
                engine.SetRendererSharedMaterials(rend, backup.originalMaterials);
            } else {
                engine.SetRendererSharedMaterial(rend, backup.originalMaterials);
            }
        }
    }

    if (backup.pin) {
        Mono::Bridge::Get().FreePin(backup.pin);
        backup.pin = 0;
    }
}

void Chams::RestoreInternal() {
    for (auto& [addr, backup] : m_TrackedRenderers) {
        RestoreRenderer(addr, backup);
    }
    m_TrackedRenderers.clear();
    m_ActiveThisFrame.clear();
}

void Chams::Restore() {
    __try {
        RestoreInternal();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        LOG_ERR("Chams::Restore intercepted native exception 0x%08X", GetExceptionCode());
    }
}

void Chams::ApplyMaterialToRenderers(MonoObject* parentObj, MonoObject* material) {
    if (!parentObj || !material) return;
    if (!Unity::Engine::IsValidUnityObject(parentObj) || !Unity::Engine::IsValidUnityObject(material)) return;

    auto& engine = Unity::Engine::Get();
    std::vector<MonoObject*> renderers;
    if (!engine.GetRenderers(parentObj, renderers) || renderers.empty()) return;

    for (MonoObject* rend : renderers) {
        if (!rend || !Unity::Engine::IsValidUnityObject(rend)) continue;
        uintptr_t rendAddr = reinterpret_cast<uintptr_t>(rend);
        m_ActiveThisFrame.push_back(rendAddr);

        auto it = m_TrackedRenderers.find(rendAddr);
        if (it == m_TrackedRenderers.end()) {
            bool isArray = false;
            MonoObject* origMaterials = engine.GetRendererSharedMaterials(rend);
            if (origMaterials && Unity::Engine::IsValidUnityObject(origMaterials)) {
                isArray = true;
            } else {
                origMaterials = engine.GetRendererSharedMaterial(rend);
            }

            if (!origMaterials || !Unity::Engine::IsValidUnityObject(origMaterials)) {
                continue;
            }
            if (origMaterials == m_MatVisible || origMaterials == m_MatInvisible) {
                continue;
            }

            uint32_t pin = Mono::Bridge::Get().PinObject(origMaterials);
            m_TrackedRenderers[rendAddr] = { origMaterials, isArray, pin };
            engine.SetRendererSharedMaterial(rend, material);
        } else {
            MonoObject* currentMat = engine.GetRendererSharedMaterial(rend);
            if (currentMat != material) {
                engine.SetRendererSharedMaterial(rend, material);
            }
        }
    }
}

void Chams::ApplyChamsToPlayer(uintptr_t avatar, bool isLocal, MonoObject* camera, const Unity::Vector3& camPos) {
    auto& gs = Game::GameState::Get();
    auto& engine = Unity::Engine::Get();

    uint8_t dead = 0;
    if (gs.offsets.PlayerAvatar_deadSet > 0) {
        Memory::ReadByte(avatar + gs.offsets.PlayerAvatar_deadSet, dead);
        if (dead) return;
    }

    uintptr_t visuals = 0;
    if (!Memory::ReadPtr(avatar + gs.offsets.PlayerAvatar_playerAvatarVisuals, visuals) ||
        !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(visuals))) return;

    uintptr_t meshParent = 0;
    if (gs.offsets.PlayerAvatarVisuals_meshParent <= 0 ||
        !Memory::ReadPtr(visuals + gs.offsets.PlayerAvatarVisuals_meshParent, meshParent) ||
        !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(meshParent))) return;

    Unity::Vector3 target;
    uintptr_t visionTarget = 0;
    if (gs.offsets.PlayerAvatar_PlayerVisionTarget > 0 &&
        Memory::ReadPtr(avatar + gs.offsets.PlayerAvatar_PlayerVisionTarget, visionTarget) &&
        Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(visionTarget))) {
        uintptr_t visionTransform = 0;
        if (gs.offsets.PlayerVisionTarget_VisionTransform > 0 &&
            Memory::ReadPtr(visionTarget + gs.offsets.PlayerVisionTarget_VisionTransform, visionTransform) &&
            Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(visionTransform))) {
            target = engine.GetTransformPosition(reinterpret_cast<MonoObject*>(visionTransform));
        }
    }

    if (!target.IsValid() && gs.offsets.PlayerAvatarVisuals_headTop > 0) {
        uintptr_t head = 0;
        if (Memory::ReadPtr(visuals + gs.offsets.PlayerAvatarVisuals_headTop, head) &&
            Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(head))) {
            target = engine.GetTransformPosition(reinterpret_cast<MonoObject*>(head));
        }
    }

    if (!target.IsValid()) {
        uintptr_t playerTransform = 0;
        if (Memory::ReadPtr(avatar + gs.offsets.PlayerAvatar_playerTransform, playerTransform) &&
            Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerTransform))) {
            target = engine.GetTransformPosition(reinterpret_cast<MonoObject*>(playerTransform));
            if (target.IsValid()) target.y += 1.2f;
        }
    }

    bool isVisible = true;
    if (isLocal) {
        auto& cfg = Core::Config::Get();
        if (cfg.thirdPerson.active && camPos.IsValid() && target.IsValid()) {
            isVisible = engine.CheckLineOfSight(camPos, target);
        } else {
            isVisible = true;
        }
    } else {
        if (camPos.IsValid() && target.IsValid()) {
            isVisible = engine.CheckLineOfSight(camPos, target);
        }
    }

    MonoObject* selectedMaterial = isVisible ? m_MatVisible : m_MatInvisible;
    if (!selectedMaterial) return;

    ApplyMaterialToRenderers(reinterpret_cast<MonoObject*>(meshParent), selectedMaterial);
}

void Chams::ApplyChamsToMonster(uintptr_t enemyParent, uintptr_t enemy, MonoObject* camera, const Unity::Vector3& camPos) {
    auto& gs = Game::GameState::Get();
    auto& engine = Unity::Engine::Get();

    uintptr_t centerTransform = 0;
    Unity::Vector3 target;
    if (gs.offsets.Enemy_CenterTransform > 0 &&
        Memory::ReadPtr(enemy + gs.offsets.Enemy_CenterTransform, centerTransform) &&
        Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(centerTransform))) {
        target = engine.GetTransformPosition(reinterpret_cast<MonoObject*>(centerTransform));
    }
    if (!target.IsValid()) {
        target = engine.GetComponentPosition(reinterpret_cast<MonoObject*>(enemy));
        if (target.IsValid()) target.y += 1.0f;
    }

    bool isVisible = true;
    if (camPos.IsValid() && target.IsValid()) {
        isVisible = engine.CheckLineOfSight(camPos, target);
    }

    MonoObject* selectedMaterial = isVisible ? m_MatVisible : m_MatInvisible;
    if (!selectedMaterial) return;

    MonoObject* enemyObj = reinterpret_cast<MonoObject*>(enemy);
    ApplyMaterialToRenderers(enemyObj, selectedMaterial);
}

void Chams::UpdateInternal() {
    auto& cfg = Core::Config::Get();
    if (!cfg.chams.enabled) {
        if (!m_TrackedRenderers.empty()) {
            RestoreInternal();
        }
        return;
    }

    auto& gs = Game::GameState::Get();
    if (gs.IsMapLoadingOrRestarting()) {
        if (!m_TrackedRenderers.empty()) {
            RestoreInternal();
        }
        return;
    }

    if (!EnsureMaterials()) return;

    Unity::Color curVis(cfg.chams.colorVisible[0], cfg.chams.colorVisible[1], cfg.chams.colorVisible[2], cfg.chams.colorVisible[3]);
    Unity::Color curInvis(cfg.chams.colorInvisible[0], cfg.chams.colorInvisible[1], cfg.chams.colorInvisible[2], cfg.chams.colorInvisible[3]);
    auto& engine = Unity::Engine::Get();

    if (curVis != m_LastColorVisible) {
        engine.SetMaterialColor(m_MatVisible, "_Color", curVis);
        m_LastColorVisible = curVis;
    }
    if (curInvis != m_LastColorInvisible) {
        engine.SetMaterialColor(m_MatInvisible, "_Color", curInvis);
        m_LastColorInvisible = curInvis;
    }

    MonoObject* camera = engine.FindActiveCamera();
    Unity::Vector3 camPos = engine.GetCameraPosition(camera);

    m_ActiveThisFrame.clear();

    if (gs.klassGameDirector) {
        uintptr_t gameDirector = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gs.klassGameDirector, "instance"));
        if (gameDirector && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(gameDirector))) {
            uintptr_t playerList = 0;
            if (Memory::ReadPtr(gameDirector + gs.offsets.GameDirector_PlayerList, playerList) && playerList) {
                std::vector<uintptr_t> avatars;
                if (Memory::ReadMonoList(playerList, avatars)) {
                    for (uintptr_t avatar : avatars) {
                        if (!avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) continue;

                        uint8_t isLocal = 0;
                        Memory::ReadByte(avatar + gs.offsets.PlayerAvatar_isLocal, isLocal);
                        if (isLocal && cfg.player.hideLocalPlayer) continue;

                        ApplyChamsToPlayer(avatar, isLocal != 0, camera, camPos);
                    }
                }
            }
        }
    }

    if (gs.klassEnemyDirector) {
        uintptr_t enemyDirector = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gs.klassEnemyDirector, "instance"));
        if (enemyDirector && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(enemyDirector))) {
            uintptr_t enemiesList = 0;
            if (Memory::ReadPtr(enemyDirector + gs.offsets.EnemyDirector_enemiesSpawned, enemiesList) && enemiesList) {
                std::vector<uintptr_t> enemyParents;
                if (Memory::ReadMonoList(enemiesList, enemyParents)) {
                    for (uintptr_t enemyParent : enemyParents) {
                        if (!enemyParent || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(enemyParent))) continue;

                        uint8_t spawned = 0;
                        Memory::ReadByte(enemyParent + gs.offsets.EnemyParent_Spawned, spawned);
                        if (!spawned) continue;

                        uintptr_t enemy = 0;
                        Memory::ReadPtr(enemyParent + gs.offsets.EnemyParent_Enemy, enemy);
                        if (!enemy || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(enemy))) continue;

                        ApplyChamsToMonster(enemyParent, enemy, camera, camPos);
                    }
                }
            }
        }
    }

    for (auto it = m_TrackedRenderers.begin(); it != m_TrackedRenderers.end(); ) {
        uintptr_t addr = it->first;
        if (std::find(m_ActiveThisFrame.begin(), m_ActiveThisFrame.end(), addr) == m_ActiveThisFrame.end()) {
            RestoreRenderer(addr, it->second);
            it = m_TrackedRenderers.erase(it);
        } else {
            ++it;
        }
    }
}

void Chams::Update() {
    __try {
        UpdateInternal();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        LOG_ERR("Chams::Update intercepted native exception 0x%08X", GetExceptionCode());
    }
}

}
