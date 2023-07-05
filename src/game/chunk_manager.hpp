#pragma once

#include "game/chunk.hpp"
#include "glm/ext/vector_float3.hpp"
#include <glm/gtx/hash.hpp>
#include "util/context.hpp"
#include <unordered_map>
#include <vector>

namespace game {

class ChunkManager {
private:
  util::Context *m_ctx;
  static const int RADIUS = 2;
  static const int MAX_GENS = 2;
  wgpu::BindGroupLayout m_offsetLayout;

public:
  std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> chunks;

  ChunkManager() = default;
  ChunkManager(util::Context *ctx);
  void Update(glm::vec2 position);
  void Render(wgpu::RenderPassEncoder &passEncoder);

  std::optional<std::tuple<Chunk *, glm::ivec3>> GetChunk(glm::ivec3 position);
  bool HasBlock(glm::ivec3 position);
  void SetBlock(glm::ivec3 position, BlockId blockId);
  void UpdateFace(glm::ivec3 position, Direction direction, bool shouldRender);
};

} // namespace game
