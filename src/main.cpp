#include "core/Engine.h"

int main(int argc, char* argv[]) {
    duck::Engine engine;
    if (!engine.init()) return -1;
    engine.run();
    engine.shutdown();
    return 0;
}
