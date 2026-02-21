#include "platform/Window.h"
#include "renderer/Renderer.h"
#include "renderer/Texture.h"

int main(int argc, char* argv[]) {
    duck::Window window;
    if (!window.init("Duck Engine", 1280, 720)) return -1;

    duck::Renderer renderer;
    if (!renderer.init(1280, 720)) return -1;

    duck::Texture yellowTex, greenTex, blueTex;
    yellowTex.createSolidColor(255, 200, 0, 255, 4, 4);
    greenTex.createSolidColor(0, 180, 0, 255, 4, 4);
    blueTex.createSolidColor(50, 50, 200, 255, 4, 4);

    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.clear({0.1f, 0.1f, 0.1f, 1.0f});
        renderer.begin();

        renderer.drawSprite(greenTex, {640.0f, 500.0f}, {800.0f, 200.0f}, 0.0f, {1,1,1,1}, 0);
        renderer.drawSprite(blueTex, {300.0f, 400.0f}, {200.0f, 100.0f}, 0.0f, {1,1,1,1}, 1);
        renderer.drawSprite(yellowTex, {640.0f, 360.0f}, {64.0f, 64.0f}, 0.0f, {1,1,1,1}, 4);

        renderer.end();
        window.swapBuffers();
    }
    return 0;
}
