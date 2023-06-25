#pragma once

#include <vector>
#include <array>
#include <webgpu/webgpu_cpp.h>
#include "glm/vec3.hpp"

#include "util/handle.hpp"
#include "game/block.hpp"
#include "mesh.hpp"
#include <inttypes.h>

namespace game {

class Chunk {
public:
  Chunk(util::Handle *handle);
  void SetBlock(glm::vec3, BlockId blockID);
  void Render(const wgpu::RenderPassEncoder& passEncoder);
  wgpu::Buffer GetVertexBuffer();
  wgpu::Buffer GetIndexBuffer();

private:
  util::Handle *m_handle;
  static constexpr const glm::ivec3 SIZE = glm::ivec3(16, 16, 128);
  static constexpr const size_t VOLUME = SIZE.x * SIZE.y * SIZE.z;
  static constexpr const glm::vec3 POS_OFFSETS[6] = {
    {0,  1,  0},
    {0, -1,  0},
    {1,  0,  0},
    {-1, 0,  0},
    {0,  0,  1},
    {0,  0, -1},
  };
  bool m_dirty;

  std::array<BlockId, VOLUME> m_blockData;
  std::vector<Face> m_faces;
  std::vector<FaceIndex> m_indices;

  wgpu::Buffer m_vertexBuffer;
  wgpu::Buffer m_indexBuffer;

  void InitializeChunkData();
  void CreateBuffers();
  void GenerateMesh();
  size_t PosToIndex(glm::ivec3 pos);
  glm::ivec3 IndexToPos(size_t index);
};

}; // namespace game
