#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace duck {

struct SpriteVertex {
    glm::vec2 position;
    glm::vec2 texCoord;
    glm::vec4 color;
};

struct SpriteDrawCall {
    GLuint textureID;
    glm::vec2 position;
    glm::vec2 size;
    float rotation;
    glm::vec4 color;
    int zOrder;
};

class SpriteBatch {
public:
    SpriteBatch() = default;
    ~SpriteBatch();

    void init(int maxSprites = 1000);
    void begin();
    void draw(GLuint textureID, const glm::vec2& pos, const glm::vec2& size,
              float rotation, const glm::vec4& color, int zOrder);
    void end();

private:
    void flush();
    void generateVertices(const SpriteDrawCall& sprite);

    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;

    std::vector<SpriteDrawCall> m_drawQueue;
    std::vector<SpriteVertex> m_vertices;
    std::vector<GLuint> m_indices;

    int m_maxSprites = 0;
    GLuint m_currentTexture = 0;
};

} // namespace duck
