# Repository Guidelines

## 專案結構與模組劃分
`src/` 是引擎原始碼，依職責分為：`core/` 放啟動流程與主迴圈，`ecs/` 放實體與元件儲存邏輯，`systems/` 放遊戲系統，`renderer/` 放 OpenGL 渲染封裝，`platform/` 放 SDL 視窗與輸入抽象。`tests/` 放獨立的 C++ 測試程式，例如 `test_ecs.cpp` 與 `test_collision.cpp`。規劃文件與筆記放在 `docs/` 與 `PLAN.md`。編譯輸出請集中在 `build/`。

## 建置、測試與開發指令
初始化建置目錄：

```bash
cmake -S . -B build
```

編譯引擎與測試：

```bash
cmake --build build
```

本機執行遊戲：

```bash
./build/duck_engine
```

執行指定測試：

```bash
./build/test_ecs
./build/test_collision
```

若缺少相依套件，請依 `CMakeLists.txt` 與 `vcpkg.json` 的設定，透過系統套件管理器與 `vcpkg` 安裝。

## 程式風格與命名慣例
使用 C++17。遵循現有風格：4 個空白縮排，函式的大括號與宣告同一行，區域輔助函式使用 `snake_case`，例如 `registerTexture`。型別名稱使用 `PascalCase`，例如 `Engine`、`CollisionSystem`；成員變數使用 `m_` 前綴，例如 `m_renderer`、`m_debugMode`。命名空間維持在 `duck` 下。註解應說明不直觀的引擎設計或數學原因，不要只是重述程式碼。

## 測試指南
目前專案使用以 `assert` 為主的可執行測試，而非完整測試框架。新測試請放在 `tests/test_<feature>.cpp`，並盡量保持可重現、純 CPU、低依賴。優先補齊 ECS 行為、幾何運算與系統邊界條件。開 PR 前至少執行與修改內容相關的測試。

## Commit 與 Pull Request 準則
近期 commit 採用 `feat:`、`fix:`、`style:`、`docs:` 等前綴，後面接簡短描述。每個 commit 應聚焦單一主題。Pull Request 需要說明這次修改對引擎或玩法的影響，列出你實際執行的指令，例如 `cmake --build build`、`./build/test_collision`，若有渲染或輸入行為變更，請附上截圖或短影片。

## 設定注意事項
不要提交 `build/` 內的產生檔。若調整相依套件，請同步更新 `CMakeLists.txt` 與 `vcpkg.json`。
