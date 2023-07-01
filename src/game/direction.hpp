#pragma once

#include "glm/ext/vector_int3.hpp"
namespace game {

enum Direction {
  NORTH,
  SOUTH,
  EAST,
  WEST,
  TOP,
  BOTTOM,
};

extern const glm::ivec3 g_DIR_OFFSETS[6];

Direction DirOpposite(Direction direction);

} // namespace game
