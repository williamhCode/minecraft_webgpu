#include "chunk_manager.hpp"
#include "gen.hpp"
#include "glm/common.hpp"
#include "util/context.hpp"
#include <unordered_map>

namespace game {

using namespace wgpu;

ChunkManager::ChunkManager(util::Context *ctx) : m_ctx(ctx) {
  const glm::ivec2 centerPos = glm::floor(glm::vec2(0, 0) / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(radius, radius),
                   maxOffset = centerPos + glm::ivec2(radius, radius);

  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      if (glm::distance(glm::vec2(x, y), glm::vec2(centerPos)) > radius - 0.1)
        continue;
      const auto offset = glm::ivec2(x, y);
      auto chunk = new Chunk(m_ctx, this, offset);
      GenChunkData(*chunk);
      chunks.emplace(offset, chunk);
    }
  }

  for (auto &[_, chunk] : chunks) {
    chunk->UpdateFaceRenderData();
    chunk->UpdateMesh();
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
      if (gens >= max_gens)
        goto exit;
      if (glm::distance(glm::vec2(x, y), glm::vec2(centerPos)) > radius - 0.1)
        continue;
      const auto offset = glm::ivec2(x, y);
      if (!chunks.contains(offset)) {
        auto chunk = new Chunk(m_ctx, this, offset);
        chunk->dirty = true;
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

  // update chunks
  for (auto &[offset, chunk] : chunks) {
    if (chunk->dirty) {
      chunk->UpdateFaceRenderData();
      chunk->UpdateMesh();
      chunk->dirty = false;
    }
  }
}

void ChunkManager::Render(wgpu::RenderPassEncoder &passEncoder) {
  for (auto &[offset, chunk] : chunks) {
    chunk->Render(passEncoder);
  }
}

// including itself
std::vector<Chunk *> ChunkManager::GetChunkNeighbors(glm::ivec2 offset) {
  std::vector<Chunk *> neighbors;
  // neighbor list
  std::array<glm::ivec2, 4> neighborOffsets = {
    glm::ivec2(0, 1), glm::ivec2(0, -1), glm::ivec2(1, 0), glm::ivec2(-1, 0)};
  for (auto &[x, y] : neighborOffsets) {
    const auto neighborOffset = offset + glm::ivec2(x, y);
    const auto it = chunks.find(neighborOffset);
    if (it != chunks.end()) {
      neighbors.push_back(it->second.get());
    }
  }

  return neighbors;
}

std::optional<std::tuple<Chunk *, glm::ivec3>>
ChunkManager::GetChunk(glm::ivec3 position) {
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

bool ChunkManager::ShouldRender(glm::ivec3 position) {
  auto chunk = GetChunk(position);
  if (!chunk) {
    return false;
  }
  auto &[chunkPtr, localPos] = *chunk;
  return !chunkPtr->HasBlock(localPos);
}

bool ChunkManager::HasBlock(glm::ivec3 position) {
  auto chunk = GetChunk(position);
  if (!chunk) {
    return false;
  }
  auto &[chunkPtr, localPos] = *chunk;
  return chunkPtr->HasBlock(localPos);
}

void ChunkManager::SetBlock(glm::ivec3 position, BlockId blockId) {
  auto chunk = GetChunk(position);
  if (!chunk) {
    return;
  }
  auto &[chunkPtr, localPos] = *chunk;
  chunkPtr->SetBlock(localPos, blockId);
}

void ChunkManager::UpdateFace(
  glm::ivec3 position, Direction direction, bool shouldRender
) {
  auto chunk = GetChunk(position);
  if (!chunk) {
    return;
  }
  auto &[chunkPtr, localPos] = *chunk;
  if (!chunkPtr->HasBlock(localPos)) {
    return;
  }
  chunkPtr->UpdateFace(localPos, direction, shouldRender);
  chunkPtr->UpdateMesh();
}

} // namespace game
