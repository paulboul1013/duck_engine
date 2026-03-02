#include "systems/CollisionSystem.h"
#include "ecs/Components.h"
#include <vector>

namespace duck {

static bool enemyCanDealTouchDamage(Registry& registry, EntityID entity) {
    if (!registry.hasComponent<Enemy>(entity)) return false;
    const auto& enemy = registry.getComponent<Enemy>(entity);
    return enemy.state != Enemy::State::Dead;
}

void CollisionSystem::update(Registry& registry, float /*dt*/) {

    // -------------------------------------------------------
    // 收集所有有 Collider 的實體
    // -------------------------------------------------------
    std::vector<EntityID> solidEntities;
    registry.view<Transform, Collider>([&](EntityID e) {
        if (registry.getComponent<Collider>(e).isSolid) {
            solidEntities.push_back(e);
        }
    });

    // -------------------------------------------------------
    // 1. Solid vs Solid：互推（O(n²)）
    // -------------------------------------------------------
    // 兩個 Solid 實體重疊時，計算推生向量各推一半
    // 若其中一方無 RigidBody（靜態障礙物），只推動態那方
    for (size_t i = 0; i < solidEntities.size(); ++i) {
        for (size_t j = i + 1; j < solidEntities.size(); ++j) {

            EntityID A = solidEntities[i];
            EntityID B = solidEntities[j];

            auto& tfA = registry.getComponent<Transform>(A);
            auto& tfB = registry.getComponent<Transform>(B);
            auto& colA = registry.getComponent<Collider>(A);
            auto& colB = registry.getComponent<Collider>(B);

            float nx = 0.0f, ny = 0.0f, depth = 0.0f;
            bool hit = false;

            // 分派到對應幾何函式
            if (colA.type == Collider::Type::Circle && colB.type == Collider::Type::Circle) {
                hit = circleVsCircle(tfA.x, tfA.y, colA.radius,
                                     tfB.x, tfB.y, colB.radius,
                                     nx, ny, depth);
            }
            else if (colA.type == Collider::Type::AABB && colB.type == Collider::Type::AABB) {
                hit = aabbVsAabb(tfA.x, tfA.y, colA.halfW, colA.halfH,
                                 tfB.x, tfB.y, colB.halfW, colB.halfH,
                                 nx, ny, depth);
            }
            else {
                // 混合：確保 Circle 在前，AABB 在後
                if (colA.type == Collider::Type::AABB) {
                    // A 是 AABB，B 是 Circle → 呼叫 circleVsAabb(B, A)，反轉方向
                    hit = circleVsAabb(tfB.x, tfB.y, colB.radius,
                                       tfA.x, tfA.y, colA.halfW, colA.halfH,
                                       nx, ny, depth);
                    nx = -nx;
                    ny = -ny;
                } else {
                    // A 是 Circle，B 是 AABB
                    hit = circleVsAabb(tfA.x, tfA.y, colA.radius,
                                       tfB.x, tfB.y, colB.halfW, colB.halfH,
                                       nx, ny, depth);
                }
            }

            if (!hit) continue;

            // 推生：判斷哪方是靜態（無 RigidBody）
            bool aIsDynamic = registry.hasComponent<RigidBody>(A);
            bool bIsDynamic = registry.hasComponent<RigidBody>(B);

            if (aIsDynamic && bIsDynamic) {
                // 雙方各推一半
                tfA.x -= nx * depth * 0.5f;
                tfA.y -= ny * depth * 0.5f;
                tfB.x += nx * depth * 0.5f;
                tfB.y += ny * depth * 0.5f;
            } else if (aIsDynamic) {
                // 只推 A（B 是靜態）
                tfA.x -= nx * depth;
                tfA.y -= ny * depth;
            } else if (bIsDynamic) {
                // 只推 B（A 是靜態）
                tfB.x += nx * depth;
                tfB.y += ny * depth;
            }
            // 兩者都靜態：不處理

            // Enemy vs Player：接觸傷害
            bool aIsEnemy = enemyCanDealTouchDamage(registry, A);
            bool bIsEnemy = enemyCanDealTouchDamage(registry, B);
            bool aIsPlayer = registry.hasComponent<InputControlled>(A);
            bool bIsPlayer = registry.hasComponent<InputControlled>(B);

            if (aIsEnemy && bIsPlayer && registry.hasComponent<Health>(B)) {
                auto& enemy = registry.getComponent<Enemy>(A);
                if (enemy.touchCooldown <= 0.0f) {
                    auto& playerHealth = registry.getComponent<Health>(B);
                    playerHealth.currentHP -= enemy.touchDamage;
                    if (playerHealth.currentHP < 0.0f) playerHealth.currentHP = 0.0f;
                    enemy.touchCooldown = enemy.touchInterval;
                }
            } else if (bIsEnemy && aIsPlayer && registry.hasComponent<Health>(A)) {
                auto& enemy = registry.getComponent<Enemy>(B);
                if (enemy.touchCooldown <= 0.0f) {
                    auto& playerHealth = registry.getComponent<Health>(A);
                    playerHealth.currentHP -= enemy.touchDamage;
                    if (playerHealth.currentHP < 0.0f) playerHealth.currentHP = 0.0f;
                    enemy.touchCooldown = enemy.touchInterval;
                }
            }
        }
    }

    // -------------------------------------------------------
    // 2. Bullet vs Solid：子彈打到固體 → 刪除子彈
    // -------------------------------------------------------
    // 子彈不加 Collider 元件，直接用 Bullet.radius 做距離判斷
    // 跳過 InputControlled（玩家），Phase 2 不做玩家受傷
    std::vector<EntityID> bulletsToDestroy;
    std::vector<EntityID> entitiesToDestroy;

    registry.view<Transform, Bullet>([&](EntityID bulletID) {
        auto& btf = registry.getComponent<Transform>(bulletID);
        auto& bullet = registry.getComponent<Bullet>(bulletID);
        float br = bullet.radius;

        for (EntityID solidID : solidEntities) {
            // 跳過玩家（自己的子彈不消失在自己身上）
            if (registry.hasComponent<InputControlled>(solidID)) continue;

            auto& stf = registry.getComponent<Transform>(solidID);
            auto& scol = registry.getComponent<Collider>(solidID);

            float nx = 0.0f, ny = 0.0f, depth = 0.0f;
            bool hit = false;

            if (scol.type == Collider::Type::AABB) {
                hit = circleVsAabb(btf.x, btf.y, br,
                                   stf.x, stf.y, scol.halfW, scol.halfH,
                                   nx, ny, depth);
            } else {
                hit = circleVsCircle(btf.x, btf.y, br,
                                     stf.x, stf.y, scol.radius,
                                     nx, ny, depth);
            }

            if (hit) {
                bulletsToDestroy.push_back(bulletID);

                if (registry.hasComponent<Health>(solidID)) {
                    auto& health = registry.getComponent<Health>(solidID);
                    health.currentHP -= bullet.damage;
                    if (health.currentHP <= 0.0f && !registry.hasComponent<InputControlled>(solidID)
                        && !registry.hasComponent<Enemy>(solidID)) {
                        entitiesToDestroy.push_back(solidID);
                    } else if (health.currentHP < 0.0f) {
                        health.currentHP = 0.0f;
                    }
                }
                break;  // 一個子彈只需要刪一次
            }
        }
    });

    // 在 view 迴圈外統一銷毀（避免邊刪邊遍歷的 UB）
    for (EntityID e : bulletsToDestroy) {
        if (registry.alive(e)) registry.destroy(e);
    }

    for (EntityID e : entitiesToDestroy) {
        if (registry.alive(e)) registry.destroy(e);
    }
}

} // namespace duck
