#include "core/MapLoader.h"
#include "core/SimpleJson.h"
#include "ecs/Components.h"
#include <fstream>
#include <sstream>

namespace duck {

namespace {

Item::Type parseItemType(const std::string& type) {
    if (type == "ammo") return Item::Type::Ammo;
    if (type == "medkit") return Item::Type::Medkit;
    return Item::Type::DuckCoin;
}

void createPlayer(const JsonValue& playerData,
                  Registry& registry,
                  const MapSceneAssets& assets) {
    auto entity = registry.create();
    float x = playerData.value("x", 640.0f);
    float y = playerData.value("y", 360.0f);
    float hp = playerData.value("hp", 5.0f);

    registry.addComponent<Transform>(entity, x, y, 0.0f, 1.0f, 1.0f);
    registry.addComponent<Sprite>(entity, assets.playerTexID, 48.0f, 48.0f, 4,
                                  1.0f, 1.0f, 1.0f, 1.0f);
    registry.addComponent<RigidBody>(entity, 0.0f, 0.0f, 1.0f, 0.85f);
    registry.addComponent<InputControlled>(entity);
    registry.addComponent<Weapon>(entity, assets.bulletTexID, 900.0f, 0.1f, 0.0f, 2.0f, 10.0f, 1.0f);
    registry.addComponent<Collider>(entity, Collider::Type::Circle, 24.0f, 24.0f, 24.0f, true);
    registry.addComponent<Health>(entity, hp, hp);
    registry.addComponent<Inventory>(entity);
}

void createGround(const JsonValue& groundData,
                  Registry& registry,
                  const MapSceneAssets& assets) {
    auto entity = registry.create();
    float width = groundData.value("width", 1280.0f);
    float height = groundData.value("height", 720.0f);
    float x = groundData.value("x", 640.0f);
    float y = groundData.value("y", 360.0f);

    registry.addComponent<Transform>(entity, x, y, 0.0f, 1.0f, 1.0f);
    registry.addComponent<Sprite>(entity, assets.groundTexID, width, height, 0,
                                  1.0f, 1.0f, 1.0f, 1.0f);
}

void createObstacle(const JsonValue& obstacleData,
                    Registry& registry,
                    const MapSceneAssets& assets) {
    float x = obstacleData.value("x", 0.0f);
    float y = obstacleData.value("y", 0.0f);
    float width = obstacleData.value("width", 48.0f);
    float height = obstacleData.value("height", 48.0f);
    float rotation = obstacleData.value("rotation", 0.0f);

    auto entity = registry.create();
    registry.addComponent<Transform>(entity, x, y, rotation, 1.0f, 1.0f);
    registry.addComponent<Sprite>(entity, assets.obstacleTexID, width, height, 2,
                                  1.0f, 1.0f, 1.0f, 1.0f);
    registry.addComponent<Collider>(entity, Collider::Type::AABB, width * 0.5f, height * 0.5f, width * 0.5f, true);
}

void createEnemy(const JsonValue& enemyData,
                 Registry& registry,
                 const MapSceneAssets& assets) {
    float x = enemyData.value("x", 0.0f);
    float y = enemyData.value("y", 0.0f);
    float hp = enemyData.value("hp", 3.0f);

    Enemy enemy;
    enemy.detectRange = enemyData.value("detect_range", enemy.detectRange);
    enemy.attackRange = enemyData.value("attack_range", enemy.attackRange);
    enemy.moveAcceleration = enemyData.value("move_acceleration", enemy.moveAcceleration);
    enemy.patrolRadius = enemyData.value("patrol_radius", enemy.patrolRadius);

    auto entity = registry.create();
    registry.addComponent<Transform>(entity, x, y, 0.0f, 1.0f, 1.0f);
    registry.addComponent<Sprite>(entity, assets.playerTexID, 42.0f, 42.0f, 4,
                                  0.85f, 0.2f, 0.2f, 1.0f);
    registry.addComponent<RigidBody>(entity, 0.0f, 0.0f, 1.0f, 0.88f);
    registry.addComponent<Collider>(entity, Collider::Type::Circle, 20.0f, 20.0f, 20.0f, true);
    registry.addComponent<Health>(entity, hp, hp);
    registry.addComponent<Enemy>(entity, enemy);
}

void createItem(const JsonValue& itemData,
                Registry& registry,
                const MapSceneAssets& assets) {
    float x = itemData.value("x", 0.0f);
    float y = itemData.value("y", 0.0f);
    std::string typeName = itemData.value("type", std::string("duck_coin"));
    int amount = itemData.value("amount", 1);

    Item item;
    item.type = parseItemType(typeName);
    item.amount = amount;
    item.pickupRadius = itemData.value("pickup_radius", item.pickupRadius);

    uint32_t texID = assets.coinTexID;
    float width = 18.0f;
    float height = 18.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
    if (item.type == Item::Type::Ammo) {
        texID = assets.ammoTexID;
        width = 16.0f; height = 24.0f;
        r = 0.92f; g = 0.92f; b = 0.65f;
    } else if (item.type == Item::Type::Medkit) {
        texID = assets.medkitTexID;
        width = 20.0f; height = 20.0f;
        r = 0.9f; g = 1.0f; b = 0.9f;
    }

    auto entity = registry.create();
    registry.addComponent<Transform>(entity, x, y, 0.0f, 1.0f, 1.0f);
    registry.addComponent<Sprite>(entity, texID, width, height, 3, r, g, b, 1.0f);
    registry.addComponent<Item>(entity, item);
}

} // namespace

bool MapLoader::loadFromFile(const std::string& path,
                             Registry& registry,
                             const MapSceneAssets& assets) const {
    std::ifstream input(path);
    if (!input.is_open()) return false;

    std::ostringstream buffer;
    buffer << input.rdbuf();

    JsonValue json;
    try {
        JsonParser parser(buffer.str());
        json = parser.parse();
    } catch (...) {
        return false;
    }

    createPlayer(json.at("player"), registry, assets);
    if (json.contains("ground")) {
        createGround(json.at("ground"), registry, assets);
    }

    if (json.contains("obstacles")) {
        for (const auto& obstacle : json.at("obstacles").asArray()) {
            createObstacle(obstacle, registry, assets);
        }
    }

    if (json.contains("enemies")) {
        for (const auto& enemy : json.at("enemies").asArray()) {
            createEnemy(enemy, registry, assets);
        }
    }

    if (json.contains("items")) {
        for (const auto& item : json.at("items").asArray()) {
            createItem(item, registry, assets);
        }
    }

    return true;
}

} // namespace duck
