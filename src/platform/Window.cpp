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
