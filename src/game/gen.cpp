#include "gen.hpp"
#include "chunk.hpp"
#include <cmath>
#include "PerlinNoise.hpp"

namespace game {

void GenChunkData(Chunk &chunk) {
  auto &data = chunk.GetBlockIdData();

  static const siv::PerlinNoise::seed_type seed = 10u;
  static const siv::PerlinNoise perlin{seed};

  auto worldOffset = chunk.GetOffsetPos();
  for (int x = 0; x < Chunk::SIZE.x; x++) {
    for (int y = 0; y < Chunk::SIZE.y; y++) {
      int xPos = worldOffset.x + x;
      int yPos = worldOffset.y + y;
      float spread = 200.0;
      int octaves = 4;
      int height = 80 + perlin.octave2D_01(xPos / spread, yPos / spread, octaves) * 40;

      for (int z = 0; z < Chunk::SIZE.z; z++) {
        auto index = Chunk::PosToIndex({x, y, z});
        if (z < height) {
          data[index] = BlockId::Stone;
        } else if (z == height) {
          data[index] = BlockId::Grass;
        } else {
          data[index] = BlockId::Air;
        }
      }
    }
  }
}

} // namespace game
