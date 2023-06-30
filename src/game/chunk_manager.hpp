#pragma once

#include "game/chunk.hpp"
#include "glm/ext/vector_float3.hpp"
#include <glm/gtx/hash.hpp>
#include "util/handle.hpp"
#include <unordered_map>
#include <vector>

namespace game {

class ChunkManager {
private:
  util::Handle *m_handle;
  const int RADIUS = 2;
  int MAX_GENS = 2;
  wgpu::BindGroupLayout m_offsetLayout;

public:
  ChunkManager(util::Handle *handle, wgpu::BindGroupLayout &layout);
  void Update(glm::vec2 position);
  void Render(wgpu::RenderPassEncoder &passEncoder);
  std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> chunks;
};

} // namespace game
