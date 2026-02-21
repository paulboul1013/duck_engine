#pragma once
#include <glad/glad.h>
#include <string>
#include <cstdint>
#include <vector>

namespace duck {

class Texture {
public:
    Texture() = default;
    ~Texture();

    bool loadFromFile(const std::string& path);
    bool createSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int width, int height);
    void bind(int slot = 0) const;

    GLuint getID() const { return m_textureID; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    GLuint m_textureID = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace duck
