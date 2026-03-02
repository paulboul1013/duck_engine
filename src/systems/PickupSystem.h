#pragma once
#include "ecs/Registry.h"

namespace duck {

// ============================================================
// PickupSystem — 玩家靠近時自動拾取掉落物
// ============================================================
class PickupSystem {
public:
    void update(Registry& registry);
};

} // namespace duck
