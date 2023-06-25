#include "chunk.hpp"
#include "game/block.hpp"
#include "game/direction.hpp"
#include "game/mesh.hpp"
#include "glm/gtx/string_cast.hpp"
#include <iostream>
#include <vector>

namespace game {

using namespace wgpu;

Chunk::Chunk(util::Handle *handle) : m_handle(handle), m_dirty(false) {
  InitializeChunkData();
  CreateBuffers();
  GenerateMesh();
}

void Chunk::InitializeChunkData() {
  for (size_t i = 0; i < VOLUME; i++) {
    m_blockData[i] = BlockId::GRASS;
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
  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    BlockId id = m_blockData[i_block];
    if (id == BlockId::AIR) continue;
    BlockType blockType = g_BLOCK_TYPES[id];
    glm::vec3 posOffset = IndexToPos(i_block);

    for (size_t i_face = 0; i_face < g_MESH_FACES.size(); i_face++) {
      // don't generate face if there's block in that direction
      glm::vec3 neighborPos = posOffset + POS_OFFSETS[i_face];
      if (!(
        neighborPos.x < 0 || neighborPos.x >= SIZE.x ||
        neighborPos.y < 0 || neighborPos.y >= SIZE.y ||
        neighborPos.z < 0 || neighborPos.z >= SIZE.z
      )) {
        auto index = PosToIndex(neighborPos);
        if (m_blockData[index] != BlockId::AIR) {
          continue;
        }
      }

      Face face = g_MESH_FACES[i_face];
      glm::vec2 texLoc = blockType.GetTextureLoc((Direction)i_face);
      for (size_t i_vertex = 0; i_vertex < face.vertices.size(); i_vertex++) {
        face.vertices[i_vertex].position += posOffset;
        face.vertices[i_vertex].texLoc = texLoc;
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

void Chunk::SetBlock(glm::vec3, BlockId blockID) {}

Buffer Chunk::GetVertexBuffer() { return m_vertexBuffer; }

Buffer Chunk::GetIndexBuffer() { return m_indexBuffer; }

void Chunk::Render(const wgpu::RenderPassEncoder &passEncoder) {
  passEncoder.SetVertexBuffer(0, m_vertexBuffer, 0, m_faces.size() * sizeof(Face));
  passEncoder.SetIndexBuffer(
    m_indexBuffer, IndexFormat::Uint32, 0, m_indices.size() * sizeof(FaceIndex)
  );
  passEncoder.DrawIndexed(m_indices.size() * 6);
}

}; // namespace game
