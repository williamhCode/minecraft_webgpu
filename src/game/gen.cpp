#include "gen.hpp"
#include "chunk.hpp"
#include <cmath>
#include "PerlinNoise.hpp"
#include "game/block.hpp"

namespace game {

enum Biome {
  Ocean,
  Beach,
  Plains,
};

constexpr int WATER_LEVEL = 64;

void GenChunkData(Chunk &chunk) {
  auto &data = chunk.GetBlockIdData();

  // static const siv::PerlinNoise::seed_type seed = 4;

  static const siv::PerlinNoise biomeNoise{20};
  static const siv::PerlinNoise topLayerNoise{10};

  auto worldOffset = chunk.GetOffsetPos();
  for (int x = 0; x < Chunk::SIZE.x; x++) {
    for (int y = 0; y < Chunk::SIZE.y; y++) {
      auto xyWorld = glm::ivec2(x, y) + glm::ivec2(worldOffset);

      float spread = 150.0;
      int height =
        28 + biomeNoise.octave2D_01(xyWorld.x / spread, xyWorld.y / spread, 4) * 99;
      int topDepth =
        2 + topLayerNoise.octave2D_01(xyWorld.x / 10.0, xyWorld.y / 10.0, 4) * 6;
      int topHeight = height - topDepth;

      Biome biome;
      if (height < WATER_LEVEL) {
        biome = Ocean;
      } else if (height < WATER_LEVEL + 4) {
        biome = Beach;
      } else {
        biome = Plains;
      }

      BlockId topBlock;
      BlockId centerBlock = BlockId::Stone;
      switch (biome) {
      case Ocean:
        topBlock = BlockId::Stone;
        break;
      case Beach:
        topBlock = BlockId::Sand;
        centerBlock = BlockId::Sand;
        break;
      case Plains:
        topBlock = BlockId::Grass;
        centerBlock = BlockId::Dirt;
        break;
      }

      for (int z = 0; z < Chunk::SIZE.z; z++) {
        auto index = Chunk::PosToIndex({x, y, z});

        if (z > height && z <= WATER_LEVEL) {
          data[index] = BlockId::Water;
        } else if (z <= topHeight) {
          data[index] = BlockId::Stone;
        } else if (z <= height - 1) {
          data[index] = centerBlock;
        } else if (z == height) {
          data[index] = topBlock;
        } else {
          data[index] = BlockId::Air;
        }
      }
    }
  }
}

} // namespace game
