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
