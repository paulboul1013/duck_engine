#pragma once
#include "ecs/Registry.h"
#include "renderer/Renderer.h"
#include <unordered_map>
#include <cstdint>

namespace duck {

class Texture;

// ============================================================
// RenderSystem — 把 ECS 資料轉換成繪製指令
// ============================================================
// 職責：
// 遍歷所有同時擁有 Transform 和 Sprite 的 entity，
// 從紋理表取出對應 Texture，呼叫 Renderer::drawSprite()。
//
// 為什麼用 textures map 而不是讓 Sprite 直接持有 Texture 指標？
//
// 方案A（不好）：Sprite { Texture* tex; ... }
// - entity 直接持有資源指標，若紋理重新載入指標就失效
// - Texture 的生命週期難以管理
//
// 方案B（我們的做法）：Sprite { uint32_t textureID; ... }
// - ID 是穩定的數字，不受記憶體移動影響
// - 紋理由 Engine 統一管理（textureStore），可以熱重載
// - RenderSystem 只需要一個「ID → Texture*」的查詢表
//
class RenderSystem {
public:
    void render(Registry& registry,
                Renderer& renderer,
                const std::unordered_map<uint32_t, Texture*>& textures);
};

} // namespace duck
