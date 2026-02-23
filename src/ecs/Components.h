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

// 武器元件：描述槍枝屬性
// bulletTextureID：子彈使用的紋理（存 ID 而非指標，熱重載安全）
// bulletSpeed：子彈飛行速度（像素/秒）
// fireRate：兩次射擊之間的最短間隔（秒）= 1/射速
// cooldown：目前的冷卻剩餘時間（每幀由 WeaponSystem 遞減）
// bulletLifetime：子彈飛行幾秒後消失
struct Weapon {
    uint32_t bulletTextureID = 0;
    float bulletSpeed   = 900.0f;
    float fireRate      = 0.1f;   // 0.1s = 10 發/秒
    float cooldown      = 0.0f;
    float bulletLifetime = 2.0f;
    float bulletSize    = 10.0f;
};

// 子彈元件：子彈自己帶速度而非依賴 RigidBody
// 為什麼不用 RigidBody？子彈不需要摩擦力，應該直線等速飛行
// 用獨立元件讓 WeaponSystem 只需要 view<Transform, Bullet>，
// 不會誤處理到有 RigidBody 的玩家/敵人
struct Bullet {
    float vx = 0.0f;
    float vy = 0.0f;
    float lifetime = 2.0f;  // 剩餘存活時間（秒）
    float radius   = 5.0f;  // 子彈碰撞半徑（像素）
};

// 碰撞元件：描述實體的碰撞形狀
// Circle：角色/圓柱障礙，旋轉不影響形狀，計算最快
// AABB：軸對齊矩形，最適合方形牆壁/箱子
// 注意：因含 enum class，Collider 不是嚴格 POD，不可直接 memcpy 序列化。
struct Collider {
    enum class Type { Circle, AABB };
    Type type = Type::Circle;

    float halfW  = 16.0f;  // AABB 半寬（中心到右邊緣）
    float halfH  = 16.0f;  // AABB 半高（中心到下邊緣）
    float radius = 16.0f;  // Circle 半徑

    // isSolid=true：碰到後會被推開（牆壁、玩家、箱子）
    // isSolid=false：穿透觸發（未來 Trigger 區域用）
    bool isSolid = true;
};

} // namespace duck
