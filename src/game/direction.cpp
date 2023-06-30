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

} // namespace game
