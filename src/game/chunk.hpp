#pragma once

#include <bitset>
#include <vector>
#include <array>
#include <webgpu/webgpu_cpp.h>
#include "game/direction.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/vec3.hpp"

#include "util/handle.hpp"
#include "game/block.hpp"
#include "mesh.hpp"
#include <inttypes.h>

namespace game {

class ChunkManager; // forward dec

class Chunk {
public:
  static constexpr glm::ivec3 SIZE = glm::ivec3(16, 16, 128);
  static constexpr size_t VOLUME = SIZE.x * SIZE.y * SIZE.z;

private:
  util::Handle *m_handle;
  ChunkManager *m_chunkManager;
  // glm::ivec2 m_offset;
  glm::ivec3 m_offsetPos;

  // shared chunk data, precomputed positions, normals, and uv
  static std::array<Cube, VOLUME> m_cubeData;

  bool m_dirty;
  std::array<BlockId, VOLUME> m_blockIdData;  // block data
  // records if faces should be rendered
  std::array<std::bitset<6>, VOLUME> m_faceRenderData;

  std::vector<Face> m_faces;
  std::vector<FaceIndex> m_indices;

  wgpu::Buffer m_vertexBuffer;
  wgpu::Buffer m_indexBuffer;

  wgpu::Buffer m_offsetBuffer;
  wgpu::BindGroup m_bindGroup;

public:
  Chunk(
    util::Handle *handle,
    ChunkManager *chunkManager,
    glm::ivec2 offset
  );
  static void InitSharedData();

  void InitializeChunkData();
  void InitFaceData();
  void CreateBuffers();
  void UpdateMesh();
  void Render(const wgpu::RenderPassEncoder& passEncoder);

  std::array<std::bitset<6>, VOLUME> &GetFaceRenderData() {
    return m_faceRenderData;
  }

  static size_t PosToIndex(glm::ivec3 pos);
  static glm::ivec3 IndexToPos(size_t index);
  bool HasNeighbor(glm::ivec3 position, Direction direction);
  bool HasBlock(glm::ivec3 position);
  BlockId GetBlock(glm::ivec3 position);
  void SetBlock(glm::ivec3 position, BlockId blockID);
  void UpdateFace(glm::ivec3 position, Direction direction, bool shouldRender);
};

}; // namespace game
