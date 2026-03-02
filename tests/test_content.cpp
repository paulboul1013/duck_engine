#include "core/MapLoader.h"
#include "ecs/Components.h"
#include "ecs/Registry.h"
#include "systems/PickupSystem.h"
#include <cassert>
#include <cstdio>
#include <fstream>

static void test_map_loader_populates_registry() {
    const char* path = "/tmp/duck_engine_test_map.json";
    std::ofstream out(path);
    out <<
R"({
  "player": { "x": 100.0, "y": 120.0, "hp": 7.0 },
  "ground": { "x": 320.0, "y": 240.0, "width": 640.0, "height": 480.0 },
  "obstacles": [
    { "x": 200.0, "y": 180.0, "width": 64.0, "height": 64.0, "rotation": 0.0 }
  ],
  "enemies": [
    { "x": 400.0, "y": 220.0, "hp": 3.0, "detect_range": 280.0, "attack_range": 48.0 }
  ],
  "items": [
    { "x": 130.0, "y": 120.0, "type": "duck_coin", "amount": 5 },
    { "x": 140.0, "y": 120.0, "type": "ammo", "amount": 12 }
  ]
})";
    out.close();

    duck::Registry reg;
    duck::MapSceneAssets assets;
    assets.playerTexID = 1;
    assets.groundTexID = 2;
    assets.obstacleTexID = 3;
    assets.bulletTexID = 4;
    assets.coinTexID = 5;
    assets.ammoTexID = 6;
    assets.medkitTexID = 7;

    duck::MapLoader loader;
    assert(loader.loadFromFile(path, reg, assets));

    int players = 0, grounds = 0, obstacles = 0, enemies = 0, items = 0, inventories = 0;
    reg.view<duck::InputControlled>([&](duck::EntityID) { ++players; });
    reg.view<duck::Inventory>([&](duck::EntityID) { ++inventories; });
    reg.view<duck::Enemy>([&](duck::EntityID) { ++enemies; });
    reg.view<duck::Item>([&](duck::EntityID) { ++items; });
    reg.view<duck::Collider>([&](duck::EntityID entity) {
        auto& col = reg.getComponent<duck::Collider>(entity);
        if (reg.hasComponent<duck::Enemy>(entity)) return;
        if (reg.hasComponent<duck::InputControlled>(entity)) return;
        if (col.type == duck::Collider::Type::AABB) ++obstacles;
    });
    reg.view<duck::Sprite>([&](duck::EntityID entity) {
        if (!reg.hasComponent<duck::Collider>(entity) && !reg.hasComponent<duck::Item>(entity)
            && !reg.hasComponent<duck::InputControlled>(entity)) {
            ++grounds;
        }
    });

    assert(players == 1);
    assert(inventories == 1);
    assert(grounds == 1);
    assert(obstacles == 1);
    assert(enemies == 1);
    assert(items == 2);
    std::printf("  [PASS] test_map_loader_populates_registry\n");
}

static void test_pickup_system_collects_items() {
    duck::Registry reg;

    auto player = reg.create();
    reg.addComponent<duck::Transform>(player, 100.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::InputControlled>(player);
    reg.addComponent<duck::Inventory>(player);

    auto coin = reg.create();
    reg.addComponent<duck::Transform>(coin, 110.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::Item>(coin, duck::Item::Type::DuckCoin, 5, 20.0f);

    auto ammo = reg.create();
    reg.addComponent<duck::Transform>(ammo, 118.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::Item>(ammo, duck::Item::Type::Ammo, 12, 20.0f);

    duck::PickupSystem system;
    system.update(reg);

    auto& inventory = reg.getComponent<duck::Inventory>(player);
    assert(inventory.duckCoins == 5);
    assert(inventory.ammo == 12);
    assert(inventory.totalPickups == 17);
    assert(!reg.alive(coin));
    assert(!reg.alive(ammo));
    std::printf("  [PASS] test_pickup_system_collects_items\n");
}

int main() {
    std::printf("=== Content Tests ===\n");
    test_map_loader_populates_registry();
    test_pickup_system_collects_items();
    std::printf("\n=== All tests passed! ===\n");
    return 0;
}
