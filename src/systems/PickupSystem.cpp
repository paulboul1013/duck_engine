#include "systems/PickupSystem.h"
#include "ecs/Components.h"
#include <vector>

namespace duck {

void PickupSystem::update(Registry& registry) {
    EntityID player = INVALID_ENTITY;
    float playerX = 0.0f;
    float playerY = 0.0f;

    registry.view<Transform, Inventory, InputControlled>([&](EntityID entity) {
        if (player != INVALID_ENTITY) return;
        auto& tf = registry.getComponent<Transform>(entity);
        player = entity;
        playerX = tf.x;
        playerY = tf.y;
    });

    if (player == INVALID_ENTITY) return;

    auto& inventory = registry.getComponent<Inventory>(player);
    std::vector<EntityID> toDestroy;

    registry.view<Transform, Item>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& item = registry.getComponent<Item>(entity);

        float dx = tf.x - playerX;
        float dy = tf.y - playerY;
        float distSq = dx * dx + dy * dy;
        if (distSq > item.pickupRadius * item.pickupRadius) return;

        switch (item.type) {
            case Item::Type::DuckCoin:
                inventory.duckCoins += item.amount;
                break;
            case Item::Type::Ammo:
                inventory.ammo += item.amount;
                break;
            case Item::Type::Medkit:
                inventory.medkits += item.amount;
                break;
        }
        inventory.totalPickups += item.amount;
        toDestroy.push_back(entity);
    });

    for (EntityID entity : toDestroy) {
        if (registry.alive(entity)) registry.destroy(entity);
    }
}

} // namespace duck
