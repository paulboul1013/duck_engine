#pragma once
#include "ecs/Registry.h"
#include "platform/Input.h"

namespace duck {

// ============================================================
// MovementSystem — 移動與物理更新
// ============================================================
// 職責：
// 1. 讀取玩家輸入，轉換成速度
// 2. 套用摩擦力讓角色自然減速
// 3. 依速度更新位置
// 4. 根據滑鼠位置更新朝向角度
//
// 為什麼 System 是一個獨立的類別而不是 free function？
// - 未來可能需要儲存狀態（例如：記錄上一幀的速度計算加速度）
// - 統一的介面讓 Engine 可以用相同方式管理所有 System
// - 方便替換：想換掉移動邏輯只需換掉這個 class
//
// 為什麼 update() 接受 const Input&？
// System 只「讀取」輸入狀態，不應該修改它
// const& 讓編譯器幫我們強制這個規則
//
class MovementSystem {
public:
    void update(Registry& registry, const Input& input, float dt);
};

} // namespace duck
