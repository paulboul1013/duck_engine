#include "renderer/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

namespace duck {

static const char* SPRITE_VERT = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 Color;

uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
}
)";

static const char* SPRITE_FRAG = R"(
#version 330 core
in vec2 TexCoord;
in vec4 Color;
out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
    FragColor = texture(uTexture, TexCoord) * Color;
}
)";

bool Renderer::init(int screenWidth, int screenHeight) {
    if (!m_spriteShader.compile(SPRITE_VERT, SPRITE_FRAG)) {
        std::printf("Sprite shader 初始化失敗\n");
        return false;
    }

    m_spriteBatch.init(1000);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setScreenSize(screenWidth, screenHeight);

    std::printf("Renderer 初始化成功\n");
    return true;
}

void Renderer::clear(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::begin() {
    m_spriteShader.use();
    m_spriteShader.setMat4("uProjection", m_projection);
    m_spriteShader.setInt("uTexture", 0);
    m_spriteBatch.begin();
}

void Renderer::drawSprite(const Texture& texture, const glm::vec2& pos, const glm::vec2& size,
                          float rotation, const glm::vec4& color, int zOrder) {
    m_spriteBatch.draw(texture.getID(), pos, size, rotation, color, zOrder);
}

void Renderer::end() {
    m_spriteBatch.end();
}

void Renderer::setScreenSize(int width, int height) {
    m_projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
}

} // namespace duck
