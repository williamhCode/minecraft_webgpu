#pragma once

#include <bitset>
#include <iostream>
#include <sys/types.h>
#include <vector>
#include <array>
#include <webgpu/webgpu_cpp.h>
#include "game/direction.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/ext/vector_int3.hpp"
#include "util/webgpu-util.hpp"

#include "gfx/context.hpp"
#include "util/frustum.hpp"
#include "game/block.hpp"
#include "mesh.hpp"
#include <inttypes.h>

// forward decl
struct GameState;

namespace game {

class ChunkManager; // forward dec

template <typename T>
struct BitPackHelper {
  T *data;
  int bitOffset = 0;

  struct Pair {
    T data;
    int bits;
  };

  BitPackHelper(T *data) : data(data) {}

  void Set(T data, int bits) {
    *this->data |= data << bitOffset;
    bitOffset += bits;
  }

  void Set(std::vector<Pair> pairs) {
    for (auto &pair : pairs) {
      Set(pair.data, pair.bits);
    }
  }
};

class Chunk {
public:
  struct VertexAttribs {
    // 0  position (5 bits, 5 bits, 10 bits)
    // 20 uv (1 bit x 2)
    // 22 texLoc (4 bits x 2)
    // 30 transparency (2 bits)
    u_int32_t data1 = 0;
    // 0 normal (2 bit x 3)
    u_int32_t data2 = 0;
  };

  struct Face {
    std::array<VertexAttribs, 4> vertices;
  };

  static constexpr glm::ivec3 SIZE = glm::ivec3(16, 16, 128);
  static constexpr size_t VOLUME = SIZE.x * SIZE.y * SIZE.z;

  bool dirty;
  glm::ivec2 chunkOffset;

  wgpu::Buffer worldPosBuffer;
  wgpu::BindGroup bindGroup;

  std::vector<glm::ivec3> outOfBoundLeafPositions;
private:
  gfx::Context *m_ctx;
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
  MeshData m_translucentData;
  MeshData m_waterData;

  MeshData &GetMeshData(BlockId id) {
    if (id == BlockId::Water) return m_waterData;
    if (g_BLOCK_TYPES[(size_t)id].transparency == 1) return m_translucentData;
    return m_opaqueData;
  }

public:
  Chunk(
    gfx::Context *ctx, GameState *state, ChunkManager *chunkManager, glm::ivec2 offset
  );

  static void InitSharedData();

  void UpdateMesh();
  void Render(const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex);
  void RenderTranslucent(const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex);
  void RenderWater(const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex);

  static size_t PosToIndex(glm::ivec3 pos);
  static glm::ivec3 IndexToPos(size_t index);
  static bool ValidIndex(size_t index);
  bool ShouldRender(BlockId id, glm::ivec3 position, Direction direction);
  bool ShouldRender(BlockId id, glm::ivec3 position);
  bool HasBlock(glm::ivec3 position);
  BlockId GetBlock(glm::ivec3 position);
  void SetBlock(glm::ivec3 position, BlockId blockID);
  auto &GetBlockIdData() {
    return m_blockIdData;
  }
  auto GetWorldOffset() {
    return m_worldOffset;
  }
  util::AABB GetBoundingBox() {
    return util::AABB{m_worldOffset, m_worldOffset + SIZE};
  }
  auto GetChunkManager() {
    return m_chunkManager;
  }
};

}; // namespace game
