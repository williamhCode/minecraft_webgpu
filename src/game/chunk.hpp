#pragma once

#include <bitset>
#include <vector>
#include <array>
#include <webgpu/webgpu_cpp.h>
#include "game/direction.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/ext/vector_int3.hpp"
#include "util/webgpu-util.hpp"

#include "util/context.hpp"
#include "util/frustum.hpp"
#include "game/block.hpp"
#include "mesh.hpp"
#include <inttypes.h>

// forward decl
struct GameState;

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
  GameState *m_state;

  ChunkManager *m_chunkManager;
  glm::ivec3 m_worldOffset;

  // shared chunk data, precomputed positions, normals, and uv
  static std::array<Cube, VOLUME> m_cubeData;

  std::array<BlockId, VOLUME> m_blockIdData; // block data

  struct MeshData {
    std::vector<Face> faces;
    std::vector<FaceIndex> indices;
    wgpu::Buffer vbo;
    wgpu::Buffer ebo;
    size_t faceNum;

    void Clear() {
      faces.clear();
      indices.clear();
      faceNum = 0;
    }

    void AddFace(Face face, FaceIndex index) {
      faces.push_back(face);
      indices.push_back(index);
      faceNum++;
    }

    void CreateBuffers(wgpu::Device &device) {
      vbo = util::CreateVertexBuffer(device, faces.size() * sizeof(Face), faces.data());
      ebo = util::CreateIndexBuffer(
        device, indices.size() * sizeof(FaceIndex), indices.data()
      );
    }
  };

  MeshData m_opaqueData;
  MeshData m_transparentData;
  MeshData m_waterData;

  MeshData &GetMeshData(BlockId id) {
    if (id == BlockId::Water) return m_waterData;
    if (g_BLOCK_TYPES[(size_t)id].transparent) return m_transparentData;
    return m_opaqueData;
  }

public:
  Chunk(
    util::Context *ctx, GameState *state, ChunkManager *chunkManager, glm::ivec2 offset
  );
  static void InitSharedData();

  void UpdateMesh();
  void Render(const wgpu::RenderPassEncoder &passEncoder);
  void RenderTransparent(const wgpu::RenderPassEncoder &passEncoder);
  void RenderWater(const wgpu::RenderPassEncoder &passEncoder);

  static size_t PosToIndex(glm::ivec3 pos);
  static glm::ivec3 IndexToPos(size_t index);
  bool ShouldRender(BlockId id, glm::ivec3 position, Direction direction);
  bool ShouldRender(BlockId id, glm::ivec3 position);
  bool HasBlock(glm::ivec3 position);
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
