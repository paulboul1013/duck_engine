#include "systems/RenderSystem.h"
#include "ecs/Components.h"
#include "renderer/Texture.h"

namespace duck {

void RenderSystem::render(Registry& registry,
                          Renderer& renderer,
                          const std::unordered_map<uint32_t, Texture*>& textures) {

    // 遍歷所有同時擁有 Transform 和 Sprite 的 entity
    // SpriteBatch 會自動在 Renderer::end() 時依 zOrder 排序後繪製
    // 所以這裡不需要自己排序，順序遍歷即可
    registry.view<Transform, Sprite>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& sp = registry.getComponent<Sprite>(entity);

        // 查詢紋理 ID 對應的 Texture 物件
        auto it = textures.find(sp.textureID);
        if (it == textures.end()) return;  // 找不到紋理就跳過（安全防呆）

        renderer.drawSprite(
            *it->second,
            {tf.x, tf.y},
            // scale 縮放整合進 size，讓 SpriteBatch 不需要知道 scale 的存在
            {sp.width * tf.scaleX, sp.height * tf.scaleY},
            tf.rotation,
            {sp.r, sp.g, sp.b, sp.a},
            sp.zOrder
        );
    });

    // ── DebugDraw：繪製碰撞框輪廓 ──
    if (!m_debugMode) return;

    auto debugIt = textures.find(m_debugTexID);
    if (debugIt == textures.end()) return;
    const Texture& debugTex = *debugIt->second;

    registry.view<Transform, Collider>([&](EntityID entity) {
        auto& tf  = registry.getComponent<Transform>(entity);
        auto& col = registry.getComponent<Collider>(entity);

        // 計算碰撞形狀的外接矩形
        float w, h;
        if (col.type == Collider::Type::Circle) {
            w = h = col.radius * 2.0f;
        } else {
            w = col.halfW * 2.0f;
            h = col.halfH * 2.0f;
        }

        // 顏色：玩家（有 InputControlled）= 藍色, 其他固體 = 紅色
        glm::vec4 debugColor = {1.0f, 0.2f, 0.2f, 0.8f};
        if (registry.hasComponent<InputControlled>(entity)) {
            debugColor = {0.2f, 0.4f, 1.0f, 0.8f};
        }

        const float L = 2.0f;  // 邊框線寬（像素）
        const int Z = 8;       // 最上層

        // 四條邊線：上/下/左/右
        renderer.drawSprite(debugTex, {tf.x, tf.y - h * 0.5f + L * 0.5f}, {w, L}, 0.0f, debugColor, Z);
        renderer.drawSprite(debugTex, {tf.x, tf.y + h * 0.5f - L * 0.5f}, {w, L}, 0.0f, debugColor, Z);
        renderer.drawSprite(debugTex, {tf.x - w * 0.5f + L * 0.5f, tf.y}, {L, h}, 0.0f, debugColor, Z);
        renderer.drawSprite(debugTex, {tf.x + w * 0.5f - L * 0.5f, tf.y}, {L, h}, 0.0f, debugColor, Z);
    });
}

} // namespace duck
