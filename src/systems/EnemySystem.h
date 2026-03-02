#pragma once
#include "ecs/Registry.h"

namespace duck {

// ============================================================
// EnemySystem — 最小版敵人追逐
// ============================================================
// Phase 2 先只做一件事：
// 找到玩家位置，讓所有 Enemy 朝玩家加速移動。
// 狀態機、攻擊、巡邏等留到後續再拆成真正的 AISystem。
class EnemySystem {
public:
    void update(Registry& registry, float dt);
};

} // namespace duck
