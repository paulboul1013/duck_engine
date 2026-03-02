#include "renderer/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace duck {

static void uploadTexturePixels(GLuint& textureID,
                                int width,
                                int height,
                                const std::vector<uint8_t>& pixels) {
    if (!textureID) glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

Texture::~Texture() {
    if (m_textureID) glDeleteTextures(1, &m_textureID);
}

bool Texture::loadFromFile(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);

    int channels;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    if (!data) {
        std::printf("紋理載入失敗: %s\n", path.c_str());
        return false;
    }

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    std::printf("紋理載入成功: %s (%dx%d)\n", path.c_str(), m_width, m_height);
    return true;
}

bool Texture::createSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int w, int h) {
    m_width = w;
    m_height = h;
    std::vector<uint8_t> pixels(w * h * 4);
    for (int i = 0; i < w * h; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = a;
    }
    uploadTexturePixels(m_textureID, w, h, pixels);
    return true;
}

bool Texture::createRoundedRectMask(int width, int height, float radius) {
    m_width = width;
    m_height = height;

    float clampedRadius = std::clamp(radius, 1.0f, 0.5f * static_cast<float>(std::min(width, height)));
    std::vector<uint8_t> pixels(width * height * 4, 255);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float py = static_cast<float>(y) + 0.5f;

            bool insideHorizontal = px >= clampedRadius && px <= static_cast<float>(width) - clampedRadius;
            bool insideVertical = py >= clampedRadius && py <= static_cast<float>(height) - clampedRadius;

            uint8_t alpha = 255;
            if (!insideHorizontal && !insideVertical) {
                float cornerX = (px < clampedRadius) ? clampedRadius : static_cast<float>(width) - clampedRadius;
                float cornerY = (py < clampedRadius) ? clampedRadius : static_cast<float>(height) - clampedRadius;
                float dx = px - cornerX;
                float dy = py - cornerY;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist > clampedRadius) {
                    alpha = 0;
                } else {
                    float edge = std::clamp(clampedRadius - dist, 0.0f, 1.0f);
                    alpha = static_cast<uint8_t>(255.0f * edge);
                }
            }

            size_t i = static_cast<size_t>(y * width + x) * 4;
            pixels[i + 0] = 255;
            pixels[i + 1] = 255;
            pixels[i + 2] = 255;
            pixels[i + 3] = alpha;
        }
    }

    uploadTexturePixels(m_textureID, width, height, pixels);
    return true;
}

bool Texture::createRoundedRectGradientMask(int width, int height, float radius) {
    m_width = width;
    m_height = height;

    float clampedRadius = std::clamp(radius, 1.0f, 0.5f * static_cast<float>(std::min(width, height)));
    std::vector<uint8_t> pixels(width * height * 4, 255);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float py = static_cast<float>(y) + 0.5f;

            bool insideHorizontal = px >= clampedRadius && px <= static_cast<float>(width) - clampedRadius;
            bool insideVertical = py >= clampedRadius && py <= static_cast<float>(height) - clampedRadius;

            float maskAlpha = 1.0f;
            if (!insideHorizontal && !insideVertical) {
                float cornerX = (px < clampedRadius) ? clampedRadius : static_cast<float>(width) - clampedRadius;
                float cornerY = (py < clampedRadius) ? clampedRadius : static_cast<float>(height) - clampedRadius;
                float dx = px - cornerX;
                float dy = py - cornerY;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist > clampedRadius) {
                    maskAlpha = 0.0f;
                } else {
                    maskAlpha = std::clamp(clampedRadius - dist, 0.0f, 1.0f);
                }
            }

            float t = (width > 1) ? static_cast<float>(x) / static_cast<float>(width - 1) : 0.0f;
            float gradientAlpha = 1.0f - t;
            uint8_t alpha = static_cast<uint8_t>(255.0f * maskAlpha * gradientAlpha);

            size_t i = static_cast<size_t>(y * width + x) * 4;
            pixels[i + 0] = 255;
            pixels[i + 1] = 255;
            pixels[i + 2] = 255;
            pixels[i + 3] = alpha;
        }
    }

    uploadTexturePixels(m_textureID, width, height, pixels);
    return true;
}

void Texture::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

} // namespace duck
