#include "chunk_manager.hpp"
#include "gen.hpp"
#include "glm/common.hpp"
#include "util/context.hpp"
#include <iostream>
#include <ostream>
#include <unordered_map>
#include "game.hpp"

namespace game {

using namespace wgpu;

ChunkManager::ChunkManager(util::Context *ctx, GameState *state)
    : m_ctx(ctx), m_state(state) {
  const glm::ivec2 centerPos = glm::floor(glm::vec2(0, 0) / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(radius, radius),
                   maxOffset = centerPos + glm::ivec2(radius, radius);

  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      if (glm::distance(glm::vec2(x, y), glm::vec2(centerPos)) > radius - 0.1) continue;
      const auto offset = glm::ivec2(x, y);
      auto chunk = new Chunk(m_ctx, this, offset);
      GenChunkData(*chunk);
      chunks.emplace(offset, chunk);
    }
  }
}

void ChunkManager::Update(glm::vec2 position) {
  const glm::ivec2 centerPos = glm::floor(position / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(radius, radius),
                   maxOffset = centerPos + glm::ivec2(radius, radius);

  // remove chunks not in radius
  std::erase_if(chunks, [&](auto &pair) {
    const auto &[offset, chunk] = pair;
    if (glm::distance(glm::vec2(offset), glm::vec2(centerPos)) > radius - 0.1) {
      return true;
    }
    return false;
  });

  // add chunks in radius
  int gens = 0;
  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      if (gens >= max_gens) goto exit;
      if (glm::distance(glm::vec2(x, y), glm::vec2(centerPos)) > radius - 0.1) continue;
      const auto offset = glm::ivec2(x, y);
      if (!chunks.contains(offset)) {
        auto chunk = new Chunk(m_ctx, this, offset);
        GenChunkData(*chunk);
        chunks.emplace(offset, chunk);
        for (auto neighbor : GetChunkNeighbors(offset)) {
          neighbor->dirty = true;
        }
        gens++;
      }
    }
  }
exit:

  // update dirty chunks
  for (auto &[offset, chunk] : chunks) {
    if (chunk->dirty) {
      chunk->UpdateMesh();
      chunk->dirty = false;
    }
  }
}

void ChunkManager::Render(const wgpu::RenderPassEncoder &passEncoder) {
  auto frustum = m_state->player.camera.GetFrustum();

  for (auto &[offset, chunk] : chunks) {
    auto boundingBox = chunk->GetBoundingBox();
    if (frustum.Intersects(boundingBox)) chunk->Render(passEncoder);
  }
}

void ChunkManager::RenderWater(const wgpu::RenderPassEncoder &passEncoder) {
  auto frustum = m_state->player.camera.GetFrustum();

  for (auto &[offset, chunk] : chunks) {
    auto boundingBox = chunk->GetBoundingBox();
    if (frustum.Intersects(boundingBox)) chunk->RenderWater(passEncoder);
  }
}

std::optional<Chunk *> ChunkManager::GetChunk(glm::ivec2 offset) {
  const auto it = chunks.find(offset);
  if (it == chunks.end()) {
    return std::nullopt;
  }
  return it->second.get();
}

std::vector<Chunk *> ChunkManager::GetChunkNeighbors(glm::ivec2 offset) {
  std::vector<Chunk *> neighbors;
  // neighbor list
  static std::array<glm::ivec2, 4> neighborOffsets = {
    glm::ivec2(0, 1),
    glm::ivec2(0, -1),
    glm::ivec2(1, 0),
    glm::ivec2(-1, 0),
  };
  for (auto neighborOffset : neighborOffsets) {
    neighborOffset += offset;
    auto chunk = GetChunk(neighborOffset);
    if (chunk) {
      neighbors.push_back(*chunk);
    }
  }

  return neighbors;
}

std::optional<std::tuple<Chunk *, glm::ivec3>>
ChunkManager::GetChunkAndPos(glm::ivec3 position) {
  if (position.z < 0 || position.z >= Chunk::SIZE.z) {
    return std::nullopt;
  }
  const glm::ivec2 offset = glm::floor(glm::vec2(position) / glm::vec2(Chunk::SIZE));
  const glm::ivec3 localPos = glm::mod(glm::vec3(position), glm::vec3(Chunk::SIZE));
  const auto it = chunks.find(offset);
  if (it == chunks.end()) {
    return std::nullopt;
  }
  return std::make_tuple(it->second.get(), localPos);
}

// in context when called from a chunk
bool ChunkManager::ShouldRender(glm::ivec3 position) {
  auto chunk = GetChunkAndPos(position);
  if (!chunk) {
    return false;
  }
  auto &[chunkPtr, localPos] = *chunk;
  return !chunkPtr->HasBlock(localPos);
}

bool ChunkManager::WaterShouldRender(glm::ivec3 position) {
  auto chunk = GetChunkAndPos(position);
  if (!chunk) {
    return false;
  }
  auto &[chunkPtr, localPos] = *chunk;
  return !chunkPtr->WaterHasBlock(localPos);
}

bool ChunkManager::HasBlock(glm::ivec3 position) {
  auto chunk = GetChunkAndPos(position);
  if (!chunk) {
    return false;
  }
  auto &[chunkPtr, localPos] = *chunk;
  return chunkPtr->HasBlock(localPos);
}

void ChunkManager::SetBlock(glm::ivec3 position, BlockId blockId) {
  auto chunk = GetChunkAndPos(position);
  if (!chunk) {
    return;
  }
  auto &[chunkPtr, localPos] = *chunk;
  chunkPtr->SetBlock(localPos, blockId);
}

} // namespace game
