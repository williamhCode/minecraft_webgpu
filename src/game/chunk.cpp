#include "chunk.hpp"
#include "game/mesh.hpp"
#include <iostream>

namespace game {

using namespace wgpu;

Chunk::Chunk(util::Handle *handle) : m_handle(handle), m_dirty(false) {
  InitializeChunkData();
  CreateBuffers();
  GenerateMesh();
}

void Chunk::InitializeChunkData() {
  for (size_t i = 0; i < VOLUME; i++) {
    m_blockData[i] = BlockType::DIRT;
  }
}

void Chunk::CreateBuffers() {
  uint64_t max_faces = VOLUME * 6;
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
      .size = max_faces * sizeof(Face),
    };
    m_vertexBuffer = m_handle->device.CreateBuffer(&bufferDesc);
  }
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Index,
      .size = max_faces * sizeof(FaceIndex),
    };
    m_indexBuffer = m_handle->device.CreateBuffer(&bufferDesc);
  }
}

void Chunk::GenerateMesh() {
  // brute force impl
  size_t numFace = 0;
  for (size_t i = 0; i < VOLUME; i++) {
    BlockType type = m_blockData[i];
    if (type == BlockType::AIR)
      continue;
    glm::vec3 posOffset = IndexToPos(i);
    for (size_t j = 0; j < g_MESH_FACES.size(); j++) {
      Face face = g_MESH_FACES[j];
      for (size_t k = 0; k < face.vertices.size(); k++) {
        face.vertices[k].position += posOffset;
      }
      m_faces.push_back(face);

      FaceIndex faceIndex;
      for (size_t i = 0; i < g_FACE_INDICES.size(); i++) {
        faceIndex.indices[i] = numFace * 4 + g_FACE_INDICES[i];
      }
      m_indices.push_back(faceIndex);
      numFace++;
    }
  }
  // upload data
  m_handle->queue.WriteBuffer(
    m_vertexBuffer, 0, m_faces.data(), m_faces.size() * sizeof(Face)
  );
  m_handle->queue.WriteBuffer(
    m_indexBuffer, 0, m_indices.data(), m_indices.size() * sizeof(FaceIndex)
  );
}

size_t Chunk::PosToIndex(glm::ivec3 pos) {
  return pos.x + pos.y * SIZE.x + pos.z * SIZE.x * SIZE.y;
}

glm::ivec3 Chunk::IndexToPos(size_t index) {
  return glm::ivec3(
    index % SIZE.x, (index / SIZE.x) % SIZE.y, index / (SIZE.x * SIZE.y)
  );
}

void Chunk::SetBlock(glm::vec3, BlockType type) {}

Buffer Chunk::GetVertexBuffer() { return m_vertexBuffer; }

Buffer Chunk::GetIndexBuffer() { return m_indexBuffer; }

void Chunk::Render(const wgpu::RenderPassEncoder &passEncoder) {
  // std::cout << m_faces.size() << "\n";
  // std::cout << m_indices.size() << "\n";
  passEncoder.SetVertexBuffer(0, m_vertexBuffer, 0, m_faces.size() * sizeof(Face));
  passEncoder.SetIndexBuffer(m_indexBuffer, IndexFormat::Uint16, 0, m_indices.size() * sizeof(FaceIndex));
  passEncoder.DrawIndexed(m_indices.size() * 6);
}

}; // namespace game
