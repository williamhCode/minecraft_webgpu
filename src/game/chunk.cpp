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
  InitFaceData();
  UpdateMesh();
}

std::array<Cube, Chunk::VOLUME> Chunk::m_cubeData;
void Chunk::InitSharedData() {
  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    Cube &cube = m_cubeData[i_block];
    glm::vec3 posOffset = IndexToPos(i_block);

    for (size_t i_face = 0; i_face < g_MESH_FACES.size(); i_face++) {
      Face face = g_MESH_FACES[i_face];
      for (size_t i_vertex = 0; i_vertex < face.vertices.size(); i_vertex++) {
        face.vertices[i_vertex].position += posOffset;
      }
      cube.faces[i_face] = face;
    }
  }
}

void Chunk::InitializeChunkData() {
  for (size_t i = 0; i < VOLUME; i++) {
    auto pos = IndexToPos(i);
    if (pos.z > 100) {
      m_blockIdData[i] = BlockId::AIR;
    } else if (pos.z == 100) {
      m_blockIdData[i] = BlockId::GRASS;
    } else {
      m_blockIdData[i] = BlockId::DIRT;
    }
  }
}

void Chunk::CreateBuffers() {
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
      .size = m_faces.size() * sizeof(Face),
    };
    m_vertexBuffer = m_handle->device.CreateBuffer(&bufferDesc);
  }
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Index,
      .size = m_indices.size() * sizeof(FaceIndex),
    };
    m_indexBuffer = m_handle->device.CreateBuffer(&bufferDesc);
  }
}

void Chunk::InitFaceData() {
  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    BlockId id = m_blockIdData[i_block];
    if (id == BlockId::AIR)
      continue;

    auto &faceRender = m_faceRenderData[i_block];
    glm::vec3 posOffset = IndexToPos(i_block);
    for (size_t i_face = 0; i_face < 6; i_face++) {
      glm::vec3 neighborPos = posOffset + POS_OFFSETS[i_face];
      // clang-format off
      if (neighborPos.x < 0 || neighborPos.x >= SIZE.x || 
          neighborPos.y < 0 || neighborPos.y >= SIZE.y || 
          neighborPos.z < 0 || neighborPos.z >= SIZE.z) { // clang-format on
        faceRender[i_face] = true;
      } else {
        auto index = PosToIndex(neighborPos);
        if (m_blockIdData[index] == BlockId::AIR) {
          faceRender[i_face] = true;
        }
      }
    }
  }
}

void Chunk::UpdateMesh() {
  m_faces.clear();
  m_indices.clear();

  size_t numFace = 0;
  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    BlockId id = m_blockIdData[i_block];
    if (id == BlockId::AIR)
      continue;
    BlockType blockType = g_BLOCK_TYPES[id];

    auto &faceRender = m_faceRenderData[i_block];
    Cube &cube = m_cubeData[i_block];
    for (size_t i_face = 0; i_face < g_MESH_FACES.size(); i_face++) {
      if (faceRender[i_face] == false)
        continue;

      Face face = cube.faces[i_face];
      for (size_t i_vertex = 0; i_vertex < face.vertices.size(); i_vertex++) {
        face.vertices[i_vertex].texLoc = blockType.GetTextureLoc((Direction)i_face);
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

  CreateBuffers();

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

void Chunk::SetBlock(glm::vec3 position, BlockId blockID) {}

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
