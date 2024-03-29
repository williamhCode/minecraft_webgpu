#include "chunk_manager.hpp"
#include "gen.hpp"
#include "glm/common.hpp"
#include "gfx/context.hpp"
#include <iostream>
#include <ostream>
#include <unordered_map>
#include "game.hpp"

namespace game {

using namespace wgpu;

ChunkManager::ChunkManager(gfx::Context *ctx, GameState *state)
    : m_ctx(ctx), m_state(state) {
  const glm::ivec2 centerPos = glm::floor(glm::vec2(0, 0) / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(radius, radius),
                   maxOffset = centerPos + glm::ivec2(radius, radius);

  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      if (glm::distance(glm::vec2(x, y), glm::vec2(centerPos)) > radius - 0.1) continue;
      const auto offset = glm::ivec2(x, y);
      auto chunk = new Chunk(m_ctx, m_state, this, offset);
      GenChunkData(*chunk);
      chunks.emplace(offset, chunk);
    }
  }

  // for (int x = minOffset.x; x <= maxOffset.x; x++) {
  //   for (int y = minOffset.y; y <= maxOffset.y; y++) {
  //     const auto offset = glm::ivec2(x, y);
  //     auto chunk = new Chunk(m_ctx, m_state, this, offset);
  //     GenChunkData(*chunk);
  //     chunks.emplace(offset, chunk);
  //   }
  // }
}

void ChunkManager::Update(glm::vec2 position) {
  int gens = 0;
  const glm::ivec2 centerPos = glm::floor(position / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(radius, radius),
                   maxOffset = centerPos + glm::ivec2(radius, radius);

  if (glm::length(position - m_prevPos) > gfx::Sun::updateDist) {
    m_prevPos = position;
    update = true;
  }

  if (!update) goto exit;

  // remove chunks not in radius
  std::erase_if(chunks, [&](auto &pair) {
    const auto &[offset, chunk] = pair;
    if (glm::distance(glm::vec2(offset), glm::vec2(centerPos)) > radius - 0.1) {
      return true;
    }
    return false;
  });

  // add chunks in radius
  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      if (gens >= max_gens) goto exit;
      if (glm::distance(glm::vec2(x, y), glm::vec2(centerPos)) > radius - 0.1) continue;
      const auto offset = glm::ivec2(x, y);
      if (!chunks.contains(offset)) {
        auto chunk = new Chunk(m_ctx, m_state, this, offset);
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

  if (update) m_state->sun.InvokeUpdate();

  if (gens == 0) update = false;

  // store offsets of chunks inside frustum/camera's view
  m_frustumOffsets.clear();
  auto frustum = m_state->player.camera.GetFrustum();
  for (auto &[offset, chunk] : chunks) {
    auto boundingBox = chunk->GetBoundingBox();
    if (frustum.Intersects(boundingBox)) {
      m_frustumOffsets.push_back(offset);
    }
  }
  // sort front to back for performance
  glm::vec2 pos = glm::vec2(m_state->player.GetPosition()) / glm::vec2(Chunk::SIZE);
  std::sort(
    m_frustumOffsets.begin(), m_frustumOffsets.end(),
    // implicit conversion from ivec2 to vec2
    [&](glm::vec2 a, glm::vec2 b) {
      a += glm::vec2(0.5);
      b += glm::vec2(0.5);
      return glm::distance(a, pos) < glm::distance(b, pos);
    }
  );

  // sort offsets back to front based on distance to camera for transparent objects
  /* m_sortedFrustumOffsets = m_frustumOffsets;
  glm::vec2 pos = glm::vec2(m_state->player.GetPosition()) / glm::vec2(Chunk::SIZE);
  std::sort(
    m_sortedFrustumOffsets.begin(), m_sortedFrustumOffsets.end(),
    // implicit conversion from ivec2 to vec2
    [&](glm::vec2 a, glm::vec2 b) {
      a += glm::vec2(0.5);
      b += glm::vec2(0.5);
      return glm::distance(a, pos) > glm::distance(b, pos);
    }
  ); */

  // update dirty chunks
  for (auto &[offset, chunk] : chunks) {
    if (chunk->dirty) {
      chunk->UpdateMesh();
      chunk->dirty = false;
    }
  }
}

void ChunkManager::RenderShadowMap(
  const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex, int cascadeLevel
) {
  std::vector<glm::ivec2> shadowOffsets;
  auto frustum = m_state->sun.GetFrustum(cascadeLevel);
  for (auto &[offset, chunk] : chunks) {
    auto boundingBox = chunk->GetBoundingBox();
    if (frustum.Intersects(boundingBox)) {
      shadowOffsets.push_back(offset);
    }
  }

  for (auto offset : shadowOffsets) {
    chunks[offset]->Render(passEncoder, groupIndex);
  }
}

void ChunkManager::Render(
  const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex
) {
  // opaque objects
  for (auto offset : m_frustumOffsets) {
    chunks[offset]->Render(passEncoder, groupIndex);
  }

  // translucent objects
  // for (auto offset : m_sortedFrustumOffsets) {
  //   chunks[offset]->RenderTranslucent(passEncoder, groupIndex);
  // }
}

void ChunkManager::RenderWater(
  const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex
) {
  for (auto offset : m_frustumOffsets) {
    chunks[offset]->RenderWater(passEncoder, groupIndex);
  }
}

void ChunkManager::RenderWire(
  const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex
) {
  // opaque objects
  for (auto offset : m_frustumOffsets) {
    chunks[offset]->RenderWire(passEncoder, groupIndex);
  }

  // translucent objects
  // for (auto offset : m_sortedFrustumOffsets) {
  //   chunks[offset]->RenderTranslucent(passEncoder, groupIndex);
  // }
}

void ChunkManager::RenderWaterWire(
  const wgpu::RenderPassEncoder &passEncoder, uint32_t groupIndex
) {
  for (auto offset : m_frustumOffsets) {
    chunks[offset]->RenderWaterWire(passEncoder, groupIndex);
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
  static glm::ivec2 neighborOffsets[] = {
    {0, 1},
    {0, -1},
    {1, 0},
    {-1, 0},
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

std::optional<std::tuple<Chunk *, glm::ivec3>> ChunkManager::GetChunkAndPos(
  glm::ivec3 position
) {
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
bool ChunkManager::ShouldRender(BlockId id, glm::ivec3 position) {
  auto chunk = GetChunkAndPos(position);
  if (!chunk) {
    return false;
  }
  auto &[chunkPtr, localPos] = *chunk;
  return chunkPtr->ShouldRender(id, localPos);
}

bool ChunkManager::HasBlock(glm::ivec3 position) {
  auto chunk = GetChunkAndPos(position);
  if (!chunk) {
    return false;
  }
  auto &[chunkPtr, localPos] = *chunk;
  return chunkPtr->HasBlock(localPos);
}

void ChunkManager::SetBlockAndUpdate(glm::ivec3 position, BlockId blockId) {
  auto chunk = GetChunkAndPos(position);
  if (!chunk) {
    return;
  }
  auto &[chunkPtr, localPos] = *chunk;
  chunkPtr->SetBlockAndUpdate(localPos, blockId);
}

} // namespace game
