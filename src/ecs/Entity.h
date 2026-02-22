#pragma once
#include <cstdint>
#include <limits>

namespace duck {

using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = std::numeric_limits<EntityID>::max();

} // namespace duck
