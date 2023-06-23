#pragma once

#include "glm/vec2.hpp"
#include <array>

#include "direction.hpp"

namespace game {

enum BlockType {
  AIR = 0,
  DIRT = 1,
  GRASS = 2,
};

struct Block {
  BlockType type;
  glm::vec2 (*GetTextureLoc)(Direction dir);
};

extern std::array<Block, 3> BLOCKS;

}; // namespace game
