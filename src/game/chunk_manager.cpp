#include "chunk_manager.hpp"
#include "glm/common.hpp"
#include "util/context.hpp"
#include <unordered_map>

namespace game {

using namespace wgpu;

ChunkManager::ChunkManager(util::Context *ctx)
    : m_ctx(ctx) {
  const glm::ivec2 centerPos = glm::floor(glm::vec2(0, 0) / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(RADIUS, RADIUS),
                   maxOffset = centerPos + glm::ivec2(RADIUS, RADIUS);

  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      const auto offset = glm::ivec2(x, y);
      chunks.emplace(offset, new Chunk(m_ctx, this, offset));
    }
  }

  for (auto &[_, chunk] : chunks) {
    chunk->InitFaceData();
    chunk->UpdateMesh();
  }
}

void ChunkManager::Update(glm::vec2 position) {
  const glm::ivec2 centerPos = glm::floor(position / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(RADIUS, RADIUS),
                   maxOffset = centerPos + glm::ivec2(RADIUS, RADIUS);

  // remove chunks not in radius
  std::erase_if(chunks, [&](auto &pair) {
    const auto &[offset, chunk] = pair;
    return offset.x < minOffset.x || offset.x > maxOffset.x || offset.y < minOffset.y ||
           offset.y > maxOffset.y;
  });

  int gens = 0;
  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      const auto offset = glm::ivec2(x, y);
      if (!chunks.contains(offset) && gens < MAX_GENS) {
        chunks.emplace(offset, new Chunk(m_ctx, this, offset));
        gens++;
      }
    }
  }
}

void ChunkManager::Render(wgpu::RenderPassEncoder &passEncoder) {
  for (auto &[offset, chunk] : chunks) {
    chunk->Render(passEncoder);
  }
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

bool ChunkManager::HasBlock(glm::ivec3 position) {
  auto chunk = GetChunk(position);
  if (!chunk.has_value()) {
    return false;
  }
  const auto &[chunkPtr, localPos] = chunk.value();
  return chunkPtr->HasBlock(localPos);
}

void ChunkManager::SetBlock(glm::ivec3 position, BlockId blockId) {
  auto chunk = GetChunk(position);
  if (!chunk.has_value()) {
    return;
  }
  const auto &[chunkPtr, localPos] = chunk.value();
  chunkPtr->SetBlock(localPos, blockId);
}

void ChunkManager::UpdateFace(
  glm::ivec3 position, Direction direction, bool shouldRender
) {
  auto chunk = GetChunk(position);
  if (!chunk.has_value()) {
    return;
  }
  const auto &[chunkPtr, localPos] = chunk.value();
  if (!chunkPtr->HasBlock(localPos)) {
    return;
  }
  chunkPtr->UpdateFace(localPos, direction, shouldRender);
  chunkPtr->UpdateMesh();
}

} // namespace game
