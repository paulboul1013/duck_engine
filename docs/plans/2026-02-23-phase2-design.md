# Phase 2 設計文件：CollisionSystem

**日期：** 2026-02-23
**範圍：** 碰撞偵測系統（基礎版）
**Phase 1 前提：** ECS、Input、Renderer、MovementSystem、WeaponSystem 已完成

---

## 目標

實作 2D 俯視角射擊遊戲的基礎碰撞系統，讓：
1. 子彈碰到固體實體時消失
2. 固體實體之間不互相重疊（推生）
3. 提供 DebugDraw 模式驗證碰撞邊界

**範圍外（Phase 3）：**
- 敵人 AI / 血量扣除
- Quadtree 空間分割
- Layer bitmask 過濾

---

## 設計決策：方案 A（O(n²) 暴力比對）

Phase 2 實體數量少（玩家 1 + 子彈 10~20），O(n²) 完全足夠。
Quadtree 最佳化留到 Phase 3（與敵人 AI 一起，才有優化價值）。

---

## 新增元件：`Collider`

```cpp
// 加入 src/ecs/Components.h

struct Collider {
    enum class Type { Circle, AABB };
    Type type = Type::Circle;

    // AABB 半寬/半高（以 Transform 為中心）
    float halfW = 16.0f;
    float halfH = 16.0f;

    // Circle 半徑
    float radius = 16.0f;

    // isSolid=true：碰撞後推開（牆壁/角色）
    // isSolid=false：穿透觸發（未來 Trigger 用）
    bool isSolid = true;
};
```

**設計原則：**
- 純資料結構（POD），符合 ECS 設計
- 不包含任何邏輯，只描述形狀與屬性

---

## 碰撞幾何函式（純函式，放在 CollisionSystem.h）

### Circle vs Circle
```
距離 = sqrt((x2-x1)² + (y2-y1)²)
碰撞 = 距離 < r1 + r2
穿透深度 = (r1 + r2) - 距離
```

### AABB vs AABB
```
重疊X = min(x1+hw1, x2+hw2) - max(x1-hw1, x2-hw2)
重疊Y = min(y1+hh1, y2+hh2) - max(y1-hh1, y2-hh2)
碰撞 = 重疊X > 0 && 重疊Y > 0
推生方向：選取較小的重疊軸
```

### Circle vs AABB
```
最近點 = clamp(circle.center, aabb.min, aabb.max)
距離 = |circle.center - 最近點|
碰撞 = 距離 < circle.radius
```

---

## CollisionSystem 運作流程

```
void CollisionSystem::update(Registry& reg, float dt):

1. 用 view<Transform, Collider> 收集所有有碰撞的 entity
2. O(n²) 雙層迴圈，跳過 i == j
3. 對每對 (A, B) 計算碰撞：
   a. 取得雙方形狀（用 Collider.type 分派）
   b. 呼叫對應幾何函式
4. 碰撞發生時，根據組合決定回應：

碰撞回應表：
┌─────────────────────────────────────────────────────────┐
│ A 有 Bullet | B 有 Bullet | A isSolid | B isSolid | 結果              │
│ yes         | -          | -         | yes       │ destroy A (bullet) │
│ -           | yes        | yes       | -         │ destroy B (bullet) │
│ no          | no         | yes       | yes       │ 互推各半           │
└─────────────────────────────────────────────────────────┘

5. 推生向量（Separate）：
   - 計算重疊方向的單位向量
   - A.transform -= overlap * 0.5f * normalVec
   - B.transform += overlap * 0.5f * normalVec
   - 若其中一方是靜態（無 RigidBody），只推動態方

6. 收集待刪除 entity，在迴圈外統一銷毀（避免邊刪邊遍歷）
```

---

## DebugDraw 功能

**觸發方式：** 按 `F1` 切換 `g_debugCollision` 全域 bool

**實作位置：** RenderSystem 最後一步，若 `g_debugCollision == true`：
- Circle Collider → 用線段模擬圓形（32 段多邊形），顏色：紅色半透明
- AABB Collider  → 繪製 4 條線框，顏色：藍色半透明

**技術細節：**
- 不需要額外 VAO，複用 SpriteBatch 的 immediate line 模式（或用 1x1 像素紋理拉伸）
- DebugDraw 在最高 Z-Order（7.5，UI 之下）繪製，確保可見

---

## 檔案變更清單

| 檔案 | 操作 | 說明 |
|---|---|---|
| `src/ecs/Components.h` | 修改 | 新增 `Collider` struct |
| `src/systems/CollisionSystem.h` | 新增 | class 宣告 + 幾何函式 |
| `src/systems/CollisionSystem.cpp` | 新增 | update() 實作 |
| `src/systems/RenderSystem.cpp` | 修改 | 加入 DebugDraw 邏輯 |
| `src/core/Engine.cpp` | 修改 | 在 GameLoop 中呼叫 CollisionSystem::update() |
| `tests/test_collision.cpp` | 新增 | 幾何函式單元測試 |
| `CMakeLists.txt` | 修改 | 加入新 .cpp 到建置目標 |

---

## 測試計畫

### 單元測試（test_collision.cpp）
- Circle vs Circle：不碰 / 剛好碰 / 深度重疊
- AABB vs AABB：不碰 / X 軸碰 / Y 軸碰 / 角落碰
- Circle vs AABB：不碰 / 面碰 / 角碰

### 整合測試（視覺確認）
1. 按 F1 開啟 DebugDraw → 看到紅圓/藍框
2. 射擊子彈 → 子彈打到玩家自己的碰撞框（暫時跳過） → 子彈消失
3. 兩個 Solid 實體靠近 → 被推開，不重疊

---

## 開發順序

1. 新增 `Collider` 到 Components.h
2. 撰寫 `CollisionSystem.h/.cpp`（先純幾何函式 + 無副作用的邏輯）
3. 撰寫單元測試 `test_collision.cpp`，跑通所有測試
4. 整合到 Engine::update()
5. 修改 RenderSystem 加 DebugDraw
6. 手動測試：F1 看碰撞框，射擊測試子彈消失

---

## 效能預期

Phase 2 場景：玩家 1 + 子彈 max 20 = 約 21 個 entity
O(n²) = 21 × 20 / 2 = 210 次比較/幀
每次比較 < 10ns → 210 × 10ns = 2100ns = 0.002ms
**完全不影響效能目標（< 16.67ms/幀）**

---

*下一階段（Phase 3）：引入 Quadtree、敵人 AI、Health/Damage 系統*
