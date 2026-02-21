# Phase 1 設計文件：核心骨架

**日期：** 2026-02-21
**範圍：** CMake/vcpkg 建置、SDL2+OpenGL 視窗、手寫 ECS、輸入系統、基礎 2D 渲染器、Fixed Timestep Game Loop

---

## 決策摘要

| 決策項 | 選擇 |
|--------|------|
| ECS | 自己手寫（學習導向） |
| 依賴管理 | vcpkg manifest mode |
| 建置順序 | Bottom-Up（先視窗/渲染 → ECS → 輸入 → GameLoop → 整合） |
| 驗證目標 | 可 WASD 控制的鴨子 Sprite + 多個靜態場景物件 |

---

## 1. 專案建置

- CMake 3.20+, C++17
- vcpkg toolchain file 管理依賴
- 依賴：SDL2, GLAD, glm, stb (stb_image)
- 輸出：`duck_engine` 可執行檔

### vcpkg.json

```json
{
  "name": "duck-engine",
  "dependencies": ["sdl2", "glad", "glm", "stb"]
}
```

## 2. 目錄結構

```
duck_engine/
├── CMakeLists.txt
├── vcpkg.json
├── src/
│   ├── main.cpp
│   ├── core/Engine.h|cpp, GameLoop.h|cpp
│   ├── ecs/Registry.h|cpp, Entity.h, Components.h
│   ├── renderer/Shader.h|cpp, Texture.h|cpp, SpriteBatch.h|cpp, Renderer.h|cpp
│   ├── platform/Window.h|cpp, Input.h|cpp
│   └── systems/RenderSystem.h|cpp, MovementSystem.h|cpp
└── assets/textures/
```

## 3. 視窗與渲染器

### Window
- `init(title, width, height)` — SDL2 視窗 + OpenGL 3.3 Core context
- `swapBuffers()` — 雙緩衝交換
- `shouldClose()` — 關閉事件檢查
- 預設 1920x1080

### Shader
- `compile(vertexSrc, fragmentSrc)` — 編譯 shader program
- `use()` / `setUniform(...)` — 綁定與設定 uniform
- Phase 1：一個 sprite shader（position + texcoord + color）

### Texture
- `loadFromFile(path)` — stb_image 載入 PNG
- `bind(slot)` — 綁定紋理槽

### SpriteBatch
- `begin()` → `draw(texture, position, size, rotation, color)` → `end()`
- VBO/VAO 動態頂點，每幀重填
- 自動依 Z-Order 排序繪製
- 同紋理批次合併減少 draw call

### Renderer
- 持有 Shader + SpriteBatch
- `init()` — OpenGL 狀態（blend, depth）
- `clear(color)` / `drawSprite(...)`

## 4. 手寫 ECS

### Entity
- `EntityID = uint32_t`，遞增 ID，Entity 只是一個 ID

### ComponentPool\<T\>
- Dense array 存放同類型元件（快取友好）
- `entityToIndex` (sparse) + `indexToEntity` (dense) 映射
- API: `add()`, `get()`, `remove()`, `has()`

### Registry
- `create()` → EntityID
- `destroy(entity)` → 移除所有 component
- `addComponent<T>(entity, ...)` / `getComponent<T>(entity)`
- `view<T1, T2, ...>()` → 同時擁有指定元件的 entity 迭代器
- 使用 `std::type_index` 作 component type key
- view 採最小集合遍歷策略

### Phase 1 元件
- `Transform { float x, y, rotation, scaleX, scaleY; }`
- `Sprite { uint32_t textureID; float width, height; int zOrder; float r, g, b, a; }`
- `RigidBody { float vx, vy, mass, friction; }`
- `InputControlled {}` — 標記元件

## 5. 輸入系統

### Input
- `update()` — 處理 SDL 事件佇列
- `isKeyDown(key)` — Polling（移動/連發）
- `isKeyPressed(key)` — Event 單次觸發
- `getMousePosition()` → `{x, y}`
- `isMouseButtonDown(button)`
- 內部用 `unordered_set` 追蹤按鍵狀態

## 6. Game Loop (Fixed Timestep)

```
accumulator = 0
while running:
    currentTime = SDL_GetPerformanceCounter()
    deltaTime = currentTime - lastTime
    lastTime = currentTime
    accumulator += deltaTime

    input.update()

    while accumulator >= FIXED_DT:    // 60Hz
        fixedUpdate(FIXED_DT)
        accumulator -= FIXED_DT

    render()
    window.swapBuffers()
```

## 7. Phase 1 Systems

- **MovementSystem** — 遍歷 `<Transform, RigidBody, InputControlled>`，根據 Input 設定速度、套用摩擦力、更新位置
- **RenderSystem** — 遍歷 `<Transform, Sprite>`，依 zOrder 排序後繪製

## 8. 驗證目標

Phase 1 完成時應可展示：
1. 一個可用 WASD 控制移動的鴨子 Sprite，滑鼠控制方向
2. 多個靜態場景 Sprite 在不同 Z-Order 層正確繪製
3. 穩定 60 FPS 的 Fixed Timestep Game Loop
4. 基本摩擦力物理（放開按鍵後角色減速停止）
