#include "systems/WeaponSystem.h"
#include "ecs/Components.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace duck {

void WeaponSystem::update(Registry& registry, const Input& input, float dt) {

    // -------------------------------------------------------
    // View 1：射擊 — 只有 InputControlled entity 能開槍
    // -------------------------------------------------------
    registry.view<Transform, Weapon, InputControlled>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& wp = registry.getComponent<Weapon>(entity);

        // 冷卻倒數（不管有沒有按鍵都在計時）
        if (wp.cooldown > 0.0f) wp.cooldown -= dt;

        // 按住左鍵 + 冷卻結束 → 射出一顆子彈
        if (input.isMouseButtonDown(SDL_BUTTON_LEFT) && wp.cooldown <= 0.0f) {

            // 從玩家位置指向滑鼠的方向向量
            glm::vec2 mouse = input.getMousePosition();
            float dx = mouse.x - tf.x;
            float dy = mouse.y - tf.y;

            // 正規化方向向量（長度 → 1）
            // 防止斜向子彈比直向快（和移動的對角線問題一樣）
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0.0f) { dx /= len; dy /= len; }

            // 在玩家位置生成子彈 entity
            auto bullet = registry.create();
            registry.addComponent<Transform>(bullet, tf.x, tf.y, 0.0f, 1.0f, 1.0f);
            registry.addComponent<Sprite>(bullet,
                wp.bulletTextureID,
                wp.bulletSize, wp.bulletSize,
                5,              // Z-Order 5：在角色(4)上方
                1.0f, 0.0f, 0.0f, 1.0f);  // 紅色
            registry.addComponent<Bullet>(bullet,
                dx * wp.bulletSpeed,
                dy * wp.bulletSpeed,
                wp.bulletLifetime);

            // 重置冷卻，防止下幀立刻再射
            wp.cooldown = wp.fireRate;
        }
    });

    // -------------------------------------------------------
    // View 2：子彈移動 + 過期清除
    // -------------------------------------------------------
    // 子彈等速直線飛行：不乘 friction，不會減速
    // view() 在呼叫前複製 entity 列表，所以在 callback 裡 destroy() 是安全的
    registry.view<Transform, Bullet>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& bl = registry.getComponent<Bullet>(entity);

        // 等速位移
        tf.x += bl.vx * dt;
        tf.y += bl.vy * dt;

        // 壽命倒數，歸零就清除
        bl.lifetime -= dt;
        if (bl.lifetime <= 0.0f) {
            registry.destroy(entity);
        }
    });
}

} // namespace duck
