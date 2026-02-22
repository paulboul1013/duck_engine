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

## 建置系統
- SDL2 用系統 apt（vcpkg 的 SDL2 需要 autoconf-archive）
- glad/glm/stb 用 vcpkg
- test_ecs 是純 CPU 測試，不依賴 OpenGL/SDL2
