#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <deque>
#include "unity/unity_math.h"

namespace Features {

enum class ItemCategory : int {
    All = 0,
    Weapons = 1,
    Utility = 2,
    Upgrades = 3,
    Survival = 4,
    Valuables = 5
};

struct ItemEntry {
    std::string name;
    std::string resourcePath;
    ItemCategory category;
};

struct SpawnRequest {
    int itemIndex = -1;
    std::string resourcePath;
    std::string displayName;
    int amount = 1;
    bool atCrosshair = false;
    float distance = 2.0f;
    int mode = 0;
    bool isAll = false;
    ItemCategory allCategory = ItemCategory::All;
};

class Spawner {
public:
    static Spawner& Get();

    void Initialize();
    void Update(float dt);

    const std::vector<ItemEntry>& GetAllItems() const { return m_Items; }
    std::vector<int> GetFilteredIndices(ItemCategory category, const std::string& filter = "") const;
    const ItemEntry* GetItem(int index) const;
    int GetItemCount() const { return static_cast<int>(m_Items.size()); }

    bool QueueSpawn(int itemIndex, int amount = 1, bool atCrosshair = false, float distance = 2.0f, int mode = 0);
    bool QueueSpawnAll(bool atCrosshair = false, float distance = 2.0f, int mode = 0);

    bool TakeStatus(std::string& outStatus);
    void SetStatus(const std::string& status);

public:
    Spawner();

private:
    void PopulateDefaultCatalog();
    void ScanRuntimeItems();
    bool ExecuteSpawnRequest(const SpawnRequest& req);

    std::vector<ItemEntry> m_Items;
    std::mutex m_QueueMutex;
    std::deque<SpawnRequest> m_Queue;

    std::mutex m_StatusMutex;
    std::string m_Status = "Ready";
    bool m_HasNewStatus = true;

    bool m_PrevHotkeyDown = false;
    bool m_HotkeyActive = false;
    float m_ScanTimer = 0.0f;
};

}
