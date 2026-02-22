#pragma once
#include "ecs/Registry.h"
#include "platform/Input.h"

namespace duck {

// ============================================================
// WeaponSystem — 射擊邏輯 + 子彈生命週期管理
// ============================================================
// 職責分兩個 view：
//
// View 1：<Transform, Weapon, InputControlled>
//   - 偵測左鍵按住 + 冷卻結束 → 生成 Bullet entity
//   - 子彈方向 = 從玩家位置指向滑鼠（normalized 向量）
//   - 重置 cooldown = fireRate，下次才能再射
//
// View 2：<Transform, Bullet>
//   - 每幀移動子彈（等速直線，無摩擦力）
//   - lifetime 倒數，歸零就 destroy()
//
// 為什麼 Bullet 不用 RigidBody？
// RigidBody 有 friction，子彈每幀都在減速 → 不符合物理
// 用獨立 Bullet 元件，MovementSystem 的物理 view 完全不會碰到它
//
class WeaponSystem {
public:
    void update(Registry& registry, const Input& input, float dt);
};

} // namespace duck
