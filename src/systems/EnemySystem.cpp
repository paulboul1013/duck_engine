#include "systems/EnemySystem.h"
#include "ecs/Components.h"
#include <cmath>

namespace duck {

void EnemySystem::update(Registry& registry, float dt) {
    EntityID player = INVALID_ENTITY;
    float playerX = 0.0f;
    float playerY = 0.0f;

    registry.view<Transform, InputControlled>([&](EntityID entity) {
        if (player != INVALID_ENTITY) return;
        auto& tf = registry.getComponent<Transform>(entity);
        player = entity;
        playerX = tf.x;
        playerY = tf.y;
    });

    if (player == INVALID_ENTITY) return;

    registry.view<Transform, RigidBody, Enemy>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& rb = registry.getComponent<RigidBody>(entity);
        auto& enemy = registry.getComponent<Enemy>(entity);

        if (enemy.touchCooldown > 0.0f) {
            enemy.touchCooldown -= dt;
            if (enemy.touchCooldown < 0.0f) enemy.touchCooldown = 0.0f;
        }

        float dx = playerX - tf.x;
        float dy = playerY - tf.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len <= 0.001f) return;

        dx /= len;
        dy /= len;

        const float chaseSpeed = 1100.0f;
        rb.vx += dx * chaseSpeed * dt;
        rb.vy += dy * chaseSpeed * dt;
        tf.rotation = std::atan2(dy, dx);
    });
}

} // namespace duck
