#include "systems/CollisionSystem.h"
#include "ecs/Components.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

namespace duck {

namespace {

struct Bounds {
    float x = 0.0f;
    float y = 0.0f;
    float halfW = 0.0f;
    float halfH = 0.0f;
};

struct SpatialEntry {
    EntityID entity = INVALID_ENTITY;
    Bounds bounds;
};

static Bounds computeBounds(const Transform& tf, const Collider& col) {
    if (col.type == Collider::Type::Circle) {
        return {tf.x, tf.y, col.radius, col.radius};
    }
    return {tf.x, tf.y, col.halfW, col.halfH};
}

static Bounds computeBulletBounds(const Transform& tf, const Bullet& bullet) {
    return {tf.x, tf.y, bullet.radius, bullet.radius};
}

static bool boundsIntersect(const Bounds& a, const Bounds& b) {
    return std::abs(a.x - b.x) <= (a.halfW + b.halfW)
        && std::abs(a.y - b.y) <= (a.halfH + b.halfH);
}

static bool boundsContains(const Bounds& outer, const Bounds& inner) {
    return (inner.x - inner.halfW) >= (outer.x - outer.halfW)
        && (inner.x + inner.halfW) <= (outer.x + outer.halfW)
        && (inner.y - inner.halfH) >= (outer.y - outer.halfH)
        && (inner.y + inner.halfH) <= (outer.y + outer.halfH);
}

class Quadtree {
public:
    explicit Quadtree(const Bounds& worldBounds)
        : m_root(std::make_unique<Node>(worldBounds, 0)) {}

    void insert(const SpatialEntry& entry) {
        insert(*m_root, entry);
    }

    void query(const Bounds& area, std::vector<EntityID>& outEntities) const {
        query(*m_root, area, outEntities);
    }

private:
    struct Node {
        explicit Node(const Bounds& nodeBounds, int nodeDepth)
            : bounds(nodeBounds), depth(nodeDepth) {}

        Bounds bounds;
        int depth = 0;
        std::vector<SpatialEntry> entries;
        std::array<std::unique_ptr<Node>, 4> children;
    };

    static constexpr int MAX_DEPTH = 5;
    static constexpr int NODE_CAPACITY = 6;

    static Bounds childBounds(const Bounds& parent, int index) {
        float childHalfW = parent.halfW * 0.5f;
        float childHalfH = parent.halfH * 0.5f;
        float childX = parent.x + ((index % 2 == 0) ? -childHalfW : childHalfW);
        float childY = parent.y + ((index < 2) ? -childHalfH : childHalfH);
        return {childX, childY, childHalfW, childHalfH};
    }

    static int findContainingChild(const Node& node, const Bounds& bounds) {
        for (int i = 0; i < 4; ++i) {
            if (!node.children[i]) continue;
            if (boundsContains(node.children[i]->bounds, bounds)) return i;
        }
        return -1;
    }

    static void subdivide(Node& node) {
        for (int i = 0; i < 4; ++i) {
            node.children[i] = std::make_unique<Node>(childBounds(node.bounds, i), node.depth + 1);
        }
    }

    static void insert(Node& node, const SpatialEntry& entry) {
        if (node.depth < MAX_DEPTH && node.entries.size() >= NODE_CAPACITY && !node.children[0]) {
            subdivide(node);

            std::vector<SpatialEntry> retained;
            retained.reserve(node.entries.size());
            for (const auto& existing : node.entries) {
                int childIndex = findContainingChild(node, existing.bounds);
                if (childIndex >= 0) {
                    insert(*node.children[childIndex], existing);
                } else {
                    retained.push_back(existing);
                }
            }
            node.entries = std::move(retained);
        }

        if (node.children[0]) {
            int childIndex = findContainingChild(node, entry.bounds);
            if (childIndex >= 0) {
                insert(*node.children[childIndex], entry);
                return;
            }
        }

        node.entries.push_back(entry);
    }

    static void query(const Node& node, const Bounds& area, std::vector<EntityID>& outEntities) {
        if (!boundsIntersect(node.bounds, area)) return;

        for (const auto& entry : node.entries) {
            if (boundsIntersect(entry.bounds, area)) {
                outEntities.push_back(entry.entity);
            }
        }

        for (const auto& child : node.children) {
            if (child) query(*child, area, outEntities);
        }
    }

    std::unique_ptr<Node> m_root;
};

static Bounds computeWorldBounds(const std::vector<SpatialEntry>& entries) {
    if (entries.empty()) {
        return {640.0f, 360.0f, 640.0f, 360.0f};
    }

    float minX = entries[0].bounds.x - entries[0].bounds.halfW;
    float maxX = entries[0].bounds.x + entries[0].bounds.halfW;
    float minY = entries[0].bounds.y - entries[0].bounds.halfH;
    float maxY = entries[0].bounds.y + entries[0].bounds.halfH;

    for (const auto& entry : entries) {
        minX = std::min(minX, entry.bounds.x - entry.bounds.halfW);
        maxX = std::max(maxX, entry.bounds.x + entry.bounds.halfW);
        minY = std::min(minY, entry.bounds.y - entry.bounds.halfH);
        maxY = std::max(maxY, entry.bounds.y + entry.bounds.halfH);
    }

    float padding = 32.0f;
    float halfW = std::max(64.0f, (maxX - minX) * 0.5f + padding);
    float halfH = std::max(64.0f, (maxY - minY) * 0.5f + padding);
    return {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f, halfW, halfH};
}

static std::uint64_t makePairKey(EntityID a, EntityID b) {
    EntityID lo = std::min(a, b);
    EntityID hi = std::max(a, b);
    return (static_cast<std::uint64_t>(lo) << 32) | static_cast<std::uint64_t>(hi);
}

} // namespace

static bool enemyCanDealTouchDamage(Registry& registry, EntityID entity) {
    if (!registry.hasComponent<Enemy>(entity)) return false;
    const auto& enemy = registry.getComponent<Enemy>(entity);
    return enemy.state != Enemy::State::Dead;
}

void CollisionSystem::update(Registry& registry, float /*dt*/) {

    // -------------------------------------------------------
    // 收集所有有 Collider 的實體，並建立 Quadtree
    // -------------------------------------------------------
    std::vector<SpatialEntry> solidEntries;
    registry.view<Transform, Collider>([&](EntityID e) {
        auto& col = registry.getComponent<Collider>(e);
        if (col.isSolid) {
            auto& tf = registry.getComponent<Transform>(e);
            solidEntries.push_back({e, computeBounds(tf, col)});
        }
    });

    Quadtree quadtree(computeWorldBounds(solidEntries));
    for (const auto& entry : solidEntries) {
        quadtree.insert(entry);
    }

    // -------------------------------------------------------
    // 1. Solid vs Solid：先用 Quadtree 收斂候選，再做精確碰撞
    // -------------------------------------------------------
    std::unordered_set<std::uint64_t> testedPairs;
    std::vector<EntityID> candidateEntities;

    for (const auto& entryA : solidEntries) {
        candidateEntities.clear();
        quadtree.query(entryA.bounds, candidateEntities);

        for (EntityID B : candidateEntities) {
            EntityID A = entryA.entity;
            if (A == B) continue;

            auto pairKey = makePairKey(A, B);
            if (!testedPairs.insert(pairKey).second) continue;

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
    // 2. Bullet vs Solid：先查 Quadtree 候選，再做精確判斷
    // -------------------------------------------------------
    std::vector<EntityID> bulletsToDestroy;
    std::vector<EntityID> entitiesToDestroy;
    std::vector<EntityID> bulletCandidates;

    registry.view<Transform, Bullet>([&](EntityID bulletID) {
        auto& btf = registry.getComponent<Transform>(bulletID);
        auto& bullet = registry.getComponent<Bullet>(bulletID);
        Bounds bulletBounds = computeBulletBounds(btf, bullet);

        bulletCandidates.clear();
        quadtree.query(bulletBounds, bulletCandidates);

        for (EntityID solidID : bulletCandidates) {
            // 跳過玩家（自己的子彈不消失在自己身上）
            if (registry.hasComponent<InputControlled>(solidID)) continue;

            auto& stf = registry.getComponent<Transform>(solidID);
            auto& scol = registry.getComponent<Collider>(solidID);

            float nx = 0.0f, ny = 0.0f, depth = 0.0f;
            bool hit = false;

            if (scol.type == Collider::Type::AABB) {
                hit = circleVsAabb(btf.x, btf.y, bullet.radius,
                                   stf.x, stf.y, scol.halfW, scol.halfH,
                                   nx, ny, depth);
            } else {
                hit = circleVsCircle(btf.x, btf.y, bullet.radius,
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
