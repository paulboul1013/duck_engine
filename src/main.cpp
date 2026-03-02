#include "core/Engine.h"
#include <string_view>

int main(int argc, char* argv[]) {
    duck::Engine::Config config;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--stress") {
            config.stressMode = true;
        }
    }

    duck::Engine engine(config);
    if (!engine.init()) return -1;
    engine.run();
    engine.shutdown();
    return 0;
}
