#pragma once

#include "game/chunk.hpp"
#include "glm/ext/vector_float3.hpp"
#include <glm/gtx/hash.hpp>
#include "util/context.hpp"
#include <unordered_map>
#include <vector>

// forward decl
struct GameState;

namespace game {

class ChunkManager {
private:
  util::Context *m_ctx;
  GameState *m_state;

public:
  int radius = 32;
  int max_gens = 4;
  std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> chunks;

  ChunkManager() = default;
  ChunkManager(util::Context *ctx, GameState *state);
  void Update(glm::vec2 position);
  void Render(const wgpu::RenderPassEncoder &passEncoder);
  void RenderWater(const wgpu::RenderPassEncoder &passEncoder);

  std::optional<Chunk *> GetChunk(glm::ivec2 offset);
  std::vector<Chunk *> GetChunkNeighbors(glm::ivec2 offset);
  std::optional<std::tuple<Chunk *, glm::ivec3>> GetChunkAndPos(glm::ivec3 position);
  bool ShouldRender(glm::ivec3 position);
  bool WaterShouldRender(glm::ivec3 position);
  bool HasBlock(glm::ivec3 position);
  void SetBlock(glm::ivec3 position, BlockId blockId);
};

} // namespace game
