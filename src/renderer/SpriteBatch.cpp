#include "renderer/SpriteBatch.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace duck {

SpriteBatch::~SpriteBatch() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_EBO) glDeleteBuffers(1, &m_EBO);
}

void SpriteBatch::init(int maxSprites) {
    m_maxSprites = maxSprites;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, maxSprites * 4 * sizeof(SpriteVertex), nullptr, GL_DYNAMIC_DRAW);

    // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)0);
    glEnableVertexAttribArray(0);
    // texCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, texCoord));
    glEnableVertexAttribArray(1);
    // color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, color));
    glEnableVertexAttribArray(2);

    // 預先產生 index buffer
    std::vector<GLuint> indices(maxSprites * 6);
    for (int i = 0; i < maxSprites; ++i) {
        int base = i * 4;
        indices[i * 6 + 0] = base + 0;
        indices[i * 6 + 1] = base + 1;
        indices[i * 6 + 2] = base + 2;
        indices[i * 6 + 3] = base + 2;
        indices[i * 6 + 4] = base + 3;
        indices[i * 6 + 5] = base + 0;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    std::printf("SpriteBatch 初始化成功 (最大 %d sprites)\n", maxSprites);
}

void SpriteBatch::begin() {
    m_drawQueue.clear();
}

void SpriteBatch::draw(GLuint textureID, const glm::vec2& pos, const glm::vec2& size,
                       float rotation, const glm::vec4& color, int zOrder) {
    m_drawQueue.push_back({textureID, pos, size, rotation, color, zOrder});
}

void SpriteBatch::end() {
    std::sort(m_drawQueue.begin(), m_drawQueue.end(),
        [](const SpriteDrawCall& a, const SpriteDrawCall& b) {
            if (a.zOrder != b.zOrder) return a.zOrder < b.zOrder;
            return a.textureID < b.textureID;
        });

    m_vertices.clear();
    m_currentTexture = 0;

    for (const auto& sprite : m_drawQueue) {
        if (m_currentTexture != 0 && m_currentTexture != sprite.textureID) {
            flush();
        }
        m_currentTexture = sprite.textureID;
        generateVertices(sprite);
    }

    if (!m_vertices.empty()) {
        flush();
    }
}

void SpriteBatch::flush() {
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(SpriteVertex), m_vertices.data());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_currentTexture);

    int spriteCount = static_cast<int>(m_vertices.size()) / 4;
    glDrawElements(GL_TRIANGLES, spriteCount * 6, GL_UNSIGNED_INT, 0);

    m_vertices.clear();
}

void SpriteBatch::generateVertices(const SpriteDrawCall& sprite) {
    float halfW = sprite.size.x / 2.0f;
    float halfH = sprite.size.y / 2.0f;

    glm::vec2 corners[4] = {
        {-halfW, -halfH},
        { halfW, -halfH},
        { halfW,  halfH},
        {-halfW,  halfH},
    };

    float cosR = std::cos(sprite.rotation);
    float sinR = std::sin(sprite.rotation);

    glm::vec2 texCoords[4] = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    };

    for (int i = 0; i < 4; ++i) {
        glm::vec2 rotated;
        rotated.x = corners[i].x * cosR - corners[i].y * sinR;
        rotated.y = corners[i].x * sinR + corners[i].y * cosR;

        m_vertices.push_back({
            sprite.position + rotated,
            texCoords[i],
            sprite.color
        });
    }
}

} // namespace duck
