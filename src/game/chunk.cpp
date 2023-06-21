#include "chunk.hpp"

namespace game {

Chunk::Chunk(util::Handle &handle, glm::vec3 size)
    : m_handle(handle), m_size(size), m_dirty(false) {
  InitializeChunkData();
  CreateBuffers();
}

void Chunk::InitializeChunkData() {}

void Chunk::CreateBuffers() {}

void Chunk::GenerateMesh() {}

void Chunk::SetBlock(glm::vec3, Block blockType) {}

void Chunk::Render(const wgpu::RenderPassEncoder &passEncoder) {}

}; // namespace game
