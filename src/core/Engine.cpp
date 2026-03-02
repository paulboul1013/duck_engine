#include "core/Engine.h"
#include "ecs/Components.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

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

static uint32_t registerRoundedRectTexture(
    std::unordered_map<uint32_t, std::unique_ptr<Texture>>& store,
    std::unordered_map<uint32_t, Texture*>& ptrs,
    uint32_t& nextID,
    int width,
    int height,
    float radius)
{
    auto tex = std::make_unique<Texture>();
    tex->createRoundedRectMask(width, height, radius);
    uint32_t id = nextID++;
    ptrs[id] = tex.get();
    store[id] = std::move(tex);
    return id;
}

static uint32_t registerRoundedRectGradientTexture(
    std::unordered_map<uint32_t, std::unique_ptr<Texture>>& store,
    std::unordered_map<uint32_t, Texture*>& ptrs,
    uint32_t& nextID,
    int width,
    int height,
    float radius)
{
    auto tex = std::make_unique<Texture>();
    tex->createRoundedRectGradientMask(width, height, radius);
    uint32_t id = nextID++;
    ptrs[id] = tex.get();
    store[id] = std::move(tex);
    return id;
}

static std::string resolveMapPath() {
    namespace fs = std::filesystem;

    const fs::path direct = fs::path("assets") / "maps" / "tutorial_map.json";
    if (fs::exists(direct)) return direct.string();

    const fs::path fromBuild = fs::path("..") / "assets" / "maps" / "tutorial_map.json";
    if (fs::exists(fromBuild)) return fromBuild.string();

    return direct.string();
}

Engine::Engine()
    : Engine(Config{}) {}

Engine::Engine(Config config)
    : m_stressMode(config.stressMode),
      m_infinitePlayerHealth(config.stressMode) {}

bool Engine::init() {
    const char* title = m_stressMode
        ? "Duck Engine - Stress Scene"
        : "Duck Engine - Phase 2";
    if (!m_window.init(title, 1280, 720)) return false;
    if (!m_renderer.init(1280, 720)) return false;

    setupScene();

    // 建立 DebugDraw 用的 1x1 白色紋理
    uint32_t debugTexID = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                          255, 255, 255, 255);
    m_renderSystem.setDebugTexID(debugTexID);
    m_uiRoundedRectTexID = registerRoundedRectTexture(
        m_textureStore, m_texturePtrs, m_nextTextureID, 96, 24, 10.0f);
    m_uiGradientTexID = registerRoundedRectGradientTexture(
        m_textureStore, m_texturePtrs, m_nextTextureID, 96, 24, 10.0f);
    std::printf("F1：切換碰撞框 DebugDraw\n");
    if (m_stressMode) {
        std::printf("Stress mode: ON（大量敵人/障礙物 + 每秒 profiler）\n");
    }

    std::printf("=== Engine 初始化完成 ===\n");
    std::printf("WASD 移動，滑鼠瞄準，左鍵射擊，ESC 退出\n");
    return true;
}

void Engine::setupScene() {
    if (m_stressMode) {
        setupStressScene();
    } else {
        setupStandardScene();
    }
}

void Engine::setupStandardScene() {
    uint32_t duckID   = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        255, 200,   0, 255);  // 鴨子黃
    uint32_t grassID  = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                         50, 150,  50, 255);  // 草地綠
    uint32_t rockID   = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        120, 120, 120, 255);  // 石頭灰
    uint32_t bulletID = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        255, 255, 255, 255);  // 白色（由 Sprite 色彩調變成紅色）
    uint32_t coinID   = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        255, 220,  70, 255);
    uint32_t ammoID   = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        220, 220, 160, 255);
    uint32_t medkitID = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        180, 255, 180, 255);

    MapSceneAssets assets;
    assets.playerTexID = duckID;
    assets.groundTexID = grassID;
    assets.obstacleTexID = rockID;
    assets.bulletTexID = bulletID;
    assets.coinTexID = coinID;
    assets.ammoTexID = ammoID;
    assets.medkitTexID = medkitID;

    std::string mapPath = resolveMapPath();
    if (!m_mapLoader.loadFromFile(mapPath, m_registry, assets)) {
        std::printf("Map 載入失敗: %s\n", mapPath.c_str());
    }
}

void Engine::setupStressScene() {
    uint32_t duckID   = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        255, 200,   0, 255);
    uint32_t grassID  = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                         50, 150,  50, 255);
    uint32_t rockID   = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        120, 120, 120, 255);
    uint32_t bulletID = registerTexture(m_textureStore, m_texturePtrs, m_nextTextureID,
                                        255, 255, 255, 255);

    auto ground = m_registry.create();
    m_registry.addComponent<Transform>(ground, 640.0f, 360.0f, 0.0f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(ground, grassID, 1280.0f, 720.0f, 0, 1.0f, 1.0f, 1.0f, 1.0f);

    auto player = m_registry.create();
    m_registry.addComponent<Transform>(player, 640.0f, 360.0f, 0.0f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(player, duckID, 48.0f, 48.0f, 4, 1.0f, 1.0f, 1.0f, 1.0f);
    m_registry.addComponent<RigidBody>(player, 0.0f, 0.0f, 1.0f, 0.85f);
    m_registry.addComponent<InputControlled>(player);
    m_registry.addComponent<Weapon>(player, bulletID, 900.0f, 0.06f, 0.0f, 2.0f, 10.0f, 1.0f);
    m_registry.addComponent<Collider>(player, Collider::Type::Circle, 24.0f, 24.0f, 24.0f, true);
    m_registry.addComponent<Health>(player, 10.0f, 10.0f);
    m_registry.addComponent<Inventory>(player);

    // 用規則網格生成障礙物，讓 Quadtree broad phase 有明顯切割空間
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 10; ++col) {
            if ((row + col) % 3 == 0) continue;
            auto rock = m_registry.create();
            float x = 110.0f + static_cast<float>(col) * 115.0f;
            float y = 90.0f + static_cast<float>(row) * 78.0f;
            m_registry.addComponent<Transform>(rock, x, y, 0.0f, 1.0f, 1.0f);
            m_registry.addComponent<Sprite>(rock, rockID, 44.0f, 44.0f, 2, 1.0f, 1.0f, 1.0f, 1.0f);
            m_registry.addComponent<Collider>(rock, Collider::Type::AABB, 22.0f, 22.0f, 22.0f, true);
        }
    }

    // 兩圈敵人，讓碰撞、AI、Quadtree 都有壓力
    const int innerRingCount = 24;
    const int outerRingCount = 40;
    for (int i = 0; i < innerRingCount + outerRingCount; ++i) {
        bool outer = i >= innerRingCount;
        int ringIndex = outer ? i - innerRingCount : i;
        int ringCount = outer ? outerRingCount : innerRingCount;
        float radius = outer ? 290.0f : 170.0f;
        float angle = (static_cast<float>(ringIndex) / static_cast<float>(ringCount)) * 6.2831853f;

        auto enemy = m_registry.create();
        float x = 640.0f + std::cos(angle) * radius;
        float y = 360.0f + std::sin(angle) * radius;
        m_registry.addComponent<Transform>(enemy, x, y, 0.0f, 1.0f, 1.0f);
        m_registry.addComponent<Sprite>(enemy, duckID, 40.0f, 40.0f, 4, 0.85f, 0.2f, 0.2f, 1.0f);
        m_registry.addComponent<RigidBody>(enemy, 0.0f, 0.0f, 1.0f, 0.88f);
        m_registry.addComponent<Collider>(enemy, Collider::Type::Circle, 19.0f, 19.0f, 19.0f, true);
        m_registry.addComponent<Health>(enemy, outer ? 4.0f : 3.0f, outer ? 4.0f : 3.0f);

        Enemy enemyData;
        enemyData.detectRange = outer ? 420.0f : 300.0f;
        enemyData.attackRange = 52.0f;
        enemyData.moveAcceleration = outer ? 980.0f : 1180.0f;
        enemyData.patrolRadius = outer ? 70.0f : 48.0f;
        enemyData.patrolAngle = angle;
        m_registry.addComponent<Enemy>(enemy, enemyData);
    }
}

void Engine::printProfilerReport(double elapsedSeconds) {
    if (m_profileFrameCount <= 0) return;

    int enemyCount = 0;
    int bulletCount = 0;
    int solidCount = 0;

    m_registry.view<Enemy>([&](EntityID) { ++enemyCount; });
    m_registry.view<Bullet>([&](EntityID) { ++bulletCount; });
    m_registry.view<Collider>([&](EntityID entity) {
        if (m_registry.getComponent<Collider>(entity).isSolid) ++solidCount;
    });

    double avgFrameMs = m_profileAccumFrameMs / static_cast<double>(m_profileFrameCount);
    double avgRenderMs = m_profileAccumRenderMs / static_cast<double>(m_profileFrameCount);
    double fixedStepsPerFrame = m_profileFrameCount > 0
        ? static_cast<double>(m_profileFixedStepCount) / static_cast<double>(m_profileFrameCount)
        : 0.0;

    double avgEnemyMs = m_profileFixedStepCount > 0
        ? m_profileAccumEnemyMs / static_cast<double>(m_profileFixedStepCount) : 0.0;
    double avgMoveMs = m_profileFixedStepCount > 0
        ? m_profileAccumMovementMs / static_cast<double>(m_profileFixedStepCount) : 0.0;
    double avgWeaponMs = m_profileFixedStepCount > 0
        ? m_profileAccumWeaponMs / static_cast<double>(m_profileFixedStepCount) : 0.0;
    double avgCollisionMs = m_profileFixedStepCount > 0
        ? m_profileAccumCollisionMs / static_cast<double>(m_profileFixedStepCount) : 0.0;

    double fps = elapsedSeconds > 0.0
        ? static_cast<double>(m_profileFrameCount) / elapsedSeconds : 0.0;

    std::printf(
        "[profiler] fps=%.1f frame=%.3fms render=%.3fms fixed/frame=%.2f "
        "enemy=%.3fms move=%.3fms weapon=%.3fms collision=%.3fms "
        "enemies=%d bullets=%d solids=%d\n",
        fps, avgFrameMs, avgRenderMs, fixedStepsPerFrame,
        avgEnemyMs, avgMoveMs, avgWeaponMs, avgCollisionMs,
        enemyCount, bulletCount, solidCount
    );

    m_profileAccumMovementMs = 0.0;
    m_profileAccumWeaponMs = 0.0;
    m_profileAccumEnemyMs = 0.0;
    m_profileAccumCollisionMs = 0.0;
    m_profileAccumRenderMs = 0.0;
    m_profileAccumFrameMs = 0.0;
    m_profileElapsedSeconds = 0.0;
    m_profileFrameCount = 0;
    m_profileFixedStepCount = 0;
}

void Engine::run() {
    Uint64 lastTime = SDL_GetPerformanceCounter();
    float  accumulator = 0.0f;
    const Uint64 freq  = SDL_GetPerformanceFrequency();

    while (!m_input.shouldQuit()) {

        // -------------------------------------------------------
        // 計算 deltaTime（可變時間步）
        // -------------------------------------------------------
        Uint64 frameBegin = SDL_GetPerformanceCounter();
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

        if (m_input.isKeyPressed(SDL_SCANCODE_F1)) {
            m_debugMode = !m_debugMode;
            m_renderSystem.setDebugMode(m_debugMode);
            std::printf("DebugDraw: %s\n", m_debugMode ? "ON" : "OFF");
        }

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
            Uint64 t0 = SDL_GetPerformanceCounter();
            m_enemySystem.update(m_registry, FIXED_DT);
            Uint64 t1 = SDL_GetPerformanceCounter();
            m_movementSystem.update(m_registry, m_input, FIXED_DT);
            Uint64 t2 = SDL_GetPerformanceCounter();
            m_weaponSystem.update(m_registry, m_input, FIXED_DT);
            Uint64 t3 = SDL_GetPerformanceCounter();
            m_pickupSystem.update(m_registry);
            Uint64 tPickup = SDL_GetPerformanceCounter();
            m_collisionSystem.update(m_registry, FIXED_DT);
            Uint64 t4 = SDL_GetPerformanceCounter();

            double counterToMs = 1000.0 / static_cast<double>(freq);
            m_profileAccumEnemyMs += static_cast<double>(t1 - t0) * counterToMs;
            m_profileAccumMovementMs += static_cast<double>(t2 - t1) * counterToMs;
            m_profileAccumWeaponMs += static_cast<double>(tPickup - t2) * counterToMs;
            m_profileAccumCollisionMs += static_cast<double>(t4 - tPickup) * counterToMs;
            ++m_profileFixedStepCount;

            bool playerDead = false;
            m_registry.view<Health, InputControlled>([&](EntityID entity) {
                (void)entity;
                auto& health = m_registry.getComponent<Health>(entity);
                if (m_infinitePlayerHealth) {
                    health.currentHP = health.maxHP;
                }
                if (health.currentHP <= 0.0f) {
                    playerDead = true;
                }
            });
            if (playerDead) {
                std::printf("玩家死亡，遊戲結束\n");
                return;
            }

            accumulator -= FIXED_DT;
        }

        // -------------------------------------------------------
        // 渲染（可變頻率，與 V-Sync 同步）
        // -------------------------------------------------------
        // 渲染不需要固定步長，以最快速度渲染即可
        // V-Sync 在 Window::init() 中已設定（SDL_GL_SetSwapInterval(1)）
        m_renderer.clear({0.15f, 0.15f, 0.2f, 1.0f});  // 深藍灰背景
        Uint64 renderBegin = SDL_GetPerformanceCounter();
        m_renderer.begin();
        m_renderSystem.render(m_registry, m_renderer, m_texturePtrs);

        auto uiIt = m_texturePtrs.find(m_uiRoundedRectTexID);
        auto gradientIt = m_texturePtrs.find(m_uiGradientTexID);
        if (uiIt != m_texturePtrs.end()) {
            float currentHP = 0.0f;
            float maxHP = 1.0f;
            m_registry.view<Health, InputControlled>([&](EntityID entity) {
                auto& health = m_registry.getComponent<Health>(entity);
                currentHP = health.currentHP;
                maxHP = health.maxHP;
            });

            float hpRatio = maxHP > 0.0f ? currentHP / maxHP : 0.0f;
            if (hpRatio < 0.0f) hpRatio = 0.0f;
            if (hpRatio > 1.0f) hpRatio = 1.0f;

            if (!m_healthBarInitialized) {
                m_healthBarLagRatio = hpRatio;
                m_healthBarFlashStartRatio = hpRatio;
                m_healthBarFlashEndRatio = hpRatio;
                m_healthBarInitialized = true;
            }

            if (hpRatio < m_healthBarLagRatio) {
                m_healthBarFlashStartRatio = hpRatio;
                m_healthBarFlashEndRatio = m_healthBarLagRatio;
                m_healthBarFlashTimer = 1.0f;
            }
            m_healthBarLagRatio = hpRatio;
            if (m_healthBarFlashTimer > 0.0f) {
                m_healthBarFlashTimer -= deltaTime;
                if (m_healthBarFlashTimer < 0.0f) m_healthBarFlashTimer = 0.0f;
            }

            const Texture& uiTex = *uiIt->second;
            const Texture* gradientTex = (gradientIt != m_texturePtrs.end()) ? gradientIt->second : nullptr;
            const float left = 24.0f;
            const float top = 22.0f;
            const float barWidth = 220.0f;
            const float barHeight = 28.0f;
            const float bgInset = 2.0f;
            const float bgWidth = barWidth - bgInset * 2.0f;
            const float bgHeight = barHeight - bgInset * 2.0f;
            const float bgCenterX = left + barWidth * 0.5f;
            const float centerY = top + barHeight * 0.5f;

            // 黑底外框
            m_renderer.drawSprite(
                uiTex,
                {bgCenterX, centerY},
                {barWidth, barHeight},
                0.0f,
                {0.03f, 0.03f, 0.04f, 0.98f},
                9
            );

            // 灰色背景底條
            m_renderer.drawSprite(
                uiTex,
                {bgCenterX, centerY},
                {bgWidth, bgHeight},
                0.0f,
                {0.34f, 0.34f, 0.37f, 0.96f},
                10
            );

            if (gradientTex && m_healthBarFlashTimer > 0.0f) {
                float flashRatio = m_healthBarFlashTimer / 1.0f;
                float flashWidth = bgWidth * (m_healthBarFlashEndRatio - m_healthBarFlashStartRatio);
                if (flashWidth > 0.0f) {
                    float flashLeft = (left + bgInset) + bgWidth * m_healthBarFlashStartRatio;
                    float flashCenterX = flashLeft + flashWidth * 0.5f;
                    float flashAlpha = 0.70f * flashRatio;
                    m_renderer.drawSprite(
                        *gradientTex,
                        {flashCenterX, centerY},
                        {flashWidth, bgHeight},
                        0.0f,
                        {1.0f, 0.62f, 0.62f, flashAlpha},
                        11
                    );
                }
            }

            float fillWidth = bgWidth * hpRatio;
            if (fillWidth > 0.0f) {
                float fillCenterX = (left + bgInset) + fillWidth * 0.5f;
                m_renderer.drawSprite(
                    uiTex,
                    {fillCenterX, centerY},
                    {fillWidth, bgHeight},
                    0.0f,
                    {0.88f, 0.14f, 0.12f, 0.98f},
                    12
                );
            }
        }
        m_renderer.end();
        Uint64 renderEnd = SDL_GetPerformanceCounter();

        double counterToMs = 1000.0 / static_cast<double>(freq);
        m_profileAccumRenderMs += static_cast<double>(renderEnd - renderBegin) * counterToMs;

        m_window.swapBuffers();

        Uint64 frameEnd = SDL_GetPerformanceCounter();
        m_profileAccumFrameMs += static_cast<double>(frameEnd - frameBegin) * counterToMs;
        m_profileElapsedSeconds += static_cast<double>(frameEnd - frameBegin) / static_cast<double>(freq);
        ++m_profileFrameCount;

        if (m_profileElapsedSeconds >= 1.0) {
            printProfilerReport(m_profileElapsedSeconds);
        }
    }
}

void Engine::shutdown() {
    std::printf("Engine 關閉\n");
}

} // namespace duck
