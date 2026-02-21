# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Duck Engine 是一個 2D 俯視角（Top-Down）動作射擊遊戲引擎，核心玩法為「搜打撤」循環（進入地圖 → 探索/戰鬥 → 搜刮 → 撤離）。參考作品：《逃離鴨科夫》、《挺進地牢》、《以撒的結合》。

## Build System

- **語言：** C++17（引擎核心）+ Lua（遊戲腳本）
- **建置系統：** CMake
- **編譯器：** GCC (Linux)

```bash
# 建置（待專案建立後）
mkdir build && cd build
cmake ..
cmake --build .
```

## Tech Stack

- `SDL2` — 視窗管理與輸入處理
- `OpenGL 3.3 Core Profile` + `GLAD` — 圖形渲染
- `sol2` — Lua C++ 綁定
- `nlohmann/json` — JSON 資料解析
- `glm` — 向量/矩陣數學庫

## Architecture

### ECS（Entity Component System）— 資料導向設計

引擎核心採用 DOD（Data-Oriented Design），元件為純資料結構，系統為純邏輯函式，避免 OOP 繼承。

**核心元件：** Transform, Sprite, RigidBody, Collider, Health, Weapon, AI, Inventory, Item

**核心系統：** MovementSystem, CollisionSystem, RenderSystem, CombatSystem, AISystem, InventorySystem, HealthSystem, AnimationSystem

### Game Loop

固定時間步（60Hz）處理物理/邏輯，可變時間步處理渲染，使用 `SDL_GetPerformanceCounter` 計時。

### Rendering

- 解析度：1920x1080 基礎，支援視窗縮放
- Sprite 批次渲染（Batch Rendering）+ 自動圖集合併
- Z-Order 8 層：地面 → 陰影 → 牆壁 → 掉落物 → 角色 → 彈道/特效 → 光源 → UI

### Collision

- 碰撞形狀：Circle（角色/子彈）、AABB（牆壁/障礙物）
- 空間分割：Quadtree 每幀動態更新
- Raycasting 用於 Hitscan 武器和 AI 視線檢測

### AI State Machine

狀態流轉：IDLE → CHASE → ATTACK → PATROL → DEAD，基於偵測範圍與攻擊範圍觸發轉換。

## Directory Structure

```
src/core/        # 引擎啟動、GameLoop、記憶體池
src/ecs/         # ECS 核心（Registry, Component, System）
src/systems/     # 子系統實作（Render, Physics 等）
src/renderer/    # OpenGL 封裝（Shader, Texture, Batch）
src/platform/    # SDL2 視窗與輸入抽象層
src/scripting/   # sol2 Lua 綁定介面
src/game/        # 遊戲本體邏輯（可編譯為 DLL/SO）
assets/          # textures/, sounds/, maps/ (JSON), scripts/ (Lua)
vendor/          # 第三方函式庫
```

## Performance Targets

- 嚴格 60 FPS（永不低於）
- 記憶體 < 512 MB
- 關卡載入 < 3 秒
- 100+ 活躍實體不掉幀

## Design Principles

- **簡潔優先：** 核心循環（移動→射擊→撿寶→撤離）必須流暢
- **Mod 友善：** Lua 腳本介面分離業務邏輯與引擎底層
- **快取友好：** DOD 資料排列，避免繼承地獄
- **範圍控制：** 嚴格遵守開發階段規劃，優先完成核心「搜打撤」循環

## 注意
用繁體中文回答
因為是學習專案，所以請把所有知識或是過程給載入到知識庫，例如know.md
把任務先拆分成函式，並直接測試功能，並一定要問我這樣的git commit，並且我自己git push
