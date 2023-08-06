#pragma once

#include <bitset>
#include <vector>
#include <array>
#include <webgpu/webgpu_cpp.h>
#include "game/direction.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/ext/vector_int3.hpp"

#include "util/context.hpp"
#include "util/frustum.hpp"
#include "game/block.hpp"
#include "mesh.hpp"
#include <inttypes.h>

namespace game {

class ChunkManager; // forward dec

class Chunk {
public:
  static constexpr glm::ivec3 SIZE = glm::ivec3(16, 16, 128);
  static constexpr size_t VOLUME = SIZE.x * SIZE.y * SIZE.z;

  bool dirty;
  glm::ivec2 chunkOffset;

private:
  util::Context *m_ctx;
  ChunkManager *m_chunkManager;
  glm::ivec3 m_worldOffset;

  // shared chunk data, precomputed positions, normals, and uv
  static std::array<Cube, VOLUME> m_cubeData;

  std::array<BlockId, VOLUME> m_blockIdData; // block data

  std::vector<Face> m_faces;
  std::vector<FaceIndex> m_indices;
  wgpu::Buffer m_vertexBuffer;
  wgpu::Buffer m_indexBuffer;

  std::vector<Face> m_waterFaces;
  std::vector<FaceIndex> m_waterIndices;
  wgpu::Buffer m_waterVbo;
  wgpu::Buffer m_waterEbo;

public:
  Chunk(util::Context *ctx, ChunkManager *chunkManager, glm::ivec2 offset);
  static void InitSharedData();

  void CreateBuffers();
  void UpdateBuffers();
  void UpdateMesh();
  void Render(const wgpu::RenderPassEncoder &passEncoder);
  void RenderWater(const wgpu::RenderPassEncoder &passEncoder);

  static size_t PosToIndex(glm::ivec3 pos);
  static glm::ivec3 IndexToPos(size_t index);
  bool ShouldRender(glm::ivec3 position, Direction direction);
  bool WaterShouldRender(glm::ivec3 position, Direction direction);
  // bool HasNeighbor(glm::ivec3 position, Direction direction);
  bool HasBlock(glm::ivec3 position);
  bool WaterHasBlock(glm::ivec3 position);
  BlockId GetBlock(glm::ivec3 position);
  void SetBlock(glm::ivec3 position, BlockId blockID);
  auto &GetBlockIdData() {
    return m_blockIdData;
  }
  auto GetOffsetPos() {
    return m_worldOffset;
  }
  util::AABB GetBoundingBox() {
    return util::AABB{m_worldOffset, m_worldOffset + SIZE};
  }
};

}; // namespace game
