# Phase 1 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 建立 Duck Engine 核心骨架 — 可 WASD 控制的鴨子 Sprite + 多個靜態場景物件，穩定 60 FPS

**Architecture:** Bottom-Up 建置。先搭 CMake/vcpkg 骨架，接著視窗→渲染器→ECS→輸入→GameLoop→Systems→Demo 整合。每個模組獨立可測試。

**Tech Stack:** C++17, CMake 3.20+, vcpkg, SDL2, OpenGL 3.3 Core, GLAD, glm, stb_image

**系統狀態：** GCC 13.3, CMake 3.28, SDL2 已裝 (apt)。vcpkg 未安裝，glm/stb 未安裝。

---

### Task 1: 安裝 vcpkg 並建立專案骨架

**Files:**
- Create: `vcpkg.json`
- Create: `CMakeLists.txt`
- Create: `src/main.cpp`
- Create: `.gitignore`

**Step 1: 安裝 vcpkg**

```bash
git clone https://github.com/microsoft/vcpkg.git /home/paulboul/vcpkg
/home/paulboul/vcpkg/bootstrap-vcpkg.sh
```

**Step 2: 建立 vcpkg.json**

```json
{
  "name": "duck-engine",
  "version-string": "0.1.0",
  "dependencies": [
    "sdl2",
    "glad",
    "glm",
    "stb"
  ]
}
```

**Step 3: 建立 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(duck_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Stb_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    glad::glad
    glm::glm
)
```

**Step 4: 建立最小 main.cpp**

```cpp
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdio>

int main(int argc, char* argv[]) {
    std::printf("Duck Engine v0.1.0 - 建置成功！\n");
    std::printf("SDL2: OK\n");
    std::printf("GLAD: OK\n");
    std::printf("GLM:  OK\n");
    return 0;
}
```

**Step 5: 建立 .gitignore**

```
build/
.cache/
compile_commands.json
*.o
*.exe
```

**Step 6: 建置並驗證**

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/paulboul/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
./build/duck_engine
```

Expected: 印出 "Duck Engine v0.1.0 - 建置成功！" 及三個 OK

**Step 7: Commit**

```bash
git add vcpkg.json CMakeLists.txt src/main.cpp .gitignore
git commit -m "feat: 建立 CMake/vcpkg 專案骨架，驗證依賴載入"
```

---

### Task 2: SDL2 視窗封裝 (Window)

**Files:**
- Create: `src/platform/Window.h`
- Create: `src/platform/Window.cpp`
- Modify: `src/main.cpp`

**Step 1: 建立 Window.h**

```cpp
#pragma once
#include <SDL2/SDL.h>
#include <string>

namespace duck {

class Window {
public:
    Window() = default;
    ~Window();

    bool init(const std::string& title, int width, int height);
    void swapBuffers();
    bool shouldClose() const;
    void pollEvents();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    SDL_Window* getSDLWindow() const { return m_window; }

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_width = 0;
    int m_height = 0;
    bool m_shouldClose = false;
};

} // namespace duck
```

**Step 2: 建立 Window.cpp**

```cpp
#include "platform/Window.h"
#include <glad/glad.h>
#include <cstdio>

namespace duck {

Window::~Window() {
    if (m_glContext) SDL_GL_DeleteContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Window::init(const std::string& title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::printf("SDL_Init 失敗: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!m_window) {
        std::printf("視窗建立失敗: %s\n", SDL_GetError());
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::printf("OpenGL context 建立失敗: %s\n", SDL_GetError());
        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::printf("GLAD 初始化失敗\n");
        return false;
    }

    SDL_GL_SetSwapInterval(1); // V-Sync

    m_width = width;
    m_height = height;

    std::printf("OpenGL %s\n", glGetString(GL_VERSION));
    std::printf("視窗初始化成功: %dx%d\n", width, height);
    return true;
}

void Window::swapBuffers() {
    SDL_GL_SwapWindow(m_window);
}

bool Window::shouldClose() const {
    return m_shouldClose;
}

void Window::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            m_shouldClose = true;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            m_width = event.window.data1;
            m_height = event.window.data2;
            glViewport(0, 0, m_width, m_height);
        }
    }
}

} // namespace duck
```

**Step 3: 更新 main.cpp 測試視窗**

```cpp
#include "platform/Window.h"
#include <glad/glad.h>

int main(int argc, char* argv[]) {
    duck::Window window;
    if (!window.init("Duck Engine", 1280, 720)) {
        return -1;
    }

    while (!window.shouldClose()) {
        window.pollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.swapBuffers();
    }

    return 0;
}
```

**Step 4: 建置並驗證**

```bash
cmake --build build
./build/duck_engine
```

Expected: 出現一個 1280x720 的深青色視窗，按 X 可正常關閉

**Step 5: Commit**

```bash
git add src/platform/Window.h src/platform/Window.cpp src/main.cpp
git commit -m "feat: 實作 SDL2+OpenGL 視窗封裝 (Window)"
```

---

### Task 3: Shader 編譯管理

**Files:**
- Create: `src/renderer/Shader.h`
- Create: `src/renderer/Shader.cpp`
- Modify: `src/main.cpp` (測試用)

**Step 1: 建立 Shader.h**

```cpp
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace duck {

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool compile(const std::string& vertexSrc, const std::string& fragmentSrc);
    void use() const;

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& v) const;
    void setVec4(const std::string& name, const glm::vec4& v) const;
    void setMat4(const std::string& name, const glm::mat4& m) const;

    GLuint getID() const { return m_programID; }

private:
    GLuint m_programID = 0;

    GLuint compileShader(GLenum type, const std::string& source);
    GLint getUniformLocation(const std::string& name) const;
};

} // namespace duck
```

**Step 2: 建立 Shader.cpp**

```cpp
#include "renderer/Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

namespace duck {

Shader::~Shader() {
    if (m_programID) glDeleteProgram(m_programID);
}

bool Shader::compile(const std::string& vertexSrc, const std::string& fragmentSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!vs || !fs) return false;

    m_programID = glCreateProgram();
    glAttachShader(m_programID, vs);
    glAttachShader(m_programID, fs);
    glLinkProgram(m_programID);

    GLint success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(m_programID, 512, nullptr, log);
        std::printf("Shader link 失敗: %s\n", log);
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    std::printf("Shader 編譯成功 (ID=%u)\n", m_programID);
    return true;
}

void Shader::use() const {
    glUseProgram(m_programID);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& v) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(v));
}

void Shader::setVec4(const std::string& name, const glm::vec4& v) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(v));
}

void Shader::setMat4(const std::string& name, const glm::mat4& m) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        const char* typeName = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        std::printf("%s shader 編譯失敗: %s\n", typeName, log);
        return 0;
    }
    return shader;
}

GLint Shader::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(m_programID, name.c_str());
}

} // namespace duck
```

**Step 3: 更新 main.cpp 測試 shader 編譯**

```cpp
#include "platform/Window.h"
#include "renderer/Shader.h"
#include <glad/glad.h>

static const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* FRAG_SRC = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.8, 0.0, 1.0); // 鴨子黃
}
)";

int main(int argc, char* argv[]) {
    duck::Window window;
    if (!window.init("Duck Engine", 1280, 720)) return -1;

    duck::Shader shader;
    if (!shader.compile(VERT_SRC, FRAG_SRC)) return -1;

    // 畫一個三角形驗證 shader
    float vertices[] = { 0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!window.shouldClose()) {
        window.pollEvents();
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.swapBuffers();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    return 0;
}
```

**Step 4: 建置並驗證**

```bash
cmake --build build
./build/duck_engine
```

Expected: 深青色背景上顯示一個鴨子黃色三角形

**Step 5: Commit**

```bash
git add src/renderer/Shader.h src/renderer/Shader.cpp src/main.cpp
git commit -m "feat: 實作 Shader 編譯管理，驗證三角形繪製"
```

---

### Task 4: Texture 紋理載入

**Files:**
- Create: `src/renderer/Texture.h`
- Create: `src/renderer/Texture.cpp`

**Step 1: 建立 Texture.h**

```cpp
#pragma once
#include <glad/glad.h>
#include <string>

namespace duck {

class Texture {
public:
    Texture() = default;
    ~Texture();

    bool loadFromFile(const std::string& path);
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
```

**Step 2: 建立 Texture.cpp**

```cpp
#include "renderer/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cstdio>

namespace duck {

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

void Texture::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

} // namespace duck
```

**Step 3: 生成測試用紋理（程式產生一個 32x32 棋盤格 PNG）**

在 main.cpp 中暫時加入 Texture 載入測試，或用程式產生一個測試 PNG。先用純色方塊當 placeholder：

```bash
# 用 ImageMagick 產生測試圖（若可用）或在程式碼中直接建立 OpenGL 紋理測試
convert -size 64x64 xc:yellow assets/textures/duck_placeholder.png 2>/dev/null || echo "需手動準備測試圖片"
```

備選方案：在 Texture 類別新增一個 `createSolidColor(r,g,b,a, width, height)` 函式，用程式產生紋理來測試，不依賴外部圖片檔：

```cpp
// 新增到 Texture.h:
bool createSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int width, int height);

// 新增到 Texture.cpp:
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
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return true;
}
```

**Step 4: 建置並驗證**

```bash
cmake --build build
./build/duck_engine
```

Expected: 編譯通過，能載入或程式產生紋理

**Step 5: Commit**

```bash
git add src/renderer/Texture.h src/renderer/Texture.cpp
git commit -m "feat: 實作 Texture 紋理載入 (stb_image + 程式產生)"
```

---

### Task 5: SpriteBatch 批次渲染器

**Files:**
- Create: `src/renderer/SpriteBatch.h`
- Create: `src/renderer/SpriteBatch.cpp`

**Step 1: 建立 SpriteBatch.h**

```cpp
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
```

**Step 2: 建立 SpriteBatch.cpp**

```cpp
#include "renderer/SpriteBatch.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
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

    // 預先產生 index buffer（每個 sprite 6 個 index: 2 個三角形）
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
    // 依 zOrder 排序，同 zOrder 依 textureID 排序（減少切換）
    std::sort(m_drawQueue.begin(), m_drawQueue.end(),
        [](const SpriteDrawCall& a, const SpriteDrawCall& b) {
            if (a.zOrder != b.zOrder) return a.zOrder < b.zOrder;
            return a.textureID < b.textureID;
        });

    m_vertices.clear();
    m_indices.clear();
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
        {-halfW, -halfH}, // bottom-left
        { halfW, -halfH}, // bottom-right
        { halfW,  halfH}, // top-right
        {-halfW,  halfH}, // top-left
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
```

**Step 3: 建置驗證編譯**

```bash
cmake --build build
```

Expected: 編譯通過無錯誤

**Step 4: Commit**

```bash
git add src/renderer/SpriteBatch.h src/renderer/SpriteBatch.cpp
git commit -m "feat: 實作 SpriteBatch 批次渲染器 (Z-Order 排序 + 紋理批次)"
```

---

### Task 6: Renderer 整合層 + Sprite Shader

**Files:**
- Create: `src/renderer/Renderer.h`
- Create: `src/renderer/Renderer.cpp`
- Modify: `src/main.cpp`

**Step 1: 建立 Renderer.h**

```cpp
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
```

**Step 2: 建立 Renderer.cpp**

```cpp
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
    // 正交投影：左上角 (0,0)，右下角 (width, height)
    m_projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
}

} // namespace duck
```

**Step 3: 更新 main.cpp 測試完整渲染管線**

```cpp
#include "platform/Window.h"
#include "renderer/Renderer.h"
#include "renderer/Texture.h"

int main(int argc, char* argv[]) {
    duck::Window window;
    if (!window.init("Duck Engine", 1280, 720)) return -1;

    duck::Renderer renderer;
    if (!renderer.init(1280, 720)) return -1;

    // 程式產生測試紋理
    duck::Texture yellowTex, greenTex, blueTex;
    yellowTex.createSolidColor(255, 200, 0, 255, 4, 4);   // 鴨子黃
    greenTex.createSolidColor(0, 180, 0, 255, 4, 4);       // 地面綠
    blueTex.createSolidColor(50, 50, 200, 255, 4, 4);      // 水面藍

    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.clear({0.1f, 0.1f, 0.1f, 1.0f});
        renderer.begin();

        // Z-Order 0: 地面
        renderer.drawSprite(greenTex, {640.0f, 500.0f}, {800.0f, 200.0f}, 0.0f, {1,1,1,1}, 0);

        // Z-Order 1: 水面
        renderer.drawSprite(blueTex, {300.0f, 400.0f}, {200.0f, 100.0f}, 0.0f, {1,1,1,1}, 1);

        // Z-Order 4: 鴨子（角色層）
        renderer.drawSprite(yellowTex, {640.0f, 360.0f}, {64.0f, 64.0f}, 0.0f, {1,1,1,1}, 4);

        renderer.end();
        window.swapBuffers();
    }
    return 0;
}
```

**Step 4: 建置並驗證**

```bash
cmake --build build
./build/duck_engine
```

Expected: 黑色背景上有綠色地面（底層）、藍色水面（中層）、黃色鴨子方塊（最上層）

**Step 5: Commit**

```bash
git add src/renderer/Renderer.h src/renderer/Renderer.cpp src/main.cpp
git commit -m "feat: 實作 Renderer 整合層 + sprite shader + Z-Order 繪製驗證"
```

---

### Task 7: 手寫 ECS — Entity + ComponentPool

**Files:**
- Create: `src/ecs/Entity.h`
- Create: `src/ecs/ComponentPool.h`
- Create: `src/ecs/Components.h`
- Create: `tests/test_ecs.cpp` (獨立測試程式)

**Step 1: 建立 Entity.h**

```cpp
#pragma once
#include <cstdint>
#include <limits>

namespace duck {

using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = std::numeric_limits<EntityID>::max();

} // namespace duck
```

**Step 2: 建立 ComponentPool.h**

```cpp
#pragma once
#include "ecs/Entity.h"
#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdio>

namespace duck {

// 型別擦除基底類別，讓 Registry 能統一管理不同類型的 Pool
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(EntityID entity) = 0;
    virtual bool has(EntityID entity) const = 0;
};

template <typename T>
class ComponentPool : public IComponentPool {
public:
    T& add(EntityID entity, T component) {
        assert(!has(entity) && "Entity already has this component");
        size_t index = m_components.size();
        m_components.push_back(std::move(component));
        m_indexToEntity.push_back(entity);
        m_entityToIndex[entity] = index;
        return m_components.back();
    }

    T& get(EntityID entity) {
        assert(has(entity) && "Entity does not have this component");
        return m_components[m_entityToIndex[entity]];
    }

    const T& get(EntityID entity) const {
        assert(has(entity) && "Entity does not have this component");
        return m_components[m_entityToIndex.at(entity)];
    }

    void remove(EntityID entity) override {
        if (!has(entity)) return;

        size_t indexToRemove = m_entityToIndex[entity];
        size_t lastIndex = m_components.size() - 1;

        if (indexToRemove != lastIndex) {
            // 把最後一個元素搬到被刪除的位置（保持 dense array 連續）
            m_components[indexToRemove] = std::move(m_components[lastIndex]);
            EntityID lastEntity = m_indexToEntity[lastIndex];
            m_indexToEntity[indexToRemove] = lastEntity;
            m_entityToIndex[lastEntity] = indexToRemove;
        }

        m_components.pop_back();
        m_indexToEntity.pop_back();
        m_entityToIndex.erase(entity);
    }

    bool has(EntityID entity) const override {
        return m_entityToIndex.find(entity) != m_entityToIndex.end();
    }

    size_t size() const { return m_components.size(); }

    // 供 View 遍歷用
    const std::vector<EntityID>& entities() const { return m_indexToEntity; }
    std::vector<T>& components() { return m_components; }

private:
    std::vector<T> m_components;             // dense array（快取友好）
    std::vector<EntityID> m_indexToEntity;    // dense → entity 映射
    std::unordered_map<EntityID, size_t> m_entityToIndex; // entity → dense 映射
};

} // namespace duck
```

**Step 3: 建立 Components.h (Phase 1 元件)**

```cpp
#pragma once
#include <cstdint>

namespace duck {

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct Sprite {
    uint32_t textureID = 0;
    float width = 0.0f;
    float height = 0.0f;
    int zOrder = 0;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

struct RigidBody {
    float vx = 0.0f;
    float vy = 0.0f;
    float mass = 1.0f;
    float friction = 0.9f;
};

struct InputControlled {};

} // namespace duck
```

**Step 4: 建立 ECS 單元測試**

在 CMakeLists.txt 新增一個 test target：

```cmake
# 加到 CMakeLists.txt 最後
add_executable(test_ecs tests/test_ecs.cpp)
target_include_directories(test_ecs PRIVATE ${CMAKE_SOURCE_DIR}/src)
```

```cpp
// tests/test_ecs.cpp
#include "ecs/Entity.h"
#include "ecs/ComponentPool.h"
#include "ecs/Components.h"
#include <cassert>
#include <cstdio>

void test_add_get() {
    duck::ComponentPool<duck::Transform> pool;
    duck::EntityID e1 = 0;
    pool.add(e1, {100.0f, 200.0f, 0.0f, 1.0f, 1.0f});
    assert(pool.has(e1));
    assert(pool.get(e1).x == 100.0f);
    assert(pool.get(e1).y == 200.0f);
    assert(pool.size() == 1);
    std::printf("  [PASS] test_add_get\n");
}

void test_remove() {
    duck::ComponentPool<duck::Transform> pool;
    pool.add(0, {1, 2, 0, 1, 1});
    pool.add(1, {3, 4, 0, 1, 1});
    pool.add(2, {5, 6, 0, 1, 1});

    pool.remove(1); // 移除中間的
    assert(!pool.has(1));
    assert(pool.has(0));
    assert(pool.has(2));
    assert(pool.size() == 2);

    // entity 2 被搬到 index 1 的位置
    assert(pool.get(2).x == 5.0f);
    std::printf("  [PASS] test_remove\n");
}

void test_tag_component() {
    duck::ComponentPool<duck::InputControlled> pool;
    pool.add(42, {});
    assert(pool.has(42));
    assert(!pool.has(0));
    pool.remove(42);
    assert(!pool.has(42));
    std::printf("  [PASS] test_tag_component\n");
}

int main() {
    std::printf("=== ComponentPool 測試 ===\n");
    test_add_get();
    test_remove();
    test_tag_component();
    std::printf("=== 全部通過 ===\n");
    return 0;
}
```

**Step 5: 建置並執行測試**

```bash
cmake --build build
./build/test_ecs
```

Expected:
```
=== ComponentPool 測試 ===
  [PASS] test_add_get
  [PASS] test_remove
  [PASS] test_tag_component
=== 全部通過 ===
```

**Step 6: Commit**

```bash
git add src/ecs/ tests/test_ecs.cpp CMakeLists.txt
git commit -m "feat: 實作手寫 ECS — Entity ID + ComponentPool (dense array) + 單元測試"
```

---

### Task 8: Registry — Entity 管理與 View

**Files:**
- Create: `src/ecs/Registry.h`
- Create: `src/ecs/Registry.cpp`
- Modify: `tests/test_ecs.cpp`

**Step 1: 建立 Registry.h**

```cpp
#pragma once
#include "ecs/Entity.h"
#include "ecs/ComponentPool.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

namespace duck {

class Registry {
public:
    EntityID create();
    void destroy(EntityID entity);
    bool alive(EntityID entity) const;

    template <typename T, typename... Args>
    T& addComponent(EntityID entity, Args&&... args) {
        auto& pool = getOrCreatePool<T>();
        return pool.add(entity, T{std::forward<Args>(args)...});
    }

    template <typename T>
    T& getComponent(EntityID entity) {
        return getPool<T>().get(entity);
    }

    template <typename T>
    bool hasComponent(EntityID entity) const {
        auto it = m_pools.find(std::type_index(typeid(T)));
        if (it == m_pools.end()) return false;
        return static_cast<const ComponentPool<T>*>(it->second.get())->has(entity);
    }

    template <typename T>
    void removeComponent(EntityID entity) {
        getPool<T>().remove(entity);
    }

    // 遍歷擁有所有指定元件的 entity
    template <typename... Ts>
    void view(std::function<void(EntityID)> func) {
        // 找最小的 pool 作為起點
        auto* smallest = findSmallestPool<Ts...>();
        if (!smallest) return;

        for (EntityID entity : smallest->entities()) {
            if ((hasComponent<Ts>(entity) && ...)) {
                func(entity);
            }
        }
    }

private:
    template <typename T>
    ComponentPool<T>& getOrCreatePool() {
        auto key = std::type_index(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            auto& ref = *pool;
            m_pools[key] = std::move(pool);
            return static_cast<ComponentPool<T>&>(ref);
        }
        return static_cast<ComponentPool<T>&>(*it->second);
    }

    template <typename T>
    ComponentPool<T>& getPool() {
        auto key = std::type_index(typeid(T));
        return static_cast<ComponentPool<T>&>(*m_pools.at(key));
    }

    // 找到參數包中 size 最小的 pool
    template <typename First, typename... Rest>
    const ComponentPool<First>* findSmallestPool() {
        auto key = std::type_index(typeid(First));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) return nullptr;
        return static_cast<const ComponentPool<First>*>(it->second.get());
    }

    EntityID m_nextID = 0;
    std::unordered_set<EntityID> m_alive;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
};

} // namespace duck
```

**Step 2: 建立 Registry.cpp**

```cpp
#include "ecs/Registry.h"
#include <cstdio>

namespace duck {

EntityID Registry::create() {
    EntityID id = m_nextID++;
    m_alive.insert(id);
    return id;
}

void Registry::destroy(EntityID entity) {
    for (auto& [type, pool] : m_pools) {
        pool->remove(entity);
    }
    m_alive.erase(entity);
}

bool Registry::alive(EntityID entity) const {
    return m_alive.count(entity) > 0;
}

} // namespace duck
```

**Step 3: 新增 Registry 測試到 test_ecs.cpp**

```cpp
// 新增到 tests/test_ecs.cpp

#include "ecs/Registry.h"

void test_registry_create_destroy() {
    duck::Registry reg;
    auto e1 = reg.create();
    auto e2 = reg.create();
    assert(reg.alive(e1));
    assert(reg.alive(e2));
    reg.destroy(e1);
    assert(!reg.alive(e1));
    assert(reg.alive(e2));
    std::printf("  [PASS] test_registry_create_destroy\n");
}

void test_registry_components() {
    duck::Registry reg;
    auto e = reg.create();
    reg.addComponent<duck::Transform>(e, 10.0f, 20.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(e, 0.0f, 0.0f, 1.0f, 0.9f);

    assert(reg.hasComponent<duck::Transform>(e));
    assert(reg.hasComponent<duck::RigidBody>(e));
    assert(!reg.hasComponent<duck::InputControlled>(e));

    auto& tf = reg.getComponent<duck::Transform>(e);
    assert(tf.x == 10.0f);
    std::printf("  [PASS] test_registry_components\n");
}

void test_registry_view() {
    duck::Registry reg;
    auto e1 = reg.create(); // 有 Transform + RigidBody
    auto e2 = reg.create(); // 只有 Transform
    auto e3 = reg.create(); // 有 Transform + RigidBody

    reg.addComponent<duck::Transform>(e1, 1, 0, 0, 1, 1);
    reg.addComponent<duck::RigidBody>(e1, 0, 0, 1, 0.9f);

    reg.addComponent<duck::Transform>(e2, 2, 0, 0, 1, 1);

    reg.addComponent<duck::Transform>(e3, 3, 0, 0, 1, 1);
    reg.addComponent<duck::RigidBody>(e3, 0, 0, 1, 0.9f);

    std::vector<duck::EntityID> result;
    reg.view<duck::Transform, duck::RigidBody>([&](duck::EntityID e) {
        result.push_back(e);
    });

    assert(result.size() == 2);
    // e1 和 e3 應該在結果中
    bool hasE1 = false, hasE3 = false;
    for (auto id : result) {
        if (id == e1) hasE1 = true;
        if (id == e3) hasE3 = true;
    }
    assert(hasE1 && hasE3);
    std::printf("  [PASS] test_registry_view\n");
}

// 更新 main():
// int main() {
//     std::printf("=== ComponentPool 測試 ===\n");
//     test_add_get();
//     test_remove();
//     test_tag_component();
//     std::printf("\n=== Registry 測試 ===\n");
//     test_registry_create_destroy();
//     test_registry_components();
//     test_registry_view();
//     std::printf("\n=== 全部通過 ===\n");
//     return 0;
// }
```

**Step 4: 建置並執行測試**

```bash
cmake --build build
./build/test_ecs
```

Expected: 6 個 PASS + "全部通過"

**Step 5: Commit**

```bash
git add src/ecs/Registry.h src/ecs/Registry.cpp tests/test_ecs.cpp
git commit -m "feat: 實作 Registry — entity 生命週期管理 + view 多元件查詢 + 測試"
```

---

### Task 9: Input 輸入系統

**Files:**
- Create: `src/platform/Input.h`
- Create: `src/platform/Input.cpp`

**Step 1: 建立 Input.h**

```cpp
#pragma once
#include <SDL2/SDL.h>
#include <unordered_set>
#include <glm/glm.hpp>

namespace duck {

class Input {
public:
    void update();

    bool isKeyDown(SDL_Scancode key) const;
    bool isKeyPressed(SDL_Scancode key) const;
    bool isMouseButtonDown(Uint8 button) const;

    glm::vec2 getMousePosition() const;
    bool shouldQuit() const { return m_quit; }

private:
    std::unordered_set<SDL_Scancode> m_keysDown;
    std::unordered_set<SDL_Scancode> m_keysPressed;   // 本幀剛按下
    std::unordered_set<Uint8> m_mouseButtonsDown;
    glm::vec2 m_mousePos{0.0f};
    bool m_quit = false;
};

} // namespace duck
```

**Step 2: 建立 Input.cpp**

```cpp
#include "platform/Input.h"

namespace duck {

void Input::update() {
    m_keysPressed.clear();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_quit = true;
                break;
            case SDL_KEYDOWN:
                if (!event.key.repeat) {
                    m_keysPressed.insert(event.key.keysym.scancode);
                }
                m_keysDown.insert(event.key.keysym.scancode);
                break;
            case SDL_KEYUP:
                m_keysDown.erase(event.key.keysym.scancode);
                break;
            case SDL_MOUSEBUTTONDOWN:
                m_mouseButtonsDown.insert(event.button.button);
                break;
            case SDL_MOUSEBUTTONUP:
                m_mouseButtonsDown.erase(event.button.button);
                break;
            case SDL_MOUSEMOTION:
                m_mousePos = {(float)event.motion.x, (float)event.motion.y};
                break;
        }
    }
}

bool Input::isKeyDown(SDL_Scancode key) const {
    return m_keysDown.count(key) > 0;
}

bool Input::isKeyPressed(SDL_Scancode key) const {
    return m_keysPressed.count(key) > 0;
}

bool Input::isMouseButtonDown(Uint8 button) const {
    return m_mouseButtonsDown.count(button) > 0;
}

glm::vec2 Input::getMousePosition() const {
    return m_mousePos;
}

} // namespace duck
```

**Step 3: 建置驗證**

```bash
cmake --build build
```

Expected: 編譯通過

**Step 4: Commit**

```bash
git add src/platform/Input.h src/platform/Input.cpp
git commit -m "feat: 實作 Input 輸入系統 (Polling + Event 模式)"
```

---

### Task 10: Engine + GameLoop + MovementSystem + RenderSystem

**Files:**
- Create: `src/systems/MovementSystem.h`
- Create: `src/systems/MovementSystem.cpp`
- Create: `src/systems/RenderSystem.h`
- Create: `src/systems/RenderSystem.cpp`
- Create: `src/core/Engine.h`
- Create: `src/core/Engine.cpp`
- Modify: `src/main.cpp`

**Step 1: 建立 MovementSystem**

```cpp
// src/systems/MovementSystem.h
#pragma once
#include "ecs/Registry.h"
#include "platform/Input.h"

namespace duck {

class MovementSystem {
public:
    void update(Registry& registry, const Input& input, float dt);
};

} // namespace duck
```

```cpp
// src/systems/MovementSystem.cpp
#include "systems/MovementSystem.h"
#include "ecs/Components.h"
#include <cmath>

namespace duck {

void MovementSystem::update(Registry& registry, const Input& input, float dt) {
    // 處理玩家輸入控制的 entity
    registry.view<Transform, RigidBody, InputControlled>([&](EntityID entity) {
        auto& rb = registry.getComponent<RigidBody>(entity);
        auto& tf = registry.getComponent<Transform>(entity);

        const float speed = 400.0f;

        // WASD 輸入
        float inputX = 0.0f, inputY = 0.0f;
        if (input.isKeyDown(SDL_SCANCODE_W)) inputY -= 1.0f;
        if (input.isKeyDown(SDL_SCANCODE_S)) inputY += 1.0f;
        if (input.isKeyDown(SDL_SCANCODE_A)) inputX -= 1.0f;
        if (input.isKeyDown(SDL_SCANCODE_D)) inputX += 1.0f;

        // 正規化對角線移動
        float len = std::sqrt(inputX * inputX + inputY * inputY);
        if (len > 0.0f) {
            inputX /= len;
            inputY /= len;
        }

        rb.vx += inputX * speed * dt;
        rb.vy += inputY * speed * dt;

        // 滑鼠方向 → rotation
        glm::vec2 mouse = input.getMousePosition();
        tf.rotation = std::atan2(mouse.y - tf.y, mouse.x - tf.x);
    });

    // 所有有 RigidBody 的 entity：套用速度和摩擦力
    registry.view<Transform, RigidBody>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& rb = registry.getComponent<RigidBody>(entity);

        tf.x += rb.vx * dt;
        tf.y += rb.vy * dt;

        rb.vx *= rb.friction;
        rb.vy *= rb.friction;

        // 停止微小速度
        if (std::abs(rb.vx) < 0.1f) rb.vx = 0.0f;
        if (std::abs(rb.vy) < 0.1f) rb.vy = 0.0f;
    });
}

} // namespace duck
```

**Step 2: 建立 RenderSystem**

```cpp
// src/systems/RenderSystem.h
#pragma once
#include "ecs/Registry.h"
#include "renderer/Renderer.h"
#include <unordered_map>

namespace duck {

class Texture;

class RenderSystem {
public:
    void render(Registry& registry, Renderer& renderer,
                const std::unordered_map<uint32_t, Texture*>& textures);
};

} // namespace duck
```

```cpp
// src/systems/RenderSystem.cpp
#include "systems/RenderSystem.h"
#include "ecs/Components.h"
#include "renderer/Texture.h"

namespace duck {

void RenderSystem::render(Registry& registry, Renderer& renderer,
                          const std::unordered_map<uint32_t, Texture*>& textures) {
    registry.view<Transform, Sprite>([&](EntityID entity) {
        auto& tf = registry.getComponent<Transform>(entity);
        auto& sp = registry.getComponent<Sprite>(entity);

        auto it = textures.find(sp.textureID);
        if (it == textures.end()) return;

        renderer.drawSprite(
            *it->second,
            {tf.x, tf.y},
            {sp.width * tf.scaleX, sp.height * tf.scaleY},
            tf.rotation,
            {sp.r, sp.g, sp.b, sp.a},
            sp.zOrder
        );
    });
}

} // namespace duck
```

**Step 3: 建立 Engine**

```cpp
// src/core/Engine.h
#pragma once
#include "platform/Window.h"
#include "platform/Input.h"
#include "renderer/Renderer.h"
#include "renderer/Texture.h"
#include "ecs/Registry.h"
#include "systems/MovementSystem.h"
#include "systems/RenderSystem.h"
#include <unordered_map>
#include <memory>

namespace duck {

class Engine {
public:
    bool init();
    void run();
    void shutdown();

private:
    void setupScene();

    Window m_window;
    Input m_input;
    Renderer m_renderer;
    Registry m_registry;

    MovementSystem m_movementSystem;
    RenderSystem m_renderSystem;

    std::unordered_map<uint32_t, std::unique_ptr<Texture>> m_textureStore;
    std::unordered_map<uint32_t, Texture*> m_texturePtrs;
    uint32_t m_nextTextureID = 1;

    static constexpr float FIXED_DT = 1.0f / 60.0f;
};

} // namespace duck
```

```cpp
// src/core/Engine.cpp
#include "core/Engine.h"
#include "ecs/Components.h"
#include <cstdio>

namespace duck {

bool Engine::init() {
    if (!m_window.init("Duck Engine", 1280, 720)) return false;
    if (!m_renderer.init(1280, 720)) return false;

    setupScene();
    std::printf("Engine 初始化完成\n");
    return true;
}

void Engine::setupScene() {
    // 建立紋理
    auto duckTex = std::make_unique<Texture>();
    duckTex->createSolidColor(255, 200, 0, 255, 4, 4);
    uint32_t duckID = m_nextTextureID++;
    m_texturePtrs[duckID] = duckTex.get();
    m_textureStore[duckID] = std::move(duckTex);

    auto grassTex = std::make_unique<Texture>();
    grassTex->createSolidColor(50, 150, 50, 255, 4, 4);
    uint32_t grassID = m_nextTextureID++;
    m_texturePtrs[grassID] = grassTex.get();
    m_textureStore[grassID] = std::move(grassTex);

    auto rockTex = std::make_unique<Texture>();
    rockTex->createSolidColor(120, 120, 120, 255, 4, 4);
    uint32_t rockID = m_nextTextureID++;
    m_texturePtrs[rockID] = rockTex.get();
    m_textureStore[rockID] = std::move(rockTex);

    // 玩家鴨子
    auto player = m_registry.create();
    m_registry.addComponent<Transform>(player, 640.0f, 360.0f, 0.0f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(player, duckID, 48.0f, 48.0f, 4, 1.0f, 1.0f, 1.0f, 1.0f);
    m_registry.addComponent<RigidBody>(player, 0.0f, 0.0f, 1.0f, 0.85f);
    m_registry.addComponent<InputControlled>(player);

    // 靜態場景 — 草地
    auto ground = m_registry.create();
    m_registry.addComponent<Transform>(ground, 640.0f, 600.0f, 0.0f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(ground, grassID, 1280.0f, 200.0f, 0, 1.0f, 1.0f, 1.0f, 1.0f);

    // 靜態場景 — 石頭障礙物
    auto rock1 = m_registry.create();
    m_registry.addComponent<Transform>(rock1, 300.0f, 300.0f, 0.3f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(rock1, rockID, 80.0f, 80.0f, 2, 1.0f, 1.0f, 1.0f, 1.0f);

    auto rock2 = m_registry.create();
    m_registry.addComponent<Transform>(rock2, 900.0f, 250.0f, -0.5f, 1.0f, 1.0f);
    m_registry.addComponent<Sprite>(rock2, rockID, 60.0f, 60.0f, 2, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Engine::run() {
    Uint64 lastTime = SDL_GetPerformanceCounter();
    float accumulator = 0.0f;
    Uint64 freq = SDL_GetPerformanceFrequency();

    while (!m_input.shouldQuit()) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (float)(currentTime - lastTime) / (float)freq;
        lastTime = currentTime;

        // 防止 spiral of death
        if (deltaTime > 0.25f) deltaTime = 0.25f;

        m_input.update();

        // ESC 退出
        if (m_input.isKeyPressed(SDL_SCANCODE_ESCAPE)) break;

        accumulator += deltaTime;

        while (accumulator >= FIXED_DT) {
            m_movementSystem.update(m_registry, m_input, FIXED_DT);
            accumulator -= FIXED_DT;
        }

        m_renderer.clear({0.15f, 0.15f, 0.2f, 1.0f});
        m_renderer.begin();
        m_renderSystem.render(m_registry, m_renderer, m_texturePtrs);
        m_renderer.end();
        m_window.swapBuffers();
    }
}

void Engine::shutdown() {
    std::printf("Engine 關閉\n");
}

} // namespace duck
```

**Step 4: 更新 main.cpp**

```cpp
// src/main.cpp
#include "core/Engine.h"

int main(int argc, char* argv[]) {
    duck::Engine engine;

    if (!engine.init()) return -1;
    engine.run();
    engine.shutdown();

    return 0;
}
```

**Step 5: 建置並驗證**

```bash
cmake --build build
./build/duck_engine
```

Expected:
- 深藍灰色背景
- 底部一條綠色草地
- 兩塊灰色石頭（微旋轉）
- 中間一個黃色鴨子方塊
- WASD 可控制鴨子移動，有摩擦力減速
- 滑鼠移動時鴨子旋轉朝向滑鼠
- ESC 或關閉視窗退出

**Step 6: Commit**

```bash
git add src/core/ src/systems/ src/main.cpp
git commit -m "feat: 實作 Engine/GameLoop/MovementSystem/RenderSystem — Phase 1 完成"
```

---

## 執行完成清單

| Task | 內容 | 驗證方式 |
|------|------|----------|
| 1 | CMake/vcpkg 骨架 | 編譯通過，印出依賴 OK |
| 2 | Window (SDL2+OpenGL) | 視窗開啟，深青色背景 |
| 3 | Shader 編譯 | 黃色三角形繪製 |
| 4 | Texture 紋理載入 | 編譯通過，紋理建立成功 |
| 5 | SpriteBatch | 編譯通過 |
| 6 | Renderer 整合 | 多個 Z-Order sprite 正確顯示 |
| 7 | ECS: Entity + ComponentPool | test_ecs 全部 PASS |
| 8 | ECS: Registry + View | test_ecs 6 個 PASS |
| 9 | Input 系統 | 編譯通過 |
| 10 | Engine 整合 Demo | WASD 控制鴨子 + 靜態場景 + 滑鼠旋轉 + 60FPS |
