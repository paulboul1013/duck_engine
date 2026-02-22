#include "core/Engine.h"
#include "ecs/Components.h"
#include <SDL2/SDL.h>
#include <cstdio>

namespace duck {

// 輔助：建立純色紋理並登記到資源表，回傳 ID
static uint32_t registerTexture(
    std::unordered_map<uint32_t, std::unique_ptr<Texture>>& store,
    std::unordered_map<uint32_t, Texture*>& ptrs,
    uint32_t& nextID,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    auto tex = std::make_unique<Texture>();
    tex->createSolidColor(r, g, b, a, 4, 4);
    uint32_t id = nextID++;
    ptrs[id] = tex.get();
    store[id] = std::move(tex);
    return id;
}

bool Engine::init() {
    if (!m_window.init("Duck Engine - Phase 1", 1280, 720)) return false;
    if (!m_renderer.init(1280, 720)) return false;

    setupScene();

    std::printf("=== Engine 初始化完成 ===\n");
    std::printf("WASD 移動，滑鼠瞄準，ESC 退出\n");
    return true;
}

void Engine::setupScene() {
    // 建立紋理（目前用程式產生的純色方塊代替真實 Sprite）
    uint32_t duckID  = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                       255, 200,   0, 255);  // 鴨子黃
    uint32_t grassID = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        50, 150,  50, 255);  // 草地綠
    uint32_t rockID  = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                       120, 120, 120, 255);  // 石頭灰

    // -------------------------------------------------------
    // 玩家鴨子
    // -------------------------------------------------------
    // 為什麼把玩家也放進 ECS 而不是特殊處理？
    // 這樣 MovementSystem、RenderSystem 等不需要知道哪個是玩家
    // 未來加入多人模式，只需要多建幾個有 InputControlled 的 entity
    auto player = m_registry.create();
    m_registry.addComponent<Transform>(player, 640.0f, 360.0f, 0.0f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(player, duckID, 48.0f, 48.0f, 4, 1.0f, 1.0f, 1.0f, 1.0f);
    m_registry.addComponent<RigidBody>(player, 0.0f, 0.0f, 1.0f, 0.85f);
    m_registry.addComponent<InputControlled>(player);

    // -------------------------------------------------------
    // 靜態場景 — Z-Order 0：草地背景
    // -------------------------------------------------------
    // 靜態物件不需要 RigidBody 和 InputControlled
    // 只要 Transform + Sprite 就能被 RenderSystem 繪製
    auto ground = m_registry.create();
    m_registry.addComponent<Transform>(ground, 640.0f, 630.0f, 0.0f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(ground, grassID, 1280.0f, 200.0f, 0, 1.0f, 1.0f, 1.0f, 1.0f);

    // -------------------------------------------------------
    // 靜態場景 — Z-Order 2：石頭障礙物（有旋轉角度）
    // -------------------------------------------------------
    auto rock1 = m_registry.create();
    m_registry.addComponent<Transform>(rock1, 300.0f, 300.0f, 0.3f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(rock1, rockID, 80.0f, 80.0f, 2, 1.0f, 1.0f, 1.0f, 1.0f);

    auto rock2 = m_registry.create();
    m_registry.addComponent<Transform>(rock2, 900.0f, 250.0f, -0.5f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(rock2, rockID, 60.0f, 60.0f, 2, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Engine::run() {
    Uint64 lastTime = SDL_GetPerformanceCounter();
    float  accumulator = 0.0f;
    const Uint64 freq  = SDL_GetPerformanceFrequency();

    while (!m_input.shouldQuit()) {

        // -------------------------------------------------------
        // 計算 deltaTime（可變時間步）
        // -------------------------------------------------------
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = static_cast<float>(currentTime - lastTime) / static_cast<float>(freq);
        lastTime = currentTime;

        // Spiral of Death 防護
        // 問題：如果某幀花了很長時間（例如載入卡頓），
        //       accumulator 會暴增，導致下一幀瘋狂跑 fixedUpdate 追趕，
        //       然後又卡頓，形成惡性循環（越慢越慢）
        // 解決：把 deltaTime 上限夾在 0.25 秒（4 FPS 的幀時間）
        //       超過就直接丟棄，讓遊戲「假裝時間只過了 0.25 秒」
        if (deltaTime > 0.25f) deltaTime = 0.25f;

        // -------------------------------------------------------
        // 輸入處理
        // -------------------------------------------------------
        m_input.update();

        if (m_input.isKeyPressed(SDL_SCANCODE_ESCAPE)) break;

        // -------------------------------------------------------
        // Fixed Timestep 邏輯更新（60Hz）
        // -------------------------------------------------------
        // 為什麼要用 Fixed Timestep？
        // 物理模擬（速度、碰撞）在不固定的 dt 下會產生數值誤差
        // 例如：在 30 FPS 的機器上，每幀 dt=0.033，兩步就是 0.066
        //       在 60 FPS 的機器上，每幀 dt=0.016，四步是 0.064
        //       微小差異累積會讓物理行為不一致
        // Fixed Timestep 讓物理在所有機器上完全相同
        //
        // 缺點：如果 FIXED_DT 太小（例如 1/120），高負載時跟不上
        // 我們選 1/60 是業界標準，夠精確又不太耗效能
        accumulator += deltaTime;

        while (accumulator >= FIXED_DT) {
            m_movementSystem.update(m_registry, m_input, FIXED_DT);
            accumulator -= FIXED_DT;
        }

        // -------------------------------------------------------
        // 渲染（可變頻率，與 V-Sync 同步）
        // -------------------------------------------------------
        // 渲染不需要固定步長，以最快速度渲染即可
        // V-Sync 在 Window::init() 中已設定（SDL_GL_SetSwapInterval(1)）
        m_renderer.clear({0.15f, 0.15f, 0.2f, 1.0f});  // 深藍灰背景
        m_renderer.begin();
        m_renderSystem.render(m_registry, m_renderer, m_texturePtrs);
        m_renderer.end();

        m_window.swapBuffers();
    }
}

void Engine::shutdown() {
    std::printf("Engine 關閉\n");
}

} // namespace duck
