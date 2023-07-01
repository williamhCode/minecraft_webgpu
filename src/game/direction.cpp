#include "direction.hpp"

namespace game {

const glm::ivec3 g_DIR_OFFSETS[6] = {
  {0,  1,  0},
  {0, -1,  0},
  {1,  0,  0},
  {-1, 0,  0},
  {0,  0,  1},
  {0,  0, -1},
};

Direction DirOpposite(Direction direction) {
  switch (direction) {
    case Direction::NORTH:
      return Direction::SOUTH;
    case Direction::SOUTH:
      return Direction::NORTH;
    case Direction::EAST:
      return Direction::WEST;
    case Direction::WEST:
      return Direction::EAST;
    case Direction::TOP:
      return Direction::BOTTOM;
    case Direction::BOTTOM:
      return Direction::TOP;
  }
}

} // namespace game
