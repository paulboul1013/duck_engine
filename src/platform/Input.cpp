#include "platform/Input.h"

namespace duck {

void Input::update() {
    // 每幀開頭清空「本幀剛按下」的集合
    // 這是 isKeyPressed() 只在按下那一幀回傳 true 的關鍵
    m_keysPressed.clear();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {

            case SDL_QUIT:
                m_quit = true;
                break;

            case SDL_KEYDOWN:
                // event.key.repeat：SDL2 會在長按時自動重複觸發 KEYDOWN
                // 我們只要「第一次按下」，不要重複觸發
                // 否則 isKeyPressed() 會在長按時每隔一段時間回傳 true
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
                m_mousePos = {
                    static_cast<float>(event.motion.x),
                    static_cast<float>(event.motion.y)
                };
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
