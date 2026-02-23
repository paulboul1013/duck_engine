#pragma once
#include "ecs/Registry.h"
#include <cmath>

namespace duck {

// ============================================================
// CollisionSystem — O(n²) 基礎碰撞偵測
// ============================================================
// Phase 2 範圍：
//   1. Solid vs Solid：計算重疊量 → 互推開（推生）
//   2. Bullet vs Solid：子彈進入固體範圍 → 刪除子彈
//
// 為什麼子彈不用 Collider 元件？
//   子彈生命週期極短（2 秒），數量多，若加 Collider 會讓
//   Solid vs Solid 迴圈多跑很多不必要的比對。
//   CollisionSystem 單獨用 Bullet.radius 做距離判斷，更清楚。
//
// 幾何函式全部是 inline（在 header），原因：
//   1. 這些函式很短（5~15 行），inline 不會造成程式碼膨脹
//   2. 方便單元測試直接呼叫（不需要 link .cpp）
//   3. 編譯器更容易優化（可能被內聯展開）

// --------------------------------------------------
// Circle vs Circle 碰撞
// --------------------------------------------------
// 回傳：是否碰撞
// 副作用：若碰撞，寫入推生向量 (outDx, outDy)（方向從 A 指向 B 的法向量）
// 以及穿透深度 outDepth
inline bool circleVsCircle(
    float ax, float ay, float ar,
    float bx, float by, float br,
    float& outDx, float& outDy, float& outDepth)
{
    float dx = bx - ax;
    float dy = by - ay;
    float distSq = dx * dx + dy * dy;
    float minDist = ar + br;

    if (distSq >= minDist * minDist) return false;

    float dist = std::sqrt(distSq);
    outDepth = minDist - dist;

    if (dist > 0.0001f) {
        outDx = dx / dist;
        outDy = dy / dist;
    } else {
        // 完全重疊：往任意方向推（避免除以零）
        outDx = 1.0f;
        outDy = 0.0f;
    }
    return true;
}

// --------------------------------------------------
// AABB vs AABB 碰撞
// --------------------------------------------------
// 軸對齊矩形（以中心點 + 半寬/半高定義）
// 推生方向：選擇穿透深度較小的軸（Minimum Penetration Axis）
inline bool aabbVsAabb(
    float ax, float ay, float ahw, float ahh,
    float bx, float by, float bhw, float bhh,
    float& outDx, float& outDy, float& outDepth)
{
    float overlapX = (ahw + bhw) - std::abs(bx - ax);
    float overlapY = (ahh + bhh) - std::abs(by - ay);

    if (overlapX <= 0.0f || overlapY <= 0.0f) return false;

    // 選擇穿透較淺的軸推生（最小穿透原則）
    if (overlapX < overlapY) {
        outDepth = overlapX;
        outDx = (bx > ax) ? 1.0f : -1.0f;
        outDy = 0.0f;
    } else {
        outDepth = overlapY;
        outDx = 0.0f;
        outDy = (by > ay) ? 1.0f : -1.0f;
    }
    return true;
}

// --------------------------------------------------
// Circle vs AABB 碰撞
// --------------------------------------------------
// 找到 AABB 上距圓心最近的點（Clamped Point），
// 計算圓心到該點的距離，若 < 半徑則碰撞。
inline bool circleVsAabb(
    float cx, float cy, float cr,
    float bx, float by, float bhw, float bhh,
    float& outDx, float& outDy, float& outDepth)
{
    // 最近點（clamp 到 AABB 邊界）
    float nearX = cx < (bx - bhw) ? (bx - bhw) : (cx > (bx + bhw) ? (bx + bhw) : cx);
    float nearY = cy < (by - bhh) ? (by - bhh) : (cy > (by + bhh) ? (by + bhh) : cy);

    float dx = nearX - cx;
    float dy = nearY - cy;
    float distSq = dx * dx + dy * dy;

    if (distSq >= cr * cr) return false;

    float dist = std::sqrt(distSq);
    outDepth = cr - dist;

    if (dist > 0.0001f) {
        // 從圓心指向最近點的方向（推生方向）
        outDx = dx / dist;
        outDy = dy / dist;
    } else {
        // 圓心在 AABB 內部：比較四邊距離，沿最近邊推出（最小穿透原則）
        float dLeft  = cx - (bx - bhw);  // 到左邊距離
        float dRight = (bx + bhw) - cx;  // 到右邊距離
        float dUp    = cy - (by - bhh);  // 到上邊距離
        float dDown  = (by + bhh) - cy;  // 到下邊距離
        float minDist = dLeft;
        outDx = -1.0f; outDy = 0.0f;
        if (dRight < minDist) { minDist = dRight; outDx =  1.0f; outDy = 0.0f; }
        if (dUp    < minDist) { minDist = dUp;    outDx =  0.0f; outDy = -1.0f; }
        if (dDown  < minDist) { minDist = dDown;  outDx =  0.0f; outDy =  1.0f; }
        outDepth = cr + minDist;  // 圓心在內部：穿透 = 半徑 + 到邊距離
    }
    return true;
}

// ============================================================
// CollisionSystem
// ============================================================
class CollisionSystem {
public:
    void update(Registry& registry, float dt);
};

} // namespace duck
