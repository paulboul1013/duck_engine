#pragma once
#include "platform/Window.h"
#include "platform/Input.h"
#include "renderer/Renderer.h"
#include "renderer/Texture.h"
#include "ecs/Registry.h"
#include "systems/MovementSystem.h"
#include "systems/RenderSystem.h"
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace duck {

// ============================================================
// Engine — 引擎頂層，整合所有子系統
// ============================================================
// 職責：
// 1. 初始化所有子系統（Window, Renderer, Input）
// 2. 建立初始場景（entity + component）
// 3. 執行 Fixed Timestep Game Loop
// 4. 管理紋理資源的生命週期
//
// 為什麼 Engine 持有 Window、Renderer、Registry 而不是用全域變數？
// - 全域變數讓初始化順序難以控制（靜態初始化順序問題）
// - Engine 持有這些物件，解構時會依照宣告的相反順序清理
// - 方便未來測試：可以建立多個 Engine 實例（例如：伺服器端無渲染模式）
//
// 紋理管理策略：
// m_textureStore：unique_ptr<Texture>，擁有紋理的生命週期
// m_texturePtrs： Texture* 的查詢表，給 RenderSystem 用
// 分開兩個 map 的原因：
// - RenderSystem 只需要讀取，不需要管理生命週期
// - 傳原始指標給 RenderSystem 比傳 unique_ptr 的引用更清晰

class Engine {
public:
    bool init();
    void run();
    void shutdown();

private:
    void setupScene();

    // 子系統（宣告順序 = 初始化順序 = 解構相反順序）
    Window   m_window;
    Input    m_input;
    Renderer m_renderer;
    Registry m_registry;

    MovementSystem m_movementSystem;
    RenderSystem   m_renderSystem;

    // 紋理資源管理
    std::unordered_map<uint32_t, std::unique_ptr<Texture>> m_textureStore;
    std::unordered_map<uint32_t, Texture*>                  m_texturePtrs;
    uint32_t m_nextTextureID = 1;  // 從 1 開始，0 保留為「無效 ID」

    // Fixed Timestep 常數：1/60 秒
    // 為什麼用 constexpr float 而不是 #define？
    // - 有型別安全，不會意外做整數除法
    // - 在 debug 時可以看到變數名稱
    // - constexpr 讓編譯器在編譯期就算出 0.016666...
    static constexpr float FIXED_DT = 1.0f / 60.0f;
};

} // namespace duck
