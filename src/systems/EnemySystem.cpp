#include "systems/EnemySystem.h"
#include "ecs/Components.h"
#include <cmath>
#include <vector>

namespace duck {

static float sqr(float v) {
    return v * v;
}

static void accelerateTowards(RigidBody& rb,
                              Transform& tf,
                              float targetX,
                              float targetY,
                              float acceleration,
                              float dt) {
    float dx = targetX - tf.x;
    float dy = targetY - tf.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.001f) return;

    dx /= len;
    dy /= len;
    rb.vx += dx * acceleration * dt;
    rb.vy += dy * acceleration * dt;
    tf.rotation = std::atan2(dy, dx);
}

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

    std::vector<EntityID> toDestroy;

    registry.view<Transform, RigidBody, Enemy>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& rb = registry.getComponent<RigidBody>(entity);
        auto& enemy = registry.getComponent<Enemy>(entity);

        if (!enemy.homeInitialized) {
            enemy.homeX = tf.x;
            enemy.homeY = tf.y;
            enemy.homeInitialized = true;
        }

        if (enemy.touchCooldown > 0.0f) {
            enemy.touchCooldown -= dt;
            if (enemy.touchCooldown < 0.0f) enemy.touchCooldown = 0.0f;
        }

        auto* sprite = registry.hasComponent<Sprite>(entity)
            ? &registry.getComponent<Sprite>(entity)
            : nullptr;
        auto* health = registry.hasComponent<Health>(entity)
            ? &registry.getComponent<Health>(entity)
            : nullptr;

        if (health && health->currentHP <= 0.0f && enemy.state != Enemy::State::Dead) {
            enemy.state = Enemy::State::Dead;
            enemy.deadTimer = enemy.deadLifetime;
            rb.vx = 0.0f;
            rb.vy = 0.0f;
            if (registry.hasComponent<Collider>(entity)) {
                registry.getComponent<Collider>(entity).isSolid = false;
            }
            if (sprite) {
                sprite->r = 0.35f;
                sprite->g = 0.35f;
                sprite->b = 0.38f;
                sprite->a = 0.85f;
            }
        }

        if (enemy.state == Enemy::State::Dead) {
            enemy.deadTimer -= dt;
            rb.vx = 0.0f;
            rb.vy = 0.0f;
            if (enemy.deadTimer <= 0.0f) {
                toDestroy.push_back(entity);
            }
            return;
        }

        if (player == INVALID_ENTITY) {
            enemy.state = Enemy::State::Idle;
            return;
        }

        float dx = playerX - tf.x;
        float dy = playerY - tf.y;
        float distSq = dx * dx + dy * dy;
        float detectSq = sqr(enemy.detectRange);
        float attackSq = sqr(enemy.attackRange);
        bool seesPlayer = distSq <= detectSq;
        bool inAttackRange = distSq <= attackSq;

        switch (enemy.state) {
            case Enemy::State::Idle: {
                rb.vx = 0.0f;
                rb.vy = 0.0f;
                if (sprite) {
                    sprite->r = 0.78f; sprite->g = 0.30f; sprite->b = 0.30f;
                }
                enemy.loseSightTimer = enemy.loseSightDelay;
                if (seesPlayer) {
                    enemy.state = inAttackRange ? Enemy::State::Attack : Enemy::State::Chase;
                }
                break;
            }

            case Enemy::State::Chase: {
                if (sprite) {
                    sprite->r = 0.95f; sprite->g = 0.20f; sprite->b = 0.20f;
                }
                if (inAttackRange) {
                    enemy.state = Enemy::State::Attack;
                    rb.vx = 0.0f;
                    rb.vy = 0.0f;
                    break;
                }

                if (seesPlayer) {
                    enemy.loseSightTimer = enemy.loseSightDelay;
                    accelerateTowards(rb, tf, playerX, playerY, enemy.moveAcceleration, dt);
                } else {
                    enemy.loseSightTimer -= dt;
                    if (enemy.loseSightTimer <= 0.0f) {
                        enemy.state = Enemy::State::Patrol;
                        enemy.patrolWaitTimer = 0.0f;
                    }
                }
                break;
            }

            case Enemy::State::Attack: {
                if (sprite) {
                    sprite->r = 1.0f; sprite->g = 0.10f; sprite->b = 0.10f;
                }
                tf.rotation = std::atan2(dy, dx);
                rb.vx = 0.0f;
                rb.vy = 0.0f;

                if (!inAttackRange) {
                    enemy.state = seesPlayer ? Enemy::State::Chase : Enemy::State::Patrol;
                    enemy.loseSightTimer = enemy.loseSightDelay;
                }
                break;
            }

            case Enemy::State::Patrol: {
                if (sprite) {
                    sprite->r = 0.88f; sprite->g = 0.42f; sprite->b = 0.22f;
                }
                if (seesPlayer) {
                    enemy.state = inAttackRange ? Enemy::State::Attack : Enemy::State::Chase;
                    enemy.loseSightTimer = enemy.loseSightDelay;
                    break;
                }

                enemy.patrolWaitTimer -= dt;
                float targetX = enemy.homeX + std::cos(enemy.patrolAngle) * enemy.patrolRadius;
                float targetY = enemy.homeY + std::sin(enemy.patrolAngle) * enemy.patrolRadius;
                float targetDx = targetX - tf.x;
                float targetDy = targetY - tf.y;
                float targetDistSq = targetDx * targetDx + targetDy * targetDy;

                if (enemy.patrolWaitTimer <= 0.0f && targetDistSq < sqr(18.0f)) {
                    enemy.patrolAngle += 1.65f;
                    enemy.patrolWaitTimer = enemy.patrolWaitDuration;
                }

                if (enemy.patrolWaitTimer <= 0.0f) {
                    accelerateTowards(rb, tf, targetX, targetY, enemy.moveAcceleration * 0.45f, dt);
                } else {
                    rb.vx *= 0.75f;
                    rb.vy *= 0.75f;
                }
                break;
            }

            case Enemy::State::Dead:
                break;
        }
    });

    for (EntityID entity : toDestroy) {
        if (registry.alive(entity)) registry.destroy(entity);
    }
}

} // namespace duck
