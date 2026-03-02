#pragma once
#include "ecs/Registry.h"

namespace duck {

// ============================================================
// EnemySystem — 敵人狀態機
// ============================================================
// 目前支援：
// IDLE: 玩家未進入偵測範圍時待機
// CHASE: 玩家被看見後追逐
// ATTACK: 進入近距離後停住並面向玩家，接觸傷害由 CollisionSystem 處理
// PATROL: 失去目標後繞出生點附近巡邏
// DEAD: 死亡殘留短時間後銷毀
class EnemySystem {
public:
    void update(Registry& registry, float dt);
};

} // namespace duck
