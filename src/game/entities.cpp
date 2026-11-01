#include "entities.h"
#include "player_bounds.h"
#include "state.h"
#include "unity/unity_engine.h"
#include "memory/memory.h"
#include "mono/mono_bridge.h"

namespace Game {

EntityManager& EntityManager::Get() {
    static EntityManager instance;
    return instance;
}

void EntityManager::CollectMonsters(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, std::vector<MonsterEntity>& out) {
    out.clear();
    auto& gameState = GameState::Get();
    if (!gameState.klassEnemyDirector || !camera) return;

    uintptr_t enemyDirector = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassEnemyDirector, "instance"));
    if (!enemyDirector || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(enemyDirector))) {
        return;
    }

    uintptr_t enemiesList = 0;
    if (!Memory::ReadPtr(enemyDirector + gameState.offsets.EnemyDirector_enemiesSpawned, enemiesList)) {
        return;
    }

    std::vector<uintptr_t> enemyParents;
    if (!Memory::ReadMonoList(enemiesList, enemyParents)) {
        return;
    }

    auto& engine = Unity::Engine::Get();

    for (uintptr_t enemyParent : enemyParents) {
        if (!enemyParent || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(enemyParent))) {
            continue;
        }

        uint8_t spawned = 0;
        Memory::ReadByte(enemyParent + gameState.offsets.EnemyParent_Spawned, spawned);
        if (!spawned) continue;

        uintptr_t enemy = 0;
        Memory::ReadPtr(enemyParent + gameState.offsets.EnemyParent_Enemy, enemy);
        if (!enemy || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(enemy))) {
            continue;
        }

        uintptr_t centerTransform = 0;
        Memory::ReadPtr(enemy + gameState.offsets.Enemy_CenterTransform, centerTransform);

        Unity::Vector3 worldPos;
        if (centerTransform && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(centerTransform))) {
            worldPos = engine.GetTransformPosition(reinterpret_cast<MonoObject*>(centerTransform));
        } else {
            worldPos = engine.GetComponentPosition(reinterpret_cast<MonoObject*>(enemy));
        }

        if (!worldPos.IsValid() || worldPos.IsZero()) continue;

        float dist = refPos.Distance(worldPos);
        if (dist > maxDist || dist < 0.1f) continue;

        Unity::Vector3 screenPos;
        if (!engine.WorldToScreen(camera, worldPos, screenPos, screenW, screenH)) continue;

        MonsterEntity monster;
        monster.parentAddress = enemyParent;
        monster.enemyAddress = enemy;
        monster.worldPosition = worldPos;
        monster.screenPosition = screenPos;
        monster.distance = dist;
        monster.isSpawned = true;

        uintptr_t mName = 0;
        if (Memory::ReadPtr(enemyParent + gameState.offsets.EnemyParent_enemyName, mName) && mName) {
            monster.name = Mono::Bridge::Get().ReadMonoString(reinterpret_cast<MonoString*>(mName));
        }
        if (monster.name.empty()) monster.name = "Monster";

        Memory::ReadInt(enemy + gameState.offsets.Enemy_CurrentState, monster.currentState);
        uint8_t stunned = 0;
        Memory::ReadByte(enemy + gameState.offsets.Enemy_Stunned, stunned);
        monster.isStunned = (stunned != 0);

        monster.isChasing = (monster.currentState == 3 || monster.currentState == 4 || monster.currentState == 5);

        Unity::Vector3 topWorld = worldPos;
        topWorld.y += 0.9f;
        Unity::Vector3 bottomWorld = worldPos;
        bottomWorld.y -= 0.9f;

        if (engine.WorldToScreen(camera, topWorld, monster.topScreenPosition, screenW, screenH) &&
            engine.WorldToScreen(camera, bottomWorld, monster.bottomScreenPosition, screenW, screenH)) {
            monster.hasScreenBox = true;
        }

        out.push_back(monster);
    }
}

void EntityManager::CollectPlayers(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, bool hideLocal, std::vector<PlayerEntity>& out) {
    out.clear();
    auto& gameState = GameState::Get();
    if (!gameState.klassGameDirector || !camera) return;

    uintptr_t gameDirector = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassGameDirector, "instance"));
    if (!gameDirector || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(gameDirector))) {
        return;
    }

    uintptr_t playerList = 0;
    if (!Memory::ReadPtr(gameDirector + gameState.offsets.GameDirector_PlayerList, playerList)) {
        return;
    }

    std::vector<uintptr_t> playerAvatars;
    if (!Memory::ReadMonoList(playerList, playerAvatars)) {
        return;
    }

    auto& engine = Unity::Engine::Get();

    for (uintptr_t avatar : playerAvatars) {
        if (!avatar || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(avatar))) {
            continue;
        }

        uint8_t isLocal = 0;
        Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_isLocal, isLocal);
        if (isLocal && hideLocal) {
            continue;
        }

        uint8_t deadSet = 0;
        Memory::ReadByte(avatar + gameState.offsets.PlayerAvatar_deadSet, deadSet);

        uintptr_t playerTransform = 0;
        Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerTransform, playerTransform);

        Unity::Vector3 worldPos;
        if (playerTransform && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerTransform))) {
            worldPos = engine.GetTransformPosition(reinterpret_cast<MonoObject*>(playerTransform));
        } else {
            worldPos = engine.GetComponentPosition(reinterpret_cast<MonoObject*>(avatar));
        }

        if (!worldPos.IsValid()) continue;
        Unity::Bounds bounds = PlayerFallbackBounds(worldPos);
        uintptr_t collider = 0;
        if (gameState.offsets.PlayerAvatar_playerCollider > 0 &&
            Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerCollider, collider) && collider) {
            Unity::Bounds actual;
            if (engine.TryGetColliderBounds(reinterpret_cast<MonoObject*>(collider), actual)) bounds = actual;
        }

        uintptr_t visuals=0;
        if (Memory::ReadPtr(avatar+gameState.offsets.PlayerAvatar_playerAvatarVisuals,visuals) &&
            Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(visuals))) {
            uintptr_t meshParent=0;
            Unity::Bounds visible;
            bool hasSkinned=false;
            bool hasVisible=gameState.offsets.PlayerAvatarVisuals_meshParent>0 &&
                Memory::ReadPtr(visuals+gameState.offsets.PlayerAvatarVisuals_meshParent,meshParent) &&
                engine.TryGetVisualBounds(reinterpret_cast<MonoObject*>(meshParent),visible,hasSkinned);
            if (hasVisible) bounds=visible;

            if ((!hasVisible || hasSkinned) && gameState.offsets.PlayerAvatarVisuals_headTop>0) {
                uintptr_t head=0;
                if (Memory::ReadPtr(visuals+gameState.offsets.PlayerAvatarVisuals_headTop,head) &&
                    Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(head))) {
                    auto position=engine.GetTransformPosition(reinterpret_cast<MonoObject*>(head));
                    if (FitPlayerBoundsToHead(bounds,position,visible)) bounds=visible;
                }
            }
        }
        worldPos = bounds.center;

        float dist = refPos.Distance(worldPos);
        if (dist > maxDist || dist < 0.1f) continue;

        Unity::Vector3 screenPos;
        if (!engine.WorldToScreen(camera, worldPos, screenPos, screenW, screenH)) continue;

        PlayerEntity player;
        player.address = avatar;
        player.worldPosition = worldPos;
        player.screenPosition = screenPos;
        player.distance = dist;
        player.isLocal = (isLocal != 0);
        player.isDead = (deadSet != 0);

        uintptr_t mName = 0;
        if (Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerName, mName) && mName) {
            player.name = Mono::Bridge::Get().ReadMonoString(reinterpret_cast<MonoString*>(mName));
        }
        if (player.name.empty()) {
            player.name = player.isLocal ? "Local Player" : "Player";
        }

        uintptr_t healthObj = 0;
        if (Memory::ReadPtr(avatar + gameState.offsets.PlayerAvatar_playerHealth, healthObj) && healthObj &&
            Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(healthObj))) {
            Memory::ReadInt(healthObj + gameState.offsets.PlayerHealth_health, player.health);
            Memory::ReadInt(healthObj + gameState.offsets.PlayerHealth_maxHealth, player.maxHealth);
            if (player.maxHealth <= 0) player.maxHealth = 100;
        } else {
            player.health = player.isDead ? 0 : 100;
            player.maxHealth = 100;
        }

        ScreenBounds screenBounds;
        if (ProjectPlayerBounds(bounds, [&](const Unity::Vector3& point, Unity::Vector3& projected) {
            return engine.WorldToScreen(camera, point, projected, screenW, screenH);
        }, screenBounds)) {
            player.screenBoxMin = screenBounds.min;
            player.screenBoxMax = screenBounds.max;
            float centerX = (screenBounds.min.x + screenBounds.max.x) * .5f;
            player.topScreenPosition = {centerX, screenBounds.min.y, screenPos.z};
            player.bottomScreenPosition = {centerX, screenBounds.max.y, screenPos.z};
            player.screenPosition.x = centerX;
            player.hasScreenBox = true;
        } else {

            Unity::Vector3 head;
            if (engine.WorldToScreen(camera, bounds.center + Unity::Vector3(0, bounds.extents.y, 0), head, screenW, screenH))
                player.screenPosition = head;
            else continue;
        }

        out.push_back(player);
    }
}

void EntityManager::ClearCache() {
    m_ValuableNameCache.clear();
    m_ValuableTransformCache.clear();
    m_ItemNameCache.clear();
    m_ItemTransformCache.clear();
}

void EntityManager::CollectValuables(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, bool hideExtracted, float minDollar, std::vector<ValuableEntity>& out, const Unity::Vector3& camForward) {
    out.clear();
    auto& gameState = GameState::Get();
    if (!gameState.klassValuableDirector || !camera) return;

    if (m_ValuableNameCache.size() > 2000) {
        ClearCache();
    }

    uintptr_t valuableDirector = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassValuableDirector, "instance"));
    if (!valuableDirector || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(valuableDirector))) {
        return;
    }

    uintptr_t valList = 0;
    if (!Memory::ReadPtr(valuableDirector + gameState.offsets.ValuableDirector_valuableList, valList)) {
        return;
    }

    if (!Memory::ReadMonoList(valList, m_ValuableAddressBuffer)) {
        return;
    }

    out.reserve(m_ValuableAddressBuffer.size());
    auto& engine = Unity::Engine::Get();

    const bool hasCamForward = camForward.IsValid() && !camForward.IsZero();
    const float maxDistSq = maxDist * maxDist;

    for (uintptr_t valuable : m_ValuableAddressBuffer) {
        if (!valuable || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(valuable))) {
            continue;
        }

        if (hideExtracted) {
            uint8_t committed = 0;
            Memory::ReadByte(valuable + gameState.offsets.ValuableObject_committedToExtraction, committed);
            if (committed) continue;
        }

        float dollarValue = 0.0f;
        Memory::ReadFloat(valuable + gameState.offsets.ValuableObject_dollarValueCurrent, dollarValue);
        if (dollarValue <= 0.0f) {
            Memory::ReadFloat(valuable + gameState.offsets.ValuableObject_dollarValueOriginal, dollarValue);
        }

        if (dollarValue < minDollar) continue;

        // 1. Direct fast memory read from PhysGrabObject.midPoint (exact world position, 0 Mono invocations)
        Unity::Vector3 worldPos{};
        bool gotPos = false;

        if (gameState.offsets.ValuableObject_physGrabObject > 0 && gameState.offsets.PhysGrabObject_midPoint > 0) {
            uintptr_t pgo = 0;
            if (Memory::ReadPtr(valuable + gameState.offsets.ValuableObject_physGrabObject, pgo) && pgo &&
                Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(pgo))) {
                Unity::Vector3 midPoint{};
                if (Memory::Read<Unity::Vector3>(pgo + gameState.offsets.PhysGrabObject_midPoint, midPoint) &&
                    midPoint.IsValid() && !midPoint.IsZero()) {
                    worldPos = midPoint;
                    gotPos = true;
                }
            }
        }

        // 2. Fallback to cached Transform (avoids repeated get_transform invocations)
        if (!gotPos) {
            MonoObject* transform = nullptr;
            auto itTransform = m_ValuableTransformCache.find(valuable);
            if (itTransform != m_ValuableTransformCache.end() && Unity::Engine::IsValidUnityObject(itTransform->second)) {
                transform = itTransform->second;
            } else {
                transform = engine.GetTransform(reinterpret_cast<MonoObject*>(valuable));
                if (transform) {
                    m_ValuableTransformCache[valuable] = transform;
                }
            }

            if (transform) {
                worldPos = engine.GetTransformPosition(transform);
                if (worldPos.IsValid() && !worldPos.IsZero()) {
                    gotPos = true;
                }
            }
        }

        if (!gotPos) continue;

        // 3. Fast distance squared check before square root
        Unity::Vector3 toItem = worldPos - refPos;
        float distSq = toItem.LengthSq();
        if (distSq > maxDistSq || distSq < 0.01f) continue;
        float dist = std::sqrt(distSq);

        // 4. Frustum / camera-forward culling (culls items behind the camera plane, saving WorldToScreen)
        if (hasCamForward) {
            float dot = camForward.Dot(toItem / dist);
            if (dot < -0.1f) {
                continue;
            }
        }

        // 5. WorldToScreen projection
        Unity::Vector3 screenPos;
        if (!engine.WorldToScreen(camera, worldPos, screenPos, screenW, screenH)) continue;

        // 6. Name resolution via cache (avoids Mono string allocation & reflection every frame)
        ValuableEntity entity;
        entity.address = valuable;
        entity.worldPosition = worldPos;
        entity.screenPosition = screenPos;
        entity.distance = dist;
        entity.dollarValue = dollarValue;

        auto itName = m_ValuableNameCache.find(valuable);
        if (itName != m_ValuableNameCache.end()) {
            entity.name = itName->second;
        } else {
            entity.name = engine.GetObjectName(reinterpret_cast<MonoObject*>(valuable));
            if (entity.name.empty()) entity.name = "Valuable";
            m_ValuableNameCache[valuable] = entity.name;
        }

        out.push_back(std::move(entity));
    }
}

void EntityManager::CollectItems(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, std::vector<ItemEntity>& out, const Unity::Vector3& camForward) {
    out.clear();
    auto& gameState = GameState::Get();
    if (!gameState.klassItemManager || !camera) return;

    if (m_ItemNameCache.size() > 2000) {
        ClearCache();
    }

    uintptr_t itemMgr = reinterpret_cast<uintptr_t>(Mono::Bridge::Get().GetStaticFieldValue(gameState.klassItemManager, "instance"));
    if (!itemMgr || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(itemMgr))) {
        return;
    }

    uintptr_t spawnedItemsList = 0;
    if (!Memory::ReadPtr(itemMgr + gameState.offsets.ItemManager_spawnedItems, spawnedItemsList)) {
        return;
    }

    if (!Memory::ReadMonoList(spawnedItemsList, m_ItemAddressBuffer)) {
        return;
    }

    out.reserve(m_ItemAddressBuffer.size());
    auto& engine = Unity::Engine::Get();

    const bool hasCamForward = camForward.IsValid() && !camForward.IsZero();
    const float maxDistSq = maxDist * maxDist;

    for (uintptr_t itemAttr : m_ItemAddressBuffer) {
        if (!itemAttr || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(itemAttr))) {
            continue;
        }

        // 1. Direct fast memory read from PhysGrabObject.midPoint
        Unity::Vector3 worldPos{};
        bool gotPos = false;

        if (gameState.offsets.ItemAttributes_physGrabObject > 0 && gameState.offsets.PhysGrabObject_midPoint > 0) {
            uintptr_t pgo = 0;
            if (Memory::ReadPtr(itemAttr + gameState.offsets.ItemAttributes_physGrabObject, pgo) && pgo &&
                Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(pgo))) {
                Unity::Vector3 midPoint{};
                if (Memory::Read<Unity::Vector3>(pgo + gameState.offsets.PhysGrabObject_midPoint, midPoint) &&
                    midPoint.IsValid() && !midPoint.IsZero()) {
                    worldPos = midPoint;
                    gotPos = true;
                }
            }
        }

        // 2. Fallback to cached Transform
        if (!gotPos) {
            MonoObject* transform = nullptr;
            auto itTransform = m_ItemTransformCache.find(itemAttr);
            if (itTransform != m_ItemTransformCache.end() && Unity::Engine::IsValidUnityObject(itTransform->second)) {
                transform = itTransform->second;
            } else {
                transform = engine.GetTransform(reinterpret_cast<MonoObject*>(itemAttr));
                if (transform) {
                    m_ItemTransformCache[itemAttr] = transform;
                }
            }

            if (transform) {
                worldPos = engine.GetTransformPosition(transform);
                if (worldPos.IsValid() && !worldPos.IsZero()) {
                    gotPos = true;
                }
            }
        }

        if (!gotPos) continue;

        // 3. Fast distance squared check before square root
        Unity::Vector3 toItem = worldPos - refPos;
        float distSq = toItem.LengthSq();
        if (distSq > maxDistSq || distSq < 0.01f) continue;
        float dist = std::sqrt(distSq);

        // 4. Frustum / camera-forward culling
        if (hasCamForward) {
            float dot = camForward.Dot(toItem / dist);
            if (dot < -0.1f) {
                continue;
            }
        }

        // 5. WorldToScreen projection
        Unity::Vector3 screenPos;
        if (!engine.WorldToScreen(camera, worldPos, screenPos, screenW, screenH)) continue;

        ItemEntity entity;
        entity.address = itemAttr;
        entity.worldPosition = worldPos;
        entity.screenPosition = screenPos;
        entity.distance = dist;

        // 6. Name resolution via cache
        auto itName = m_ItemNameCache.find(itemAttr);
        if (itName != m_ItemNameCache.end()) {
            entity.name = itName->second;
        } else {
            uintptr_t mName = 0;
            Memory::ReadPtr(itemAttr + gameState.offsets.ItemAttributes_itemName, mName);
            if (mName) entity.name = Mono::Bridge::Get().ReadMonoString(reinterpret_cast<MonoString*>(mName));
            if (entity.name.empty()) {
                Memory::ReadPtr(itemAttr + gameState.offsets.ItemAttributes_instanceName, mName);
                if (mName) entity.name = Mono::Bridge::Get().ReadMonoString(reinterpret_cast<MonoString*>(mName));
            }
            if (entity.name.empty()) entity.name = "Item";
            m_ItemNameCache[itemAttr] = entity.name;
        }

        Memory::ReadInt(itemAttr + gameState.offsets.ItemAttributes_value, entity.value);

        out.push_back(std::move(entity));
    }
}

}
