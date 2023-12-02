#include "gen.hpp"
#include "chunk.hpp"
#include "chunk_manager.hpp"
#include <cmath>
#include <random>
#include "glm/gtx/hash.hpp"
#include <unordered_map>
#include "PerlinNoise.hpp"
#include "game/block.hpp"
#include "glm/gtx/string_cast.hpp"
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

void Tree(Chunk &chunk, glm::ivec3 rootPos) {
  auto &data = chunk.GetBlockIdData();

  static std::default_random_engine gen;
  static std::uniform_int_distribution<int> randomInt(4, 7);
  // int height = randomInt(gen);
  int height = 6;

  // generate leaves
  static std::uniform_real_distribution<float> randomFloat(2, 3);
  // float radius = randomFloat(gen);
  float radius = 2;
  float radiusSq = radius * radius;
  auto center = rootPos + glm::ivec3(0, 0, height - 1);

  int radiusInt = std::ceil(radius);
  for (int x = -radiusInt; x <= radiusInt; x++) {
    for (int y = -radiusInt; y <= radiusInt; y++) {
      for (int z = -radiusInt; z <= radiusInt; z++) {
        auto posFromCenter = glm::ivec3(x, y, z);
        auto localPos = center + posFromCenter;
        auto index = Chunk::PosToIndex(localPos);
        // if (glm::length2(glm::vec3(posFromCenter)) > radiusSq) continue;
        if (localPos.z < 0 || localPos.z >= Chunk::SIZE.z) {
          continue;
        } else if (localPos.x < 0 || localPos.x >= Chunk::SIZE.x || localPos.y < 0 || localPos.y >= Chunk::SIZE.y) {
          // if extends out of chunk, store for later use
          auto globalPos = localPos + chunk.GetWorldOffset();
          chunk.outOfBoundLeafPositions.push_back(globalPos);
          continue;
        }
        data[index] = BlockId::Leaf;
      }
    }
  }

  // generate out of bound leaves from neighboring chunks
  // for (int x = -1; x <= 1; x++) {
  //   for (int y = -1; y <= 1; y++) {
  //     auto neighborOffset = chunk.chunkOffset + glm::ivec2(x, y);
  //     auto neighborChunk = chunk.GetChunkManager()->GetChunk(neighborOffset);
  //     if (!neighborChunk) continue;
  //     auto &neighborChunkRef = **neighborChunk;
  //     for (auto &pos : neighborChunkRef.outOfBoundLeafPositions) {
  //       auto localPos = pos - chunk.GetWorldOffset();
  //       auto index = Chunk::PosToIndex(localPos);
  //       data[index] = BlockId::Leaf;
  //     }
  //   }
  // }

  // generate stem
  for (int i = 0; i < height; i++) {
    auto index = Chunk::PosToIndex(rootPos + glm::ivec3(0, 0, i));
    if (!Chunk::ValidIndex(index)) continue;
    data[index] = BlockId::Wood;
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
