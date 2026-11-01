#include "spawner.h"
#include "core/config.h"
#include "core/keybind.h"
#include "core/logger.h"
#include "hook/input.h"
#include "game/state.h"
#include "memory/memory.h"
#include "unity/unity_engine.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace Features {

Spawner& Spawner::Get() {
    static Spawner instance;
    return instance;
}

Spawner::Spawner() {
    PopulateDefaultCatalog();
}

void Spawner::Initialize() {
    ScanRuntimeItems();
}

void Spawner::PopulateDefaultCatalog() {
    m_Items.clear();

    m_Items.push_back({ "Shotgun", "Items/Item Gun Shotgun", ItemCategory::Weapons });
    m_Items.push_back({ "Revolver", "Items/Item Gun Revolver", ItemCategory::Weapons });
    m_Items.push_back({ "Handgun", "Items/Item Gun Handgun", ItemCategory::Weapons });
    m_Items.push_back({ "Laser Gun", "Items/Item Gun Laser", ItemCategory::Weapons });
    m_Items.push_back({ "Shockwave Gun", "Items/Item Gun Shockwave", ItemCategory::Weapons });
    m_Items.push_back({ "Stun Gun", "Items/Item Gun Stun", ItemCategory::Weapons });
    m_Items.push_back({ "Tranq Gun", "Items/Item Gun Tranq", ItemCategory::Weapons });
    m_Items.push_back({ "Explosive Grenade", "Items/Item Grenade Explosive", ItemCategory::Weapons });
    m_Items.push_back({ "Shockwave Grenade", "Items/Item Grenade Shockwave", ItemCategory::Weapons });
    m_Items.push_back({ "Stun Grenade", "Items/Item Grenade Stun", ItemCategory::Weapons });
    m_Items.push_back({ "Duct Taped Grenade", "Items/Item Grenade Duct Taped", ItemCategory::Weapons });
    m_Items.push_back({ "Human Grenade", "Items/Item Grenade Human", ItemCategory::Weapons });
    m_Items.push_back({ "Explosive Mine", "Items/Item Mine Explosive", ItemCategory::Weapons });
    m_Items.push_back({ "Shockwave Mine", "Items/Item Mine Shockwave", ItemCategory::Weapons });
    m_Items.push_back({ "Stun Mine", "Items/Item Mine Stun", ItemCategory::Weapons });
    m_Items.push_back({ "Sword", "Items/Item Melee Sword", ItemCategory::Weapons });
    m_Items.push_back({ "Sledge Hammer", "Items/Item Melee Sledge Hammer", ItemCategory::Weapons });
    m_Items.push_back({ "Stun Baton", "Items/Item Melee Stun Baton", ItemCategory::Weapons });
    m_Items.push_back({ "Baseball Bat", "Items/Item Melee Baseball Bat", ItemCategory::Weapons });
    m_Items.push_back({ "Frying Pan", "Items/Item Melee Frying Pan", ItemCategory::Weapons });
    m_Items.push_back({ "Inflatable Hammer", "Items/Item Melee Inflatable Hammer", ItemCategory::Weapons });
    m_Items.push_back({ "Whip", "Items/Item Melee Whip", ItemCategory::Weapons });

    m_Items.push_back({ "Small Cart", "Items/Item Cart Small", ItemCategory::Utility });
    m_Items.push_back({ "Medium Cart", "Items/Item Cart Medium", ItemCategory::Utility });
    m_Items.push_back({ "Cart Cannon", "Items/Item Cart Cannon", ItemCategory::Utility });
    m_Items.push_back({ "Cart Laser", "Items/Item Cart Laser", ItemCategory::Utility });
    m_Items.push_back({ "Semiscooter", "Items/Item Vehicle Semiscooter", ItemCategory::Utility });
    m_Items.push_back({ "Small Semiscooter", "Items/Item Vehicle Semiscooter Small", ItemCategory::Utility });
    m_Items.push_back({ "Drone Battery", "Items/Item Drone Battery", ItemCategory::Utility });
    m_Items.push_back({ "Drone Feather", "Items/Item Drone Feather", ItemCategory::Utility });
    m_Items.push_back({ "Drone Indestructible", "Items/Item Drone Indestructible", ItemCategory::Utility });
    m_Items.push_back({ "Drone Torque", "Items/Item Drone Torque", ItemCategory::Utility });
    m_Items.push_back({ "Drone Zero Gravity", "Items/Item Drone Zero Gravity", ItemCategory::Utility });
    m_Items.push_back({ "Extraction Tracker", "Items/Item Extraction Tracker", ItemCategory::Utility });
    m_Items.push_back({ "Valuable Tracker", "Items/Item Valuable Tracker", ItemCategory::Utility });
    m_Items.push_back({ "Phase Bridge", "Items/Item Phase Bridge", ItemCategory::Utility });
    m_Items.push_back({ "Leaf Blower", "Items/Item Leaf blower", ItemCategory::Utility });
    m_Items.push_back({ "Orb Zero Gravity", "Items/Item Orb Zero Gravity", ItemCategory::Utility });
    m_Items.push_back({ "Staff Torque", "Items/Item Staff Torque", ItemCategory::Utility });
    m_Items.push_back({ "Staff Void", "Items/Item Staff Void", ItemCategory::Utility });
    m_Items.push_back({ "Staff Zero Gravity", "Items/Item Staff Zero Gravity", ItemCategory::Utility });
    m_Items.push_back({ "Walkie Talkie Box", "Items/Item WalkieTalkieBox", ItemCategory::Utility });
    m_Items.push_back({ "Duck Bucket", "Items/Item Duck Bucket", ItemCategory::Utility });
    m_Items.push_back({ "Rubber Duck", "Items/Item Rubber Duck", ItemCategory::Utility });

    m_Items.push_back({ "Player Health Upgrade", "Items/Item Upgrade Player Health", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Energy Upgrade", "Items/Item Upgrade Player Energy", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Sprint Speed Upgrade", "Items/Item Upgrade Player Sprint Speed", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Extra Jump Upgrade", "Items/Item Upgrade Player Extra Jump", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Grab Range Upgrade", "Items/Item Upgrade Player Grab Range", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Grab Strength Upgrade", "Items/Item Upgrade Player Grab Strength", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Crouch Rest Upgrade", "Items/Item Upgrade Player Crouch Rest", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Tumble Climb Upgrade", "Items/Item Upgrade Player Tumble Climb", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Tumble Launch Upgrade", "Items/Item Upgrade Player Tumble Launch", ItemCategory::Upgrades });
    m_Items.push_back({ "Player Tumble Wings Upgrade", "Items/Item Upgrade Player Tumble Wings", ItemCategory::Upgrades });
    m_Items.push_back({ "Map Player Count Upgrade", "Items/Item Upgrade Map Player Count", ItemCategory::Upgrades });
    m_Items.push_back({ "Death Head Battery Upgrade", "Items/Item Upgrade Death Head Battery", ItemCategory::Upgrades });

    m_Items.push_back({ "Large Health Pack", "Items/Item Health Pack Large", ItemCategory::Survival });
    m_Items.push_back({ "Medium Health Pack", "Items/Item Health Pack Medium", ItemCategory::Survival });
    m_Items.push_back({ "Small Health Pack", "Items/Item Health Pack Small", ItemCategory::Survival });
    m_Items.push_back({ "Power Crystal", "Items/Item Power Crystal", ItemCategory::Survival });
    m_Items.push_back({ "Revive Item", "Items/Item ReviveItem", ItemCategory::Survival });

    m_Items.push_back({ "Golden Pumpkin (Small)", "Valuables/02 Small/Valuable Golden Pumpkin Small", ItemCategory::Valuables });
    m_Items.push_back({ "Golden Pumpkin (Medium)", "Valuables/03 Medium/Valuable Golden Pumpkin Medium", ItemCategory::Valuables });
    m_Items.push_back({ "Golden Pumpkin (Big)", "Valuables/04 Big/Valuable Golden Pumpkin Big", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Vase (Small)", "Valuables/02 Small/Valuable Manor Vase Small", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Vase", "Valuables/03 Medium/Valuable Manor Vase", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Vase (Big)", "Valuables/04 Big/Valuable Manor Vase Big", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Vase (Chunky)", "Valuables/04 Big/Valuable Manor Vase Chunky", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Bottle", "Valuables/03 Medium/Valuable Manor Bottle", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Trophy", "Valuables/03 Medium/Valuable Manor Trophy", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Television", "Valuables/04 Big/Valuable Manor Television", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Piano", "Valuables/05 Wide/Valuable Manor Piano", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Dinosaur", "Valuables/05 Wide/Valuable Manor Dinosaur", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Harp", "Valuables/06 Tall/Valuable Manor Harp", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Coffin", "Valuables/07 Very Tall/Valuable Manor Coffin", ItemCategory::Valuables });
    m_Items.push_back({ "Manor Golden Statue", "Valuables/07 Very Tall/Valuable Manor Golden Statue", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Car", "Valuables/02 Small/Valuable Museum Car", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Plane", "Valuables/02 Small/Valuable Museum Plane", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Cubic Sculpture", "Valuables/03 Medium/Valuable Museum Cubic Sculpture", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Gumball", "Valuables/03 Medium/Valuable Museum Gumball", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Handface", "Valuables/03 Medium/Valuable Museum Handface", ItemCategory::Valuables });
    m_Items.push_back({ "Museum MonkeyBox", "Valuables/03 Medium/Valuable Museum MonkeyBox", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Baby Head", "Valuables/04 Big/Valuable Museum Baby Head", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Egg", "Valuables/04 Big/Valuable Museum Egg", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Uranium Mug Deluxe", "Valuables/04 Big/Valuable Museum Uranium Mug Deluxe", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Horse", "Valuables/05 Wide/Valuable Museum Horse", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Worm", "Valuables/05 Wide/Valuable Museum Worm", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Milk", "Valuables/06 Tall/Valuable Museum Milk", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Tall Guy", "Valuables/06 Tall/Valuable Museum Tall Guy", ItemCategory::Valuables });
    m_Items.push_back({ "Museum Blender", "Valuables/07 Very Tall/Valuable Museum Blender", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Fan", "Valuables/03 Medium/Valuable Arctic Fan", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Propane Tank", "Valuables/03 Medium/Valuable Arctic Propane Tank", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Sample", "Valuables/03 Medium/Valuable Arctic Sample", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Sample Six Pack", "Valuables/03 Medium/Valuable Arctic Sample Six Pack", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Barrel", "Valuables/04 Big/Valuable Arctic Barrel", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Guitar", "Valuables/04 Big/Valuable Arctic Guitar", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Sample Cooler", "Valuables/04 Big/Valuable Arctic Sample Cooler", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Ice Block", "Valuables/05 Wide/Valuable Arctic Ice Block", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Snow Bike", "Valuables/05 Wide/Valuable Arctic Snow Bike", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Heavy Water", "Valuables/06 Tall/Valuable Arctic Heavy Water", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Science Station", "Valuables/06 Tall/Valuable Arctic Science Station", ItemCategory::Valuables });
    m_Items.push_back({ "Arctic Server Rack", "Valuables/07 Very Tall/Valuable Arctic Server Rack", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Levitation Potion", "Valuables/02 Small/Valuable Wizard Levitation Potion", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Love Potion", "Valuables/02 Small/Valuable Wizard Love Potion", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Crystal", "Valuables/03 Medium/Valuable Wizard Crystal", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Cube of Knowledge", "Valuables/04 Big/Valuable Wizard Cube of Knowledge", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Master Potion", "Valuables/04 Big/Valuable Wizard Master Potion", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Spider Potion", "Valuables/04 Big/Valuable Wizard Spider Potion", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Alchemy Station", "Valuables/05 Wide/Valuable Wizard Alchemy Station", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Griffin Statue", "Valuables/05 Wide/Valuable Wizard Griffin Statue", ItemCategory::Valuables });
    m_Items.push_back({ "Wizard Dumgolfs Staff", "Valuables/06 Tall/Valuable Wizard Dumgolfs Staff", ItemCategory::Valuables });
    m_Items.push_back({ "Desert Explosive Tower", "Valuables/07 Very Tall/Valuable Desert Explosive Tower - Dynamite", ItemCategory::Valuables });
}

void Spawner::ScanRuntimeItems() {
    auto& bridge = Mono::Bridge::Get();
    if (!bridge.IsReady() || !bridge.GetAppImage()) return;

    MonoClass* klassStatsManager = bridge.FindClass(bridge.GetAppImage(), "", "StatsManager");
    if (!klassStatsManager) return;

    uintptr_t statsMgr = reinterpret_cast<uintptr_t>(bridge.GetStaticFieldValue(klassStatsManager, "instance"));
    if (!statsMgr || !Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(statsMgr))) return;

    MonoMethod* methodGetItemName = bridge.FindMethod(klassStatsManager, "GetItemNameFromIndexInItemDictionary", 1);
    if (!methodGetItemName) return;

    for (int i = 0; i < 500; ++i) {
        void* args[1] = { &i };
        MonoObject* exc = nullptr;
        MonoObject* nameObj = bridge.InvokeMethod(methodGetItemName, reinterpret_cast<void*>(statsMgr), args, &exc);
        if (!nameObj || exc) break;

        std::string rawName = bridge.ReadMonoString(reinterpret_cast<MonoString*>(nameObj));
        if (rawName.empty()) break;

        std::string cleanName = rawName;
        if (cleanName.find("Item ") == 0) {
            cleanName = cleanName.substr(5);
        }

        std::string resPath = "Items/" + rawName;

        bool alreadyExists = false;
        for (const auto& existing : m_Items) {
            if (existing.resourcePath == resPath || existing.name == cleanName) {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists) {
            ItemCategory cat = ItemCategory::Utility;
            std::string lower = cleanName;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("gun") != std::string::npos || lower.find("grenade") != std::string::npos ||
                lower.find("mine") != std::string::npos || lower.find("melee") != std::string::npos ||
                lower.find("bat") != std::string::npos || lower.find("hammer") != std::string::npos) {
                cat = ItemCategory::Weapons;
            } else if (lower.find("upgrade") != std::string::npos) {
                cat = ItemCategory::Upgrades;
            } else if (lower.find("health") != std::string::npos || lower.find("revive") != std::string::npos ||
                       lower.find("crystal") != std::string::npos) {
                cat = ItemCategory::Survival;
            } else if (lower.find("valuable") != std::string::npos) {
                cat = ItemCategory::Valuables;
            }
            m_Items.push_back({ cleanName, resPath, cat });
        }
    }
}

std::vector<int> Spawner::GetFilteredIndices(ItemCategory category, const std::string& filter) const {
    std::vector<int> result;
    std::string lowerFilter = filter;
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

    for (int i = 0; i < static_cast<int>(m_Items.size()); ++i) {
        const auto& item = m_Items[i];
        if (category != ItemCategory::All && item.category != category) {
            continue;
        }

        if (!lowerFilter.empty()) {
            std::string lowerName = item.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(lowerFilter) == std::string::npos) {
                continue;
            }
        }

        result.push_back(i);
    }
    return result;
}

const ItemEntry* Spawner::GetItem(int index) const {
    if (index >= 0 && index < static_cast<int>(m_Items.size())) {
        return &m_Items[index];
    }
    return nullptr;
}

bool Spawner::QueueSpawn(int itemIndex, int amount, bool atCrosshair, float distance, int mode) {
    const ItemEntry* item = GetItem(itemIndex);
    if (!item) {
        SetStatus("Error: Invalid item index");
        return false;
    }

    SpawnRequest req;
    req.itemIndex = itemIndex;
    req.resourcePath = item->resourcePath;
    req.displayName = item->name;
    req.amount = (std::max)(1, (std::min)(amount, 20));
    req.atCrosshair = atCrosshair;
    req.distance = (std::max)(0.5f, distance);
    req.mode = mode;
    req.isAll = false;

    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_Queue.push_back(req);
    }

    SetStatus("Queued " + std::to_string(req.amount) + "x " + req.displayName);
    return true;
}

bool Spawner::QueueSpawnAll(bool atCrosshair, float distance, int mode) {
    SpawnRequest req;
    req.isAll = true;
    req.atCrosshair = atCrosshair;
    req.distance = (std::max)(1.5f, distance);
    req.mode = mode;
    auto& cfg = Core::Config::Get();
    req.allCategory = static_cast<ItemCategory>(cfg.spawner.selectedCategory);

    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_Queue.push_back(req);
    }

    SetStatus("Queued spawn all in category");
    return true;
}

bool Spawner::TakeStatus(std::string& outStatus) {
    std::lock_guard<std::mutex> lock(m_StatusMutex);
    if (m_HasNewStatus) {
        outStatus = m_Status;
        m_HasNewStatus = false;
        return true;
    }
    return false;
}

void Spawner::SetStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(m_StatusMutex);
    m_Status = status;
    m_HasNewStatus = true;
    auto& cfg = Core::Config::Get();
    cfg.spawner.hasStatus = true;
    strncpy_s(cfg.spawner.statusMessage, status.c_str(), sizeof(cfg.spawner.statusMessage) - 1);
}

void Spawner::Update(float dt) {
    m_ScanTimer += dt;
    if (m_ScanTimer >= 3.0f) {
        m_ScanTimer = 0.0f;
        ScanRuntimeItems();
    }

    auto& cfg = Core::Config::Get();
    if (cfg.spawner.hotkey > 0) {
        bool isDown = Hook::InputHook::IsKeyHeld(cfg.spawner.hotkey);
        bool triggered = Core::EvaluateKeybind(cfg.spawner.hotkeyMode, isDown, m_PrevHotkeyDown, m_HotkeyActive);
        m_PrevHotkeyDown = isDown;

        if (triggered) {
            QueueSpawn(cfg.spawner.selectedItemIndex, cfg.spawner.spawnAmount,
                       cfg.spawner.spawnAtCrosshair, cfg.spawner.spawnDistance, cfg.spawner.spawnMode);
        }
    } else {
        m_PrevHotkeyDown = false;
        m_HotkeyActive = false;
    }

    std::deque<SpawnRequest> toProcess;
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        toProcess.swap(m_Queue);
    }

    while (!toProcess.empty()) {
        SpawnRequest req = toProcess.front();
        toProcess.pop_front();
        ExecuteSpawnRequest(req);
    }
}

bool Spawner::ExecuteSpawnRequest(const SpawnRequest& req) {
    auto& gameState = Game::GameState::Get();
    if (!gameState.IsReady() || gameState.IsMapLoadingOrRestarting()) {
        SetStatus("Failed: Game is loading or not ready");
        return false;
    }

    Unity::Vector3 basePos{};
    Unity::Vector3 camFwd{ 0.0f, 0.0f, 1.0f };
    Unity::Vector3 camRight{ 1.0f, 0.0f, 0.0f };

    MonoObject* cam = Unity::Engine::Get().FindActiveCamera();
    MonoObject* camTrans = cam ? Unity::Engine::Get().GetTransform(cam) : nullptr;
    if (camTrans) {
        camFwd = Unity::Engine::Get().GetTransformForward(camTrans);
        camRight = Unity::Engine::Get().GetTransformRight(camTrans);
    }

    if (req.atCrosshair && camTrans) {
        Unity::Vector3 camPos = Unity::Engine::Get().GetTransformPosition(camTrans);
        Unity::Vector3 hitPoint{};
        if (Unity::Engine::Get().Linecast(camPos, camPos + camFwd * 50.0f, hitPoint)) {
            basePos = hitPoint + Unity::Vector3{ 0.0f, 0.15f, 0.0f };
        } else {
            basePos = camPos + camFwd * req.distance;
        }
    } else {
        uintptr_t playerCtrl = reinterpret_cast<uintptr_t>(
            Mono::Bridge::Get().GetStaticFieldValue(gameState.klassPlayerController, "instance")
        );
        if (playerCtrl && Unity::Engine::IsValidUnityObject(reinterpret_cast<void*>(playerCtrl))) {
            MonoObject* pTrans = Unity::Engine::Get().GetTransform(reinterpret_cast<MonoObject*>(playerCtrl));
            if (pTrans) {
                basePos = Unity::Engine::Get().GetTransformPosition(pTrans);
            }
        }

        if (!basePos.IsValid() && camTrans) {
            basePos = Unity::Engine::Get().GetTransformPosition(camTrans);
        }

        Unity::Vector3 flatFwd = camFwd;
        flatFwd.y = 0.0f;
        flatFwd = flatFwd.Normalized();
        basePos = basePos + flatFwd * req.distance + Unity::Vector3{ 0.0f, 0.3f, 0.0f };
    }

    if (!basePos.IsValid()) {
        SetStatus("Failed: Cannot determine player/camera position");
        return false;
    }

    Unity::Quaternion rot = Unity::Quaternion::Identity();

    if (req.isAll) {
        std::vector<int> indices = GetFilteredIndices(req.allCategory, "");
        if (indices.empty()) {
            SetStatus("Spawn all: No items match selected category");
            return false;
        }

        int count = static_cast<int>(indices.size());
        float radius = (std::max)(1.5f, count * 0.18f);
        int spawnedCount = 0;

        for (int i = 0; i < count; ++i) {
            float angle = (2.0f * 3.14159265f * i) / count;
            Unity::Vector3 offset{ std::cos(angle) * radius, 0.1f, std::sin(angle) * radius };
            const ItemEntry* entry = GetItem(indices[i]);
            if (entry) {
                if (gameState.SpawnItemInternal(entry->resourcePath, basePos + offset, rot, req.mode)) {
                    spawnedCount++;
                }
            }
        }

        std::string modeStr = (req.mode == 1 || (!gameState.IsMasterClient() && gameState.IsMultiplayer())) ? "Client Network (Non-Host)" : "Network/Local";
        SetStatus("Spawned " + std::to_string(spawnedCount) + "/" + std::to_string(count) + " items (" + modeStr + ")");
        return spawnedCount > 0;
    }

    int successCount = 0;
    for (int i = 0; i < req.amount; ++i) {
        Unity::Vector3 spread{};
        if (req.amount > 1) {
            float t = static_cast<float>(i) - (req.amount - 1) * 0.5f;
            spread = camRight * (t * 0.35f) + Unity::Vector3{ 0.0f, i * 0.08f, 0.0f };
        }

        if (gameState.SpawnItemInternal(req.resourcePath, basePos + spread, rot, req.mode)) {
            successCount++;
        }
    }

    if (successCount > 0) {
        bool isMulti = gameState.IsMultiplayer();
        bool isMaster = gameState.IsMasterClient();
        std::string modeDesc;
        if (req.mode == 1 || (req.mode == 0 && isMulti && !isMaster)) {
            modeDesc = "Client Network (Visible to ALL - Non-Host)";
        } else if (req.mode == 2 || (req.mode == 0 && isMulti && isMaster)) {
            modeDesc = "Room Object (Host)";
        } else {
            modeDesc = "Local";
        }

        SetStatus("Spawned " + std::to_string(successCount) + "x " + req.displayName + " [" + modeDesc + "]");
        return true;
    } else {
        SetStatus("Failed to spawn " + req.displayName);
        return false;
    }
}

}
