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
}

} // namespace duck
