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
