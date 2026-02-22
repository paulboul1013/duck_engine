# Duck Engine 知識庫

## ECS（Entity Component System）核心概念

### Entity 只是一個 ID
- `EntityID = uint32_t`，不是物件，不繼承任何東西
- 用 4 bytes 而非 8 bytes，因為足夠且更 cache-friendly
- `INVALID_ENTITY = UINT32_MAX` 作為空值標記

### ComponentPool — Dense Array + Sparse Set
- 同類型元件在記憶體中**連續排列**（cache-friendly）
- 三層結構：
  - `m_components`: vector<T> — 實際資料（dense）
  - `m_indexToEntity`: vector<EntityID> — dense index → entity 映射
  - `m_entityToIndex`: unordered_map<EntityID, size_t> — entity → index 映射
- **Swap-and-Pop 刪除**：把最後一個搬到被刪位置，O(1) 而非 O(n)
- 不保證順序，但 ECS 不需要順序

### 為什麼比 std::map 快？
- `std::map` 每個節點分散在 heap，遍歷時 cache miss
- Dense array 連續存放，CPU prefetcher 預取有效，快 5-10 倍
- 在 100+ entity 的遊戲場景中差異明顯

### IComponentPool — 型別擦除
- Registry 用 `unordered_map<type_index, unique_ptr<IComponentPool>>` 管理所有 pool
- `IComponentPool` 提供 `remove()` 和 `has()` 虛擬介面
- `ComponentPool<T>` 繼承它，加入泛型的 `add()`, `get()` 等方法

### Tag Component（標記元件）
- 空結構體如 `InputControlled {}`，sizeof=1
- 不儲存資料，只用來標記 entity 具有某種屬性
- System 透過 `view<Transform, RigidBody, InputControlled>` 篩選

### Components 設計原則
- 純資料結構（POD），不含方法
- 所有行為由 System 實作
- 好處：可序列化、可 memcpy、無 vtable 開銷

### Registry — ECS 的大腦
- `create()` 回傳遞增 EntityID，`destroy()` 清除所有元件並移除 entity
- 用 `std::type_index(typeid(T))` 作為 pool 的 key，不需手動編號
- `view<T1, T2, ...>()` 用 C++17 fold expression 展開成多個 hasComponent 檢查
- view 遍歷時複製 entities 列表，防止 callback 中修改 pool 導致 UB

## Input 輸入系統

### Polling vs Event 模式
- **Polling (`isKeyDown`)**: 鍵是否被按住？每幀回傳 true → 用於移動、連發
- **Event (`isKeyPressed`)**: 鍵是否本幀剛按下？只回傳一次 true → 用於換彈、互動
- 實作：`m_keysDown`（持續追蹤）vs `m_keysPressed`（每幀清空）

### SDL_Scancode vs SDL_Keycode
- Scancode = 物理位置（遊戲用這個，不受鍵盤佈局影響）
- Keycode = 字元輸出（法語 AZERTY 會讓 WASD 錯位）

### event.key.repeat 過濾
- 長按時 OS 會自動重複觸發 KEYDOWN
- 不過濾的話 isKeyPressed() 長按時會週期性觸發 → bug
- 用 `if (!event.key.repeat)` 只取第一次按下

### 踩坑紀錄
- **GCC vs Clang alias template 差異**：`template<typename First, typename...> using FirstType = First;` 配合 `FirstType<Ts...>` 在 GCC 會報錯（pack expansion argument for non-pack parameter），改用 `viewImpl<First, Rest...>` 拆開參數包解決
- test_ecs 需要 link Registry.cpp（因為有非模板成員函式），CMake 中要明確列出

## 建置系統
- SDL2 用系統 apt（vcpkg 的 SDL2 需要 autoconf-archive）
- glad/glm/stb 用 vcpkg
- test_ecs 是純 CPU 測試，不依賴 OpenGL/SDL2，但需要 link Registry.cpp

### GLOB_RECURSE 的陷阱
- `file(GLOB_RECURSE SOURCES "src/*.cpp")` 在 CMake configure 階段**快取**檔案列表
- **新增 .cpp 後只執行 `cmake --build build` 不夠**，連結器找不到新符號（undefined reference）
- 必須重新執行 `cmake -B build -S .` 刷新 GLOB，再 `cmake --build build`
- 替代方案：明確列出所有 .cpp（更保險，但維護成本高）

## Game Loop 設計

### Fixed Timestep（固定時間步）
- 物理/邏輯在固定 1/60 秒的 tick 更新，渲染以可變頻率跟上
- 使用 accumulator 模式：`accumulator += deltaTime`，每 >= FIXED_DT 消耗一次
- 好處：不同幀率的機器物理行為完全一致

### Spiral of Death 防護
- 問題：某幀耗時過長（如卡頓），accumulator 暴增 → 下幀拚命追趕 → 更卡頓
- 解決：`if (deltaTime > 0.25f) deltaTime = 0.25f;`（上限 4 FPS 的幀時間）
- 超過就直接丟棄，讓遊戲「假裝時間只過了 0.25 秒」

## Rendering 設計

### 紋理 ID 間接引用（Texture ID Indirection）
- `Sprite` 存 `uint32_t textureID`，不直接存 `Texture*`
- 好處：紋理熱重載時指標可能移動，但 ID 永遠有效
- `Engine` 持有 `m_textureStore`（unique_ptr，擁有生命週期）+ `m_texturePtrs`（raw ptr，供讀取）
- ID=0 保留為無效值，從 1 開始分配

### Z-Order 繪製順序
- 數字小 = 先繪製（在下層）
- 0: 地面/草地, 2: 靜態障礙物, 4: 角色/玩家
- SpriteBatch 在 end() 時按 zOrder 排序後批次提交

## 子系統設計原則

### Tag Component 用於行為標記
- `InputControlled` 空結構體：標記「這個 entity 受玩家控制」
- MovementSystem 第一個 view `<Transform, RigidBody, InputControlled>` 只處理玩家
- 第二個 view `<Transform, RigidBody>` 對所有物理物件套用速度/摩擦力
- 好處：未來加敵人只需不加 InputControlled，邏輯自動分離

### Diagonal Movement Normalization
- 同時按 W+D 時 inputX=1, inputY=-1，合速度 = sqrt(2) ≈ 1.41 倍
- 修正：`float len = sqrt(inputX*inputX + inputY*inputY); if (len > 0) { inputX/=len; inputY/=len; }`

### Friction Dead Zone（摩擦力死區）
- 每幀速度 *= friction（0.85），永遠不會精確等於 0，浪費 CPU
- 解決：`if (abs(vx) < 0.1f) vx = 0.0f;`

### 滑鼠朝向
- `atan2(mouse.y - entity.y, mouse.x - entity.x)` 計算朝向角度
- 回傳 [-π, π]，直接存入 Transform.rotation，由 SpriteBatch 用 cos/sin 旋轉
