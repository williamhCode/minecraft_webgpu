#pragma once

#include <vector>
#include <webgpu/webgpu_cpp.h>
#include "glm/vec3.hpp"

#include "util/handle.hpp"
#include "game/block.hpp"

namespace game {

class Chunk {
public:
  Chunk(util::Handle &handle, glm::vec3 size);
  void SetBlock(glm::vec3, Block blockType);
  void Render(const wgpu::RenderPassEncoder& passEncoder);

private:
  util::Handle &m_handle;
  glm::vec3 m_size;
  bool m_dirty;

  std::vector<Block> blockData;

  wgpu::Buffer m_vertexBuffer;
  wgpu::Buffer m_indexBuffer;

  void InitializeChunkData();
  void CreateBuffers();
  void GenerateMesh();
};

}; // namespace game
