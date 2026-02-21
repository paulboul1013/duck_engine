#  Duck Engine (2D) 遊戲引擎與專案規格書

## 1. 專案概述
### 1.1 目標遊戲類型
* **視覺視角：** 2D 俯視角（Top-Down）
* **遊戲類型：** 動作射擊 + 輕度 Roguelike + 資源管理（搜打撤）
* **參考作品：** 《逃離鴨科夫》(Escape From Duckov)、《挺進地牢》(Enter the Gungeon)、《以撒的結合》(The Binding of Isaac)

### 1.2 設計哲學
* **簡潔優先：** 確保核心循環（移動 → 射擊 → 撿戰利品 → 撤離）流暢且具娛樂性。
* **Mod 友善：** 預留 Lua 腳本接口，將業務邏輯與引擎底層分離，未來支援社群二次創作。

---

## 2. 核心系統規格
### 2.1 遊戲循環（Game Loop）
採用 **固定時間步（Fixed Timestep）** 處理物理與邏輯，**可變時間步（Variable Timestep）** 處理渲染，確保不同硬體上的物理表現一致。

| 項目 | 規格說明 |
| :--- | :--- |
| **目標幀率** | 60 FPS（支援顯示器 V-Sync 同步） |
| **時間精度** | 毫秒級（使用 `SDL_GetPerformanceCounter`） |

**主循環結構：**
```text
引擎初始化
└── 主循環（直到接收結束訊號）
    ├── 1. 處理輸入事件（鍵盤 / 滑鼠 / 手把）
    ├── 2. 邏輯與物理更新（60Hz 固定頻率 FixedUpdate）
    │   ├── 系統：移動、戰鬥、AI、物理碰撞檢測
    ├── 3. 渲染更新（與螢幕刷新率同步 Render）
    │   ├── 清除畫面
    │   ├── 繪製地圖圖塊
    │   ├── 繪製實體（依 Z-Order 排序）
    │   ├── 繪製粒子特效
    │   └── 繪製 UI 介面
    └── 4. 幀率控制與休眠（必要時 Delay 等待）
```

### 2.2 實體組件系統（ECS 架構）
**設計原則：** 資料導向設計（DOD），快取友好，避免傳統 OOP 的繼承地獄。

**核心組件（Components - 純資料）：**
| 組件名稱 | 資料內容 | 用途說明 |
| :--- | :--- | :--- |
| **Transform** | `x`, `y`, `rotation`, `scale` | 記錄實體位置與變形 |
| **Sprite** | `textureID`, `width`, `height`, `zOrder`, `color` | 視覺呈現與繪製參數 |
| **RigidBody** | `velocityX`, `velocityY`, `mass`, `friction` | 物理運動屬性 |
| **Collider** | `type` (Circle/AABB), `width`, `height`, `offset` | 碰撞邊界定義 |
| **Health** | `currentHP`, `maxHP`, `armor` | 生命值與護甲系統 |
| **Weapon** | `damage`, `fireRate`, `ammo`, `maxAmmo`, `range` | 武器射擊屬性 |
| **AI** | `state`, `targetEntity`, `detectRange`, `attackRange`| 敵人行為與狀態記錄 |
| **Inventory**| `slots[]`, `maxWeight`, `currentWeight` | 網格/槽位背包系統 |
| **Item** | `itemType`, `rarity`, `value`, `stackSize` | 掉落物與道具屬性 |

**核心系統（Systems - 純邏輯）：**
| 系統名稱 | 依賴組件 | 職責說明 |
| :--- | :--- | :--- |
| **MovementSystem** | `Transform` + `RigidBody` | 計算速度、套用摩擦力、處理地圖邊界限制 |
| **CollisionSystem**| `Transform` + `Collider` | 碰撞檢測、空間分割（四叉樹優化） |
| **RenderSystem** | `Transform` + `Sprite` | 批次繪製（Batching）、Z-Order 深度排序 |
| **CombatSystem** | `Weapon` + `Transform` | 射擊冷卻計算、生成子彈/彈道、傷害判定 |
| **AISystem** | `AI` + `Transform` + `RigidBody`| AI 狀態機切換、尋路、目標追蹤 |
| **InventorySystem**| `Inventory` + `Item` | 物品拾取、堆疊邏輯、重量計算、裝備切換 |
| **HealthSystem** | `Health` | 扣血邏輯、死亡判定、治療回復 |
| **AnimationSystem**| `Sprite` + `AnimationState` | 影格動畫更新、狀態切換（走/跑/死） |

---

## 3. 子系統規格
### 3.1 渲染系統
* **API：** OpenGL 3.3 Core Profile
* **解析度：** 基礎支援 1920x1080，支援視窗縮放。
* **優化技術：** 精靈圖（Sprite）批次渲染（Batch Rendering）、自動圖集合併（Texture Atlas）。
* **繪製層級 (Z-Order)：** `0.地面` → `1.陰影` → `2.牆壁/障礙` → `3.掉落物` → `4.角色(玩家/敵人)` → `5.彈道/特效` → `6.光源疊加` → `7.UI介面`

### 3.2 輸入與控制系統
* **鍵鼠控制：** `WASD` 移動，滑鼠瞄準方向，`左鍵` 射擊，`右鍵` 互動/次要開火，`R` 換彈，`Tab` 開啟背包。
* **輸入處理模式：**
  * **直接模式（Polling）：** 每幀偵測（用於移動、連發射擊）。
  * **觸發模式（Events）：** 單次觸發（用於換彈、開門、切換武器）。

### 3.3 物理與碰撞系統
* **碰撞形狀：** 圓形（角色、子彈）、AABB 軸對齊矩形（牆壁、障礙物）。
* **空間優化：** 四叉樹（Quadtree），每幀動態更新以減少碰撞計算量。
* **射線檢測（Raycasting）：** 用於 Hitscan 武器（如狙擊槍）的瞬間命中判定及 AI 視線遮擋檢測。

### 3.4 AI 行為樹系統 (狀態機)
```text
IDLE（閒置）
  └── 玩家進入偵測範圍 → 切換至 CHASE（追逐）
  
CHASE（追逐）
  ├── 玩家進入攻擊範圍 → 切換至 ATTACK（攻擊）
  ├── 玩家離開偵測範圍 → 等待 3 秒 → 切換至 PATROL（巡邏）
  └── 徹底失去目標 → 切換至 IDLE
  
ATTACK（攻擊）
  ├── 玩家離開攻擊範圍 → 切換回 CHASE
  └── 玩家死亡 → 切換至 IDLE
  
PATROL（巡邏）
  └── 玩家進入偵測範圍 → 切換至 CHASE
  
DEAD（死亡）
  └── 播放死亡動畫 → 掉落 Loot → 銷毀 ECS 實體
```

---

## 4. 遊戲機制：搜打撤循環
### 4.1 核心循環 (Core Loop)
**`進入地圖 → 探索/戰鬥 → 搜刮戰利品 → 尋找撤離點 → 倒數撤離 → 結算/販售/升級 → 重複`**
* **風險與獎勵：** 地圖停留越久，高價值戰利品越多，但敵人會逐漸變強或數量增加。
* **死亡懲罰：** 局內死亡將失去身上所有未放入「安全箱」的物品。

### 4.2 戰鬥與經濟
* **武器機制：** 具備彈匣容量與備彈限制，需手動 `R` 鍵換彈；連續射擊會增加準心擴散（後座力模擬）。
* **經濟系統：** 局外貨幣為 **鴨幣 (DuckCoin)**，可向商人購買武器、彈藥、醫療包，或升級基礎屬性（背包容量、血量上限）。

---

## 5. 技術規格與架構
### 5.1 開發環境與技術棧
| 項目 | 選擇方案 |
| :--- | :--- |
| **開發語言** | C++17（引擎核心） + Lua（遊戲腳本） |
| **建置系統** | CMake |
| **編譯器** | MSVC (Windows) / GCC (Linux) |
| **第三方依賴** | `SDL2` (視窗/輸入), `OpenGL / GLAD` (渲染), `sol2` (Lua 綁定), `nlohmann/json` (資料解析), `glm` (數學庫) |

### 5.2 效能目標
* **FPS：** 嚴格保持 60 幀（永不低於）。
* **記憶體佔用：** 穩定運行時 < 512 MB。
* **載入速度：** 關卡地圖載入 < 3 秒。
* **實體乘載量：** 同畫面支援 100+ 活躍實體（含敵人、子彈、掉落物）而不掉幀。

### 5.3 專案目錄結構
```text
DuckEngine/
├── src/                  # C++ 原始碼
│   ├── core/             # 引擎啟動、GameLoop、記憶體池
│   ├── ecs/              # ECS 核心架構 (Registry, Component, System)
│   ├── systems/          # 各子系統具體實作 (Render, Physics, etc.)
│   ├── renderer/         # OpenGL 封裝 (Shader, Texture, Batch)
│   ├── platform/         # SDL2 視窗與輸入抽象層
│   ├── scripting/        # sol2 Lua 綁定介面
│   └── game/             # 遊戲本體特定邏輯 (可編譯為 DLL/SO)
├── assets/               # 遊戲資源
│   ├── textures/         # 精靈圖、圖塊集 (Sprite Sheets)
│   ├── sounds/           # BGM 與 SFX
│   ├── maps/             # 地圖資料 (JSON)
│   └── scripts/          # Lua 模組與腳本
├── vendor/               # 第三方函式庫 (SDL2, GLM, JSON 等)
└── CMakeLists.txt        # 專案建置檔
```

---

## 6. 開發里程碑 (Milestones)

- [ ] **Phase 1：核心骨架（2-3 週）**
  - [ ] 整合 CMake、SDL2 與 OpenGL，建立視窗與 Fixed Game Loop。
  - [ ] 實作基礎 ECS 架構（Entity 創建、Component 掛載、System 遍歷）。
  - [ ] 實作鍵盤與滑鼠輸入捕捉。
  - [ ] 完成基本 2D 渲染器（Shader 編譯、載入 PNG、繪製靜態 Sprite）。
- [ ] **Phase 2：遊戲基礎（3-4 週）**
  - [ ] 玩家移動控制與滑鼠跟隨瞄準。
  - [ ] 實作基本射擊系統（生成子彈實體、直線飛行）。
  - [ ] 實作 AABB 與圓形碰撞檢測（Collision System）。
  - [ ] 實作最基礎的敵人（追逐玩家）與扣血機制。
- [ ] **Phase 3：內容擴展（4-6 週）**
  - [ ] 引入四叉樹（Quadtree）優化碰撞。
  - [ ] 擴展武器系統（步槍連發、霰彈槍多彈丸）。
  - [ ] 實作 AI 狀態機（巡邏、追逐、攻擊）。
  - [ ] 解析 JSON 載入地圖；實作背包與掉落物撿拾邏輯。
- [ ] **Phase 4：完整循環（3-4 週）**
  - [ ] 實作「撤離點」機制與倒數計時。
  - [ ] 開發局外介面（主選單、商店購買、背包整理）。
  - [ ] 實作 JSON 存讀檔（保存玩家金錢與倉庫裝備）。
  - [ ] 完善 Lua 腳本綁定，將部分物品數值移交 Lua 管理。
- [ ] **Phase 5：打磨優化（持續）**
  - [ ] 加入粒子系統（開火火光、流血特效）。
  - [ ] 實作螢幕震動（Screen Shake）增強打擊感。
  - [ ] 記憶體與效能 Profiling 優化，修復 Bug。

---

## 7. 風險與緩解策略
| 風險項目 | 緩解策略 |
| :--- | :--- |
| **效能不足 (掉幀)** | 早期引入 Profiler 工具；嚴格落實 Sprite 批次渲染；大量碰撞提早實作 Quadtree。 |
| **範圍膨脹 (Scope Creep)** | 嚴格遵守 Phase 1~4 規劃，**優先完成核心「搜打撤」循環**，再考慮擴充槍枝或敵人種類。 |
| **除錯困難** | 建立統一的 Logger 系統；在渲染器中實作 `DebugDraw` 模式（用紅線畫出所有碰撞框與 AI 視線）。 |
| **跨平台