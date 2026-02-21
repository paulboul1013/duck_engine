#pragma once
#include "renderer/Shader.h"
#include "renderer/SpriteBatch.h"
#include "renderer/Texture.h"
#include <glm/glm.hpp>

namespace duck {

class Renderer {
public:
    Renderer() = default;

    bool init(int screenWidth, int screenHeight);
    void clear(const glm::vec4& color);
    void begin();
    void drawSprite(const Texture& texture, const glm::vec2& pos, const glm::vec2& size,
                    float rotation, const glm::vec4& color, int zOrder);
    void end();

    void setScreenSize(int width, int height);

private:
    Shader m_spriteShader;
    SpriteBatch m_spriteBatch;
    glm::mat4 m_projection{1.0f};
};

} // namespace duck
