#include "gen.hpp"
#include "chunk.hpp"
#include <random>
#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include "PerlinNoise.hpp"
#include "game/block.hpp"
#include "glm/gtx/norm.hpp"

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
        int height = 99;
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

void SetBlock(Chunk &chunk, glm::ivec3 pos, BlockId blockId) {
  if (pos.z < 0 || pos.z >= Chunk::SIZE.z) {
    return;
  } else if (pos.x < 0 || pos.x >= Chunk::SIZE.x || pos.y < 0 || pos.y >= Chunk::SIZE.y) {
    // if extends out of chunk, store for later use
    auto globalPos = pos + chunk.GetWorldOffset();
    chunk.outOfBoundLeafPositions.push_back(globalPos);
    return;
  }
  chunk.SetBlock(pos, blockId);
}

void Tree(Chunk &chunk, glm::ivec3 rootPos) {
  auto &data = chunk.GetBlockIdData();

  static std::default_random_engine gen;
  static std::uniform_int_distribution<int> randomInt(4, 7);
  // int height = randomInt(gen);
  int height = 6;

  // generate leaves
  static std::uniform_real_distribution<float> randomFloat(2, 3);
  auto center = rootPos + glm::ivec3(0, 0, height - 2);

  auto layer = [&](int zStart, int height, float radius, float cornerScale) {
    for (int x = -radius; x <= radius; x++) {
      for (int y = -radius; y <= radius; y++) {
        for (int z = 0; z < height; z++) {
          auto posFromCenter = glm::ivec3(x, y, z + zStart);
          auto localPos = center + posFromCenter;

          // some sus math for interpolation between circle and square
          auto xy = glm::vec2(x, y);
          auto normVec = glm::normalize(xy);
          auto circleVec = normVec * radius;
          auto axisVec = glm::vec2(0, 0);
          if (glm::abs(circleVec.x) > glm::abs(circleVec.y)) {
            axisVec.x = glm::sign(circleVec.x);
          } else {
            axisVec.y = glm::sign(circleVec.y);
          }
          float scale = 1 - glm::dot(normVec, axisVec);
          auto finalXy = circleVec * (1 + scale * cornerScale);
          if (glm::length2(xy) > glm::length2(finalXy) + 0.2) continue;

          SetBlock(chunk, localPos, BlockId::Leaf);
        }
      }
    }
  };

  layer(0, 3, 2, 1.0);
  layer(3, 1, 1, 1.0);

  // generate stem
  for (int i = 0; i < height; i++) {
    auto pos = rootPos + glm::ivec3(0, 0, i);
    if (!Chunk::ValidPos(pos)) continue;
    data[Chunk::PosToIndex(pos)] = BlockId::Wood;
  }
}

void GenTerrain(Chunk &chunk) {
  auto &data = chunk.GetBlockIdData();
  chunk.outOfBoundLeafPositions.clear();

  static const siv::PerlinNoise::seed_type seed = 20;
  static const siv::PerlinNoise biomeNoise{seed};
  static const siv::PerlinNoise topLayerNoise{10};
  static const siv::PerlinNoise treeGen{10};

  auto worldOffset = chunk.GetWorldOffset();
  for (int x = 0; x < Chunk::SIZE.x; x++) {
    for (int y = 0; y < Chunk::SIZE.y; y++) {
      auto xyWorld = glm::ivec2(x, y) + glm::ivec2(worldOffset);

      float spread = 150.0;
      int height =
        28 + biomeNoise.octave2D_01(xyWorld.x / spread, xyWorld.y / spread, 4) * 99;
      int topDepth =
        2 + topLayerNoise.octave2D_01(xyWorld.x / 10.0, xyWorld.y / 10.0, 4) * 6;
      int topHeight = height - topDepth;

      float treeChance = treeGen.octave2D_01(xyWorld.x, xyWorld.y, 4);

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
          // data[index] = BlockId::Air;
        }
      }

      if (biome == Plains) {
        if (treeChance > 0.83) {
          Tree(chunk, {x, y, height});
        }
      }
    }
  }
}

} // namespace game
