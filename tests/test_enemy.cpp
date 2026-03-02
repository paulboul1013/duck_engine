#include "systems/EnemySystem.h"
#include "ecs/Components.h"
#include "ecs/Registry.h"
#include <cassert>
#include <cstdio>

static void test_enemy_idle_to_chase() {
    duck::Registry reg;

    auto player = reg.create();
    reg.addComponent<duck::Transform>(player, 100.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::InputControlled>(player);

    auto enemy = reg.create();
    reg.addComponent<duck::Transform>(enemy, 220.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(enemy, 0.0f, 0.0f, 1.0f, 0.9f);
    duck::Enemy enemyData;
    enemyData.detectRange = 160.0f;
    reg.addComponent<duck::Enemy>(enemy, enemyData);

    duck::EnemySystem system;
    system.update(reg, 1.0f / 60.0f);

    assert(reg.getComponent<duck::Enemy>(enemy).state == duck::Enemy::State::Chase);
    std::printf("  [PASS] test_enemy_idle_to_chase\n");
}

static void test_enemy_chase_to_attack() {
    duck::Registry reg;

    auto player = reg.create();
    reg.addComponent<duck::Transform>(player, 100.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::InputControlled>(player);

    auto enemy = reg.create();
    reg.addComponent<duck::Transform>(enemy, 130.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(enemy, 0.0f, 0.0f, 1.0f, 0.9f);
    duck::Enemy enemyData;
    enemyData.state = duck::Enemy::State::Chase;
    enemyData.detectRange = 200.0f;
    enemyData.attackRange = 48.0f;
    reg.addComponent<duck::Enemy>(enemy, enemyData);

    duck::EnemySystem system;
    system.update(reg, 1.0f / 60.0f);

    assert(reg.getComponent<duck::Enemy>(enemy).state == duck::Enemy::State::Attack);
    std::printf("  [PASS] test_enemy_chase_to_attack\n");
}

static void test_enemy_lost_player_to_patrol() {
    duck::Registry reg;

    auto player = reg.create();
    reg.addComponent<duck::Transform>(player, 600.0f, 600.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::InputControlled>(player);

    auto enemy = reg.create();
    reg.addComponent<duck::Transform>(enemy, 100.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(enemy, 0.0f, 0.0f, 1.0f, 0.9f);
    duck::Enemy enemyData;
    enemyData.state = duck::Enemy::State::Chase;
    enemyData.detectRange = 120.0f;
    enemyData.loseSightDelay = 0.1f;
    enemyData.loseSightTimer = 0.05f;
    reg.addComponent<duck::Enemy>(enemy, enemyData);

    duck::EnemySystem system;
    system.update(reg, 1.0f / 60.0f);
    system.update(reg, 1.0f / 60.0f);
    system.update(reg, 1.0f / 60.0f);
    system.update(reg, 1.0f / 60.0f);

    assert(reg.getComponent<duck::Enemy>(enemy).state == duck::Enemy::State::Patrol);
    std::printf("  [PASS] test_enemy_lost_player_to_patrol\n");
}

static void test_enemy_dead_state_destroys_entity() {
    duck::Registry reg;

    auto player = reg.create();
    reg.addComponent<duck::Transform>(player, 100.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::InputControlled>(player);

    auto enemy = reg.create();
    reg.addComponent<duck::Transform>(enemy, 130.0f, 100.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(enemy, 0.0f, 0.0f, 1.0f, 0.9f);
    reg.addComponent<duck::Health>(enemy, 0.0f, 3.0f);
    reg.addComponent<duck::Collider>(enemy, duck::Collider::Type::Circle, 16.0f, 16.0f, 16.0f, true);
    reg.addComponent<duck::Sprite>(enemy, 1u, 32.0f, 32.0f, 4, 1.0f, 1.0f, 1.0f, 1.0f);
    duck::Enemy enemyData;
    enemyData.deadLifetime = 0.03f;
    enemyData.deadTimer = 0.03f;
    reg.addComponent<duck::Enemy>(enemy, enemyData);

    duck::EnemySystem system;
    system.update(reg, 1.0f / 60.0f);
    assert(reg.alive(enemy));
    assert(reg.getComponent<duck::Enemy>(enemy).state == duck::Enemy::State::Dead);

    system.update(reg, 1.0f / 60.0f);
    system.update(reg, 1.0f / 60.0f);
    assert(!reg.alive(enemy));
    std::printf("  [PASS] test_enemy_dead_state_destroys_entity\n");
}

int main() {
    std::printf("=== Enemy AI Tests ===\n");
    test_enemy_idle_to_chase();
    test_enemy_chase_to_attack();
    test_enemy_lost_player_to_patrol();
    test_enemy_dead_state_destroys_entity();
    std::printf("\n=== All tests passed! ===\n");
    return 0;
}
