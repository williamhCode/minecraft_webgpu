#include "ray.hpp"
#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include "chunk_manager.hpp"

namespace game {

float ceil(float x) {
  int y = glm::ceil(x);
  if (y == x) {
    return y + 1;
  }
  return y;
}

std::optional<std::tuple<glm::ivec3, game::Direction>> Raycast(
  glm::vec3 origin, glm::vec3 direction, float maxDist, ChunkManager &chunkManager
) {
  direction = glm::normalize(direction);
  glm::ivec3 pos = glm::floor(origin);
  glm::ivec3 step = glm::sign(direction);
  glm::vec3 tMax;
  for (size_t i = 0; i < 3; i++) {
    tMax[i] = (direction[i] > 0 ? ceil(origin[i]) - origin[i]
                                : origin[i] - glm::floor(origin[i])) /
              glm::abs(direction[i]);
  }
  glm::vec3 tDelta = glm::vec3(step) / direction;

  Direction dir;
  while (true) {
    if (chunkManager.HasBlock(pos)) {
      return std::make_tuple(pos, dir);
    }
    if (tMax.x < tMax.y) {
      if (tMax.x < tMax.z) {
        if (tMax.x > maxDist) {
          break;
        }
        pos.x += step.x;
        tMax.x += tDelta.x;
        dir = step.x > 0 ? Direction::WEST : Direction::EAST;
      } else {
        if (tMax.z > maxDist) {
          break;
        }
        pos.z += step.z;
        tMax.z += tDelta.z;
        dir = step.z > 0 ? Direction::BOTTOM : Direction::TOP;
      }
    } else {
      if (tMax.y < tMax.z) {
        if (tMax.y > maxDist) {
          break;
        }
        pos.y += step.y;
        tMax.y += tDelta.y;
        dir = step.y > 0 ? Direction::SOUTH : Direction::NORTH;
      } else {
        if (tMax.z > maxDist) {
          break;
        }
        pos.z += step.z;
        tMax.z += tDelta.z;
        dir = step.z > 0 ? Direction::BOTTOM : Direction::TOP;
      }
    }
  }

  return std::nullopt;
}

} // namespace game
