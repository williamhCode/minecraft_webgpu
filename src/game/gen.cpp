#include "gen.hpp"
#include "chunk.hpp"
#include <cmath>
#include <random>
#include "PerlinNoise.hpp"
#include "game/block.hpp"

namespace game {

void GenTest(Chunk &chunk);
void GenTerrain(Chunk &chunk);

void GenChunkData(Chunk &chunk) {
  GenTerrain(chunk);
  // GenTest(chunk);
}

void GenTest(Chunk &chunk) {
  auto &data = chunk.GetBlockIdData();

  for (int x = 0; x < Chunk::SIZE.x; x++) {
    for (int y = 0; y < Chunk::SIZE.y; y++) {
      for (int z = 0; z < Chunk::SIZE.z; z++) {
        auto index = Chunk::PosToIndex({x, y, z});
        int height = 100;
        if (z < height) {
          data[index] = BlockId::Dirt;
        } else if (z == height) {
          data[index] = BlockId::Grass;
        } else {
          data[index] = BlockId::Air;
        }
      }
    }
  }
}

enum Biome {
  Ocean,
  Beach,
  Plains,
};
constexpr int WATER_LEVEL = 64;

void Tree(Chunk &chunk, glm::ivec3 pos) {
  auto &data = chunk.GetBlockIdData();

  static std::default_random_engine gen;
  static std::uniform_int_distribution<int> randomInt(4, 7);
  int height = randomInt(gen);

  // generate leaves
  static std::uniform_real_distribution<float> randomFloat(2, 3);
  float radius = randomFloat(gen);
  auto center = pos + glm::ivec3(0, 0, height);
  // for (int x = -radius; x <= radius; x++) {
  //   for (int y = -radius; y <= radius; y++) {
  //     for (int z = -radius; z <= radius; z++) {
  //       auto leafPos = center + glm::ivec3(x, y, z);
  //       auto index = Chunk::PosToIndex(leafPos);
  //       if (!Chunk::ValidIndex(index)) continue;
  //       if (glm::length(glm::vec3(center - leafPos)) <= radius) {
  //         data[index] = BlockId::Leaf;
  //       }
  //     }
  //   }
  // }

  // generate stem
  for (int i = 0; i < height; i++) {
    auto index = Chunk::PosToIndex(pos + glm::ivec3(0, 0, i));
    if (!Chunk::ValidIndex(index)) continue;
    data[index] = BlockId::Wood;
  }
}

void GenTerrain(Chunk &chunk) {
  auto &data = chunk.GetBlockIdData();

  static const siv::PerlinNoise::seed_type seed = 20;
  static const siv::PerlinNoise biomeNoise{seed};
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

      if (biome == Plains) {
        static std::default_random_engine gen;
        std::uniform_real_distribution<float> randomFloat(0, 1);
        if (randomFloat(gen) < 0.005) {
        // if (randomFloat(gen) < 0.05) {
          Tree(chunk, {x, y, height});
        }
      }
    }
  }
}

} // namespace game
