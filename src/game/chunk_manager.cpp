#include "chunk_manager.hpp"
#include "glm/common.hpp"
#include "util/handle.hpp"
#include <unordered_map>

namespace game {

using namespace wgpu;

ChunkManager::ChunkManager(util::Handle *handle, BindGroupLayout &layout)
    : m_handle(handle), m_offsetLayout(layout) {
  const glm::ivec2 centerPos = glm::floor(glm::vec2(0, 0) / glm::vec2(Chunk::SIZE)),
                   minOffset = centerPos - glm::ivec2(RADIUS, RADIUS),
                   maxOffset = centerPos + glm::ivec2(RADIUS, RADIUS);

  for (int x = minOffset.x; x <= maxOffset.x; x++) {
    for (int y = minOffset.y; y <= maxOffset.y; y++) {
      const auto offset = glm::ivec2(x, y);
      chunks.emplace(offset, new Chunk(m_handle, this, offset, layout));
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
        chunks.emplace(offset, new Chunk(m_handle, this, offset, m_offsetLayout));
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

} // namespace game
