#pragma once
#include <cstdint>

namespace duck {

// ============================================================
// Phase 1 元件 — 全部都是純資料結構（POD / Plain Old Data）
// ============================================================
// ECS 設計原則：元件 = 純資料，不含任何邏輯（方法）
// 所有行為由 System 實作，元件只負責儲存狀態
//
// 為什麼？
// 1. 純資料結構可以用 memcpy 複製、序列化到磁碟、透過網路傳輸
// 2. 沒有虛擬函式表（vtable），sizeof 就是欄位大小，更 cache-friendly
// 3. 邏輯集中在 System，容易測試和替換

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;  // 弧度（radians），不是角度
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct Sprite {
    uint32_t textureID = 0;   // 對應 Renderer 的紋理 ID
    float width = 0.0f;
    float height = 0.0f;
    int zOrder = 0;           // 繪製層級：0=地面, 4=角色, 7=UI
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;  // 色彩調變
};

struct RigidBody {
    float vx = 0.0f;         // X 軸速度（像素/秒）
    float vy = 0.0f;         // Y 軸速度
    float mass = 1.0f;       // 質量（目前未用，Phase 2 碰撞用）
    float friction = 0.9f;   // 摩擦力係數：每幀速度乘以此值
                              // 0.9 = 快速減速, 0.99 = 冰面滑行
};

// 標記元件（Tag Component）：沒有資料，只用來「標記」entity
// 例如：只有玩家有 InputControlled，AI 敵人沒有
// System 透過 view<Transform, RigidBody, InputControlled>
// 就能精確篩選出「受玩家控制的、有物理屬性的實體」
struct InputControlled {};

} // namespace duck
