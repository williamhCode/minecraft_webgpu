#pragma once

#include "game/chunk.hpp"
#include "game/direction.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_float3.hpp"
#include <optional>
#include <tuple>

namespace game {

std::optional<std::tuple<glm::ivec3, game::Direction>>
Raycast(glm::vec3 origin, glm::vec3 direction, float maxDist, ChunkManager &chunkManager);

} // namespace game
