#pragma once
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <unordered_set>

namespace duck {

// ============================================================
// Input — 輸入管理器
// ============================================================
// 兩種查詢模式：
//
// 1. Polling（持續偵測）：isKeyDown()
//    「這個鍵現在是否被按住？」
//    用途：移動（WASD）、連發射擊（按住左鍵）
//    每幀都回傳 true，只要按鍵還沒放開
//
// 2. Event（單次觸發）：isKeyPressed()
//    「這個鍵是否在這一幀剛被按下？」
//    用途：換彈（R）、開門、切換武器
//    只在按下的那一幀回傳 true，之後即使按住也回傳 false
//
// 為什麼要分兩種？
// 如果換彈用 isKeyDown()，按住 R 會每幀觸發一次換彈 → 錯誤
// 如果移動用 isKeyPressed()，必須快速連按 W 才能移動 → 不流暢
//
// 實作方式：
// - m_keysDown：目前被按住的所有鍵（SDL_KEYDOWN 加入，SDL_KEYUP 移除）
// - m_keysPressed：本幀剛按下的鍵（每幀開頭清空，SDL_KEYDOWN 且非 repeat 加入）
//
class Input {
public:
    // 每幀呼叫一次，處理所有排隊的 SDL 事件
    void update();

    // Polling：鍵是否被按住？
    bool isKeyDown(SDL_Scancode key) const;

    // Event：鍵是否在本幀剛按下？（不含重複觸發）
    bool isKeyPressed(SDL_Scancode key) const;

    // 滑鼠按鍵是否被按住？
    bool isMouseButtonDown(Uint8 button) const;

    // 滑鼠位置（螢幕座標，左上角 (0,0)）
    glm::vec2 getMousePosition() const;

    // 是否收到 SDL_QUIT（關閉視窗）
    bool shouldQuit() const { return m_quit; }

private:
    // 目前被按住的鍵集合
    // 用 unordered_set 而非 bool 陣列的原因：
    // SDL_Scancode 的值可能很大（最大 ~512），但同時被按住的鍵通常 < 10 個
    // unordered_set 只儲存實際按下的鍵，記憶體更省
    // 查詢也是 O(1)
    std::unordered_set<SDL_Scancode> m_keysDown;

    // 本幀剛按下的鍵（每幀開頭清空）
    std::unordered_set<SDL_Scancode> m_keysPressed;

    // 目前被按住的滑鼠按鍵
    std::unordered_set<Uint8> m_mouseButtonsDown;

    // 滑鼠位置
    glm::vec2 m_mousePos{0.0f};

    // 是否收到關閉事件
    bool m_quit = false;
};

} // namespace duck
