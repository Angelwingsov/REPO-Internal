#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "unity/unity_math.h"
#include "mono/mono_types.h"

namespace Game {

struct MonsterEntity {
    uintptr_t parentAddress = 0;
    uintptr_t enemyAddress = 0;
    std::string name;
    Unity::Vector3 worldPosition;
    Unity::Vector3 screenPosition;
    Unity::Vector3 topScreenPosition;
    Unity::Vector3 bottomScreenPosition;
    bool hasScreenBox = false;
    float distance = 0.0f;
    int32_t currentState = 0;
    bool isSpawned = false;
    bool isStunned = false;
    bool isChasing = false;
};

struct PlayerEntity {
    Unity::Vector3 screenBoxMin;
    Unity::Vector3 screenBoxMax;
    uintptr_t address = 0;
    std::string name;
    Unity::Vector3 worldPosition;
    Unity::Vector3 screenPosition;
    Unity::Vector3 topScreenPosition;
    Unity::Vector3 bottomScreenPosition;
    bool hasScreenBox = false;
    float distance = 0.0f;
    int32_t health = 100;
    int32_t maxHealth = 100;
    bool isDead = false;
    bool isLocal = false;
};

struct ValuableEntity {
    uintptr_t address = 0;
    std::string name;
    Unity::Vector3 worldPosition;
    Unity::Vector3 screenPosition;
    float distance = 0.0f;
    float dollarValue = 0.0f;
    bool isCommitted = false;
};

struct ItemEntity {
    uintptr_t address = 0;
    std::string name;
    Unity::Vector3 worldPosition;
    Unity::Vector3 screenPosition;
    float distance = 0.0f;
    int32_t value = 0;
};

class EntityManager {
public:
    static EntityManager& Get();

    void CollectMonsters(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, std::vector<MonsterEntity>& out);
    void CollectPlayers(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, bool hideLocal, std::vector<PlayerEntity>& out);
    void CollectValuables(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, bool hideExtracted, float minDollar, std::vector<ValuableEntity>& out, const Unity::Vector3& camForward = {});
    void CollectItems(MonoObject* camera, const Unity::Vector3& refPos, float screenW, float screenH, float maxDist, std::vector<ItemEntity>& out, const Unity::Vector3& camForward = {});

    void ClearCache();

private:
    EntityManager() = default;

    std::unordered_map<uintptr_t, std::string> m_ValuableNameCache;
    std::unordered_map<uintptr_t, MonoObject*> m_ValuableTransformCache;
    std::unordered_map<uintptr_t, std::string> m_ItemNameCache;
    std::unordered_map<uintptr_t, MonoObject*> m_ItemTransformCache;

    std::vector<uintptr_t> m_ValuableAddressBuffer;
    std::vector<uintptr_t> m_ItemAddressBuffer;
};

}
