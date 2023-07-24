#include "gen.hpp"
#include "chunk.hpp"
#include <cmath>

namespace game {

void GenChunkData(Chunk &chunk) {
  auto &data = chunk.GetBlockIdData();

  for (size_t i = 0; i < data.size(); i++) {
    auto pos = Chunk::IndexToPos(i);
    auto worldPos = pos + chunk.GetOffsetPos();

    // sin waves
    float height = 80;
    height += sin(worldPos.x / 10.0f) * 10.0f;
    height += sin(worldPos.y / 10.0f) * 10.0f;

    if (worldPos.z > height) {
      data[i] = BlockId::Air;
    } else if (worldPos.z > height - 1) {
      data[i] = BlockId::Grass;
    } else {
      data[i] = BlockId::Stone;
    }
  }
}

} // namespace game
