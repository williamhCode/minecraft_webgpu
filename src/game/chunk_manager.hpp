#pragma once

#include "game/chunk.hpp"
#include "glm/ext/vector_float3.hpp"
#include <glm/gtx/hash.hpp>
#include "gfx/context.hpp"
#include <unordered_map>
#include <vector>

// forward decl
struct GameState;

namespace game {

class ChunkManager {
private:
  gfx::Context *m_ctx;
  GameState *m_state;

  std::vector<glm::ivec2> m_frustumOffsets;
  std::vector<glm::ivec2> m_sortedFrustumOffsets;

  glm::vec2 m_prevPos;

public:
  bool update = true;

  int radius = 16;
  int max_gens = 4;
  std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> chunks;

  ChunkManager() = default;
  ChunkManager(gfx::Context *ctx, GameState *state);
  void Update(glm::vec2 position);
  void RenderShadowMap(const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex, int cascadeLevel);
  void Render(const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex);
  void RenderWater(const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex);

  std::optional<Chunk *> GetChunk(glm::ivec2 offset);
  std::vector<Chunk *> GetChunkNeighbors(glm::ivec2 offset);
  std::optional<std::tuple<Chunk *, glm::ivec3>> GetChunkAndPos(glm::ivec3 position);
  bool ShouldRender(BlockId id, glm::ivec3 position);
  bool HasBlock(glm::ivec3 position);
  void SetBlockAndUpdate(glm::ivec3 position, BlockId blockId);
};

} // namespace game
