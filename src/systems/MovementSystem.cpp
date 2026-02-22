#include "systems/MovementSystem.h"
#include "ecs/Components.h"
#include <cmath>

namespace duck {

void MovementSystem::update(Registry& registry, const Input& input, float dt) {

    // -------------------------------------------------------
    // 第一個 view：處理玩家輸入
    // 只有帶有 InputControlled 標記元件的 entity 才受玩家控制
    // 這樣敵人即使有 RigidBody 也不會被這段邏輯影響
    // -------------------------------------------------------
    registry.view<Transform, RigidBody, InputControlled>([&](EntityID entity) {
        auto& rb = registry.getComponent<RigidBody>(entity);
        auto& tf = registry.getComponent<Transform>(entity);

        // 移動速度（像素/秒）
        // 為什麼用 400.0f？
        // 1280 寬的畫面，3.2 秒橫越螢幕，手感測試後的合適值
        const float speed = 1500.0f;

        float inputX = 0.0f, inputY = 0.0f;
        if (input.isKeyDown(SDL_SCANCODE_W)) inputY -= 1.0f;  // Y 軸向上是負
        if (input.isKeyDown(SDL_SCANCODE_S)) inputY += 1.0f;
        if (input.isKeyDown(SDL_SCANCODE_A)) inputX -= 1.0f;
        if (input.isKeyDown(SDL_SCANCODE_D)) inputX += 1.0f;

        // 對角線移動正規化
        // 問題：同時按 W+D，inputX=1, inputY=-1，合速度 = sqrt(2) ≈ 1.41
        // 這會讓對角線移動比直線快 41%，玩家可以利用這個 bug 加速
        // 修正：如果有輸入，把向量正規化回長度 1
        float len = std::sqrt(inputX * inputX + inputY * inputY);
        if (len > 0.0f) {
            inputX /= len;
            inputY /= len;
        }

        // 把輸入方向轉換成速度增量（而不是直接設定速度）
        // 為什麼用 += 而不是 =？
        // 讓 RigidBody 的摩擦力機制能正常運作
        // 如果直接 = speed，每幀都重設速度，摩擦力就沒有效果了
        rb.vx += inputX * speed * dt;
        rb.vy += inputY * speed * dt;

        // 滑鼠朝向：計算從 entity 位置到滑鼠的角度
        // atan2(y, x) 回傳 [-π, π] 的弧度
        // 右方 = 0，上方 = -π/2，左方 = ±π，下方 = π/2
        glm::vec2 mouse = input.getMousePosition();
        tf.rotation = std::atan2(mouse.y - tf.y, mouse.x - tf.x);
    });

    // -------------------------------------------------------
    // 第二個 view：套用物理（速度 → 位置，摩擦力）
    // 所有有 RigidBody 的 entity 都會套用，包括未來的敵人
    // -------------------------------------------------------
    registry.view<Transform, RigidBody>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& rb = registry.getComponent<RigidBody>(entity);

        // 位置更新：x += vx * dt
        // 為什麼乘以 dt（delta time）而不是直接加？
        // 如果直接加：60 FPS 的玩家移動速度是 30 FPS 玩家的兩倍 → 不公平
        // 乘以 dt：速度單位是「像素/秒」，無論幀率都一致
        // 但我們在 GameLoop 用 Fixed Timestep，dt 永遠是 1/60
        // 所以這裡的 dt 其實是常數，乘上去保持概念清晰
        tf.x += rb.vx * dt;
        tf.y += rb.vy * dt;

        // 摩擦力：每幀速度衰減
        // friction = 0.85 代表每幀保留 85% 的速度
        // 在 60 FPS 下，1 秒後剩餘速度 = 0.85^60 ≈ 0.0001（幾乎停止）
        // friction = 0.99 → 1 秒後剩 54%（冰面感）
        // friction = 0.8  → 1 秒後剩 0.0001%（非常黏）
        rb.vx *= rb.friction;
        rb.vy *= rb.friction;

        // 死區（Dead Zone）：消除微小速度的浮點誤差
        // 沒有這個，vx 會無限趨近 0 但永遠不等於 0
        // 每幀都在做無意義的浮點運算
        if (std::abs(rb.vx) < 0.1f) rb.vx = 0.0f;
        if (std::abs(rb.vy) < 0.1f) rb.vy = 0.0f;
    });
}

} // namespace duck
