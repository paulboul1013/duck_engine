#pragma once
#include "ecs/Registry.h"
#include <cstdint>
#include <string>

namespace duck {

struct MapSceneAssets {
    uint32_t playerTexID = 0;
    uint32_t groundTexID = 0;
    uint32_t obstacleTexID = 0;
    uint32_t bulletTexID = 0;
    uint32_t coinTexID = 0;
    uint32_t ammoTexID = 0;
    uint32_t medkitTexID = 0;
};

class MapLoader {
public:
    bool loadFromFile(const std::string& path,
                      Registry& registry,
                      const MapSceneAssets& assets) const;
};

} // namespace duck
