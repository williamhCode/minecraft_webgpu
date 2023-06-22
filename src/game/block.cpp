#include "block.hpp"

namespace game {

std::array<Block, 3> BLOCKS = {
  Block{
    .type = BlockType::AIR,
  },
  Block{
    .type = BlockType::DIRT,
  },
  Block{
    .type = BlockType::GRASS,
  },
};

} // namespace game
