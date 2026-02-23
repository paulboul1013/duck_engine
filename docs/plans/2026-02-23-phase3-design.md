# Phase 3 設計文件：Health/Damage + AISystem + Quadtree

**日期：** 2026-02-23
**範圍：** 敵人 AI（IDLE→CHASE）、Health/Damage 系統、Quadtree 空間分割
**Phase 2 前提：** CollisionSystem（Solid 推生 + Bullet 擊中消失）、DebugDraw 已完成

---

## 目標

Phase 3 結束後，玩家應能：
1. 射擊敵人 → 敵人扣血 → 血量歸零後消失
2. 敵人在偵測範圍內發現玩家 → 追上來
3. 敵人碰到玩家 → 玩家扣血（附無敵幀避免連扣）

---

## 新增元件

### Health

```cpp
struct Health {
    float hp              = 100.0f;
    float maxHp           = 100.0f;
    float invincibleTimer = 0.0f;   // 無敵幀倒數（秒）
    float damageOnTouch   = 20.0f;  // 接觸傷害（敵人碰玩家時扣多少）
};
```

**設計說明：**
- `invincibleTimer`：避免敵人一直碰著玩家時每幀都扣血（設 0.5 秒無敵窗）
- `damageOnTouch`：直接存在 Health 元件，不另開 Damage 元件（Phase 3 傷害來源簡單）
- 玩家和敵人都用同一個 Health 元件（ECS 統一處理）

### AIAgent（狀態機資料）

```cpp
struct AIAgent {
    enum class State { IDLE, CHASE };
    State state       = State::IDLE;
    float detectRange = 200.0f;   // 偵測半徑（像素）
    float speed       = 80.0f;    // 追擊速度（像素/秒）
};
```

**設計說明：**
- 純資料，不含邏輯（符合 ECS 原則）
- 滯留帶（hysteresis）在 AISystem 以 `detectRange * 1.2` 實作，避免玩家在邊界時敵人狀態抖動
- Phase 4 擴充 ATTACK 狀態時只需新增欄位，不影響現有邏輯

---

## 新增系統

### AISystem

**職責：** 每幀更新所有敵人的狀態機，並設定 RigidBody 速度

**執行位置：** Fixed Update 最前面（在 MovementSystem 之前，確保速度已設定）

```
AISystem::update(registry, dt):
1. 找玩家：view<Transform, InputControlled> → 取第一個 entity 的位置 (px, py)
2. view<Transform, AIAgent, RigidBody> for each enemy:
   a. 計算距離 d = sqrt((ex-px)² + (ey-py)²)
   b. IDLE → CHASE 條件：d < detectRange
   c. CHASE → IDLE 條件：d > detectRange * 1.2（滯留帶）
   d. CHASE 時：
      nx = (px - ex) / d, ny = (py - ey) / d
      rb.vx = nx * speed, rb.vy = ny * speed
   e. IDLE 時：rb.vx = 0, rb.vy = 0
```

**為什麼用 RigidBody 速度而非直接修改 Transform？**
- MovementSystem 已處理 RigidBody 的摩擦力減速
- 一致性：所有移動都經過 RigidBody，物理行為統一
- 未來加上障礙物推開時，物理系統自動處理

### HealthSystem

**職責：** 每幀處理無敵幀倒數與死亡判斷

**執行位置：** Fixed Update 最後（在 CollisionSystem 之後）

```
HealthSystem::update(registry, dt):
1. view<Health> for each entity:
   a. if invincibleTimer > 0: invincibleTimer -= dt
   b. if hp <= 0: registry.destroy(entity)
```

**注意：** destroy 在 view 迴圈內是安全的（Registry::view 複製了 entity 列表）

---

## CollisionSystem 擴充（Damage 觸發）

Phase 3 在既有的 Bullet vs Solid 邏輯之後新增：

### 子彈傷害（Bullet → 有 Health 的實體）

```
原有：bullet 碰到任何 Solid → destroy bullet
Phase 3：若 Solid 有 Health 元件 → 額外扣 HP（預設 30）
```

子彈傷害值暫時 hardcode 為 30，Phase 4 移入 Weapon 元件。

### 接觸傷害（AIAgent → 玩家）

```
在 Solid vs Solid 推生後，額外判斷：
  若 A 有 AIAgent 且 B 有 InputControlled：
    若 B（玩家）的 invincibleTimer <= 0：
      B.hp -= A.damageOnTouch
      B.invincibleTimer = 0.5 秒
  反向同理（B 有 AIAgent，A 有 InputControlled）
```

---

## Quadtree（header-only，只給 CollisionSystem 用）

### 設計

```cpp
// src/systems/Quadtree.h

struct QRect { float x, y, hw, hh; };  // 中心 + 半寬高

class Quadtree {
    static const int MAX_DEPTH    = 5;
    static const int MAX_CAPACITY = 4;  // 每格超過此數才分裂

    struct Node {
        QRect bounds;
        std::vector<std::pair<EntityID, glm::vec2>> items;
        std::array<std::unique_ptr<Node>, 4> children;  // NW, NE, SW, SE
        bool isLeaf() const { return !children[0]; }
    };

public:
    explicit Quadtree(QRect worldBounds);
    void clear();
    void insert(EntityID id, float x, float y);
    void queryRange(QRect range, std::vector<EntityID>& out) const;

private:
    Node m_root;
};
```

### CollisionSystem 整合

```cpp
// CollisionSystem.h 新增成員
Quadtree m_quadtree;  // worldBounds = {640, 360, 640, 360}（1280x720）

// update() 開頭：
m_quadtree.clear();
for each solid entity:
    m_quadtree.insert(e, tf.x, tf.y);

// Solid vs Solid 配對：
for each entity A in solidEntities:
    vector<EntityID> candidates;
    m_quadtree.queryRange({tfA.x, tfA.y, colA.maxRadius()*2, colA.maxRadius()*2}, candidates);
    for B in candidates where B > A:  // 避免重複
        // 原有碰撞判斷
```

**為什麼 worldBounds 是 1280x720？**
目前場景固定大小，Phase 4 做房間系統時再改成動態邊界。

---

## 執行順序（Fixed Update）

```
1. AISystem::update          ← 設定敵人速度
2. MovementSystem::update    ← 根據速度移動所有 entity（含玩家、敵人）
3. WeaponSystem::update      ← 更新子彈移動和生命週期
4. CollisionSystem::update   ← Quadtree 建立 + 碰撞 + Damage
5. HealthSystem::update      ← 無敵幀倒數 + 死亡銷毀
```

---

## 場景設置（Engine::setupScene）

新增兩隻敵人鴨子（紅色）：

```cpp
// 敵人 1
auto enemy1 = registry.create();
addComponent<Transform>(enemy1, 400.0f, 200.0f, ...);
addComponent<Sprite>(enemy1, redDuckTexID, 48.0f, 48.0f, 4, ...);
addComponent<RigidBody>(enemy1, 0.0f, 0.0f, 1.0f, 0.85f);
addComponent<Collider>(enemy1, Collider::Type::Circle, 24.0f, 24.0f, 24.0f, true);
addComponent<Health>(enemy1, 100.0f, 100.0f, 0.0f, 20.0f);
addComponent<AIAgent>(enemy1, AIAgent::State::IDLE, 200.0f, 80.0f);
```

玩家也加上 Health（預設 100 HP）：

```cpp
addComponent<Health>(player, 100.0f, 100.0f, 0.0f, 0.0f);
```

---

## 測試計畫

### 單元測試（test_quadtree.cpp）

- 插入 0/1/多個 entity
- queryRange 回傳正確的鄰居
- 超過 MAX_CAPACITY 後正確分裂

### 視覺整合測試

| 行為 | 驗證方式 |
|---|---|
| 射擊敵人 3 發後死亡 | 30 HP × 3 = 90，第 4 發死（若 HP=100 需 4 發） |
| 敵人死亡後消失 | 不殘留 entity |
| 敵人進入範圍追玩家 | 靠近後看到敵人向玩家移動 |
| 敵人離開範圍停止 | 跑出範圍後停止追擊 |
| 敵人碰玩家扣血 | HP 顯示（暫時用 printf） |
| 無敵幀生效 | 碰觸後 0.5 秒內不再扣血 |

---

## 檔案變更清單

| 檔案 | 操作 | 說明 |
|---|---|---|
| `src/ecs/Components.h` | 修改 | 新增 `Health`、`AIAgent` struct |
| `src/systems/Quadtree.h` | 新增 | header-only Quadtree 實作 |
| `src/systems/AISystem.h` | 新增 | AISystem 宣告（inline 小函式） |
| `src/systems/AISystem.cpp` | 新增 | AISystem::update 實作 |
| `src/systems/HealthSystem.h` | 新增 | HealthSystem 宣告 |
| `src/systems/HealthSystem.cpp` | 新增 | HealthSystem::update 實作 |
| `src/systems/CollisionSystem.cpp` | 修改 | 整合 Quadtree + Damage 觸發 |
| `src/core/Engine.h` | 修改 | 新增 AISystem、HealthSystem 成員 |
| `src/core/Engine.cpp` | 修改 | 場景加敵人 + FixedUpdate 加系統 |
| `tests/test_quadtree.cpp` | 新增 | Quadtree 單元測試 |
| `CMakeLists.txt` | 修改 | 加入新 .cpp 到建置目標 |

---

## 效能預期

Phase 3 場景：玩家 1 + 敵人 3 + 子彈 max 20 = 約 24 個 entity

Quadtree 查詢（平均每格 2~3 個 entity，最大深度 5）：
- 建立時間：24 次插入 ≈ 24 × 100ns = 2.4μs
- 查詢時間：每個 entity 查鄰居 ≈ 24 × 200ns = 4.8μs
- O(n²) 原本 = 24 × 23 / 2 × 10ns = 2760ns = 2.76μs

Phase 3 entity 數量下 Quadtree 幾乎沒有優勢，但這是學習投資，Phase 4（100+ entity）時會顯現差距。

---

*下一階段（Phase 4）：武器系統擴充、地圖載入（JSON）、完整「搜打撤」循環*
