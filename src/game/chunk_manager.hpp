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
  static const int RADIUS = 2;
  static const int MAX_GENS = 2;
  wgpu::BindGroupLayout m_offsetLayout;

public:
  std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> chunks;

  ChunkManager() = default;
  ChunkManager(util::Handle *handle, wgpu::BindGroupLayout &layout);
  void Update(glm::vec2 position);
  void Render(wgpu::RenderPassEncoder &passEncoder);

  std::optional<std::tuple<Chunk *, glm::ivec3>> GetChunk(glm::ivec3 position);
  bool HasBlock(glm::ivec3 position);
  void SetBlock(glm::ivec3 position, BlockId blockId);
  void UpdateFace(glm::ivec3 position, Direction direction, bool shouldRender);
};

} // namespace game
