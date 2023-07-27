#include "chunk.hpp"
#include "chunk_manager.hpp"
#include "game/block.hpp"
#include "game/direction.hpp"
#include "game/mesh.hpp"
#include "glm/gtx/string_cast.hpp"
#include <iostream>
#include <vector>

namespace game {

using namespace wgpu;

Chunk::Chunk(util::Context *ctx, ChunkManager *chunkManager, glm::ivec2 offset)
    : dirty(false), m_ctx(ctx), m_chunkManager(chunkManager) {
  m_offsetPos = glm::ivec3(offset * glm::ivec2(SIZE.x, SIZE.y), 0);

  // create bind group
  const glm::vec3 posOffset(glm::vec2(offset) * glm::vec2(SIZE), 0);
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
    .size = sizeof(posOffset),
  };
  m_offsetBuffer = ctx->device.CreateBuffer(&bufferDesc);
  ctx->queue.WriteBuffer(m_offsetBuffer, 0, &posOffset, sizeof(posOffset));

  BindGroupEntry entry{
    .binding = 0,
    .buffer = m_offsetBuffer,
  };
  BindGroupDescriptor bindGroupDesc{
    .layout = ctx->pipeline.bgl_offset,
    .entryCount = 1,
    .entries = &entry,
  };
  this->m_bindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
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

void Chunk::UpdateFaceRenderData() {
  m_faceRenderData = {};
  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    BlockId id = m_blockIdData[i_block];
    if (id == BlockId::Air)
      continue;

    auto &faceRender = m_faceRenderData[i_block];
    auto posOffset = IndexToPos(i_block);
    for (size_t i_face = 0; i_face < 6; i_face++) {
      faceRender[i_face] = ShouldRender(posOffset, (Direction)i_face);
    }
  }
}

bool Chunk::ShouldRender(glm::ivec3 position, Direction direction) {
  glm::ivec3 neighborPos = position + g_DIR_OFFSETS[direction];

  if (neighborPos.z < 0) {
    return false;
  } else if (neighborPos.z >= SIZE.z) {
    return true;
  }
  else if (neighborPos.x < 0 || neighborPos.x >= SIZE.x || 
           neighborPos.y < 0 || neighborPos.y >= SIZE.y) {
    return m_chunkManager->ShouldRender(neighborPos + m_offsetPos);
  } else {
    auto index = PosToIndex(neighborPos);
    return m_blockIdData[index] == BlockId::Air;
  }
}

void Chunk::CreateBuffers() {
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
      .size = m_faces.size() * sizeof(Face),
    };
    m_vertexBuffer = m_ctx->device.CreateBuffer(&bufferDesc);
  }
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Index,
      .size = m_indices.size() * sizeof(FaceIndex),
    };
    m_indexBuffer = m_ctx->device.CreateBuffer(&bufferDesc);
  }
}

void Chunk::UpdateBuffers() {
  m_ctx->queue.WriteBuffer(
    m_vertexBuffer, 0, m_faces.data(), m_faces.size() * sizeof(Face)
  );
  m_ctx->queue.WriteBuffer(
    m_indexBuffer, 0, m_indices.data(), m_indices.size() * sizeof(FaceIndex)
  );
}

void Chunk::UpdateMesh() {
  m_faces.clear();
  m_indices.clear();

  size_t numFace = 0;
  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    BlockId id = m_blockIdData[i_block];
    if (id == BlockId::Air)
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
  UpdateBuffers();
}

void Chunk::Render(const wgpu::RenderPassEncoder &passEncoder) {
  passEncoder.SetBindGroup(2, m_bindGroup);
  passEncoder.SetVertexBuffer(0, m_vertexBuffer, 0, m_faces.size() * sizeof(Face));
  passEncoder.SetIndexBuffer(
    m_indexBuffer, IndexFormat::Uint32, 0, m_indices.size() * sizeof(FaceIndex)
  );
  passEncoder.DrawIndexed(m_indices.size() * 6);
}

size_t Chunk::PosToIndex(glm::ivec3 pos) {
  return pos.x + pos.y * SIZE.x + pos.z * SIZE.x * SIZE.y;
}

glm::ivec3 Chunk::IndexToPos(size_t index) {
  return glm::ivec3(
    index % SIZE.x, (index / SIZE.x) % SIZE.y, index / (SIZE.x * SIZE.y)
  );
}

bool Chunk::HasNeighbor(glm::ivec3 position, Direction direction) {
  glm::ivec3 neighborPos = position + g_DIR_OFFSETS[direction];

  if (neighborPos.z < 0 || neighborPos.z >= SIZE.z) {
    return false;
  }
  else if (neighborPos.x < 0 || neighborPos.x >= SIZE.x || 
           neighborPos.y < 0 || neighborPos.y >= SIZE.y) {
    return m_chunkManager->HasBlock(neighborPos + m_offsetPos);
  } else {
    auto index = PosToIndex(neighborPos);
    return m_blockIdData[index] != BlockId::Air;
  }
}

BlockId Chunk::GetBlock(glm::ivec3 position) {
  return m_blockIdData[PosToIndex(position)];
}

void Chunk::SetBlock(glm::ivec3 position, BlockId blockID) {
  if (position.z < 0 || position.z >= SIZE.z) {
    return;
  }
  auto index = PosToIndex(position);
  m_blockIdData[index] = blockID;
  if (blockID != BlockId::Air) {
    auto &faceRender = m_faceRenderData[index];
    for (size_t i_face = 0; i_face < 6; i_face++) {
      faceRender[i_face] = !HasNeighbor(position, (Direction)i_face);
    }
  }
  // update block faces for neighbors
  bool shouldRender = blockID == BlockId::Air;
  for (size_t i_face = 0; i_face < 6; i_face++) {
    glm::ivec3 neighborPos = position + g_DIR_OFFSETS[i_face];
    Direction neighborDir = DirOpposite((Direction)i_face);
    if (neighborPos.z < 0 || neighborPos.z >= SIZE.z) {
      continue;
    }
    else if (neighborPos.x < 0 || neighborPos.x >= SIZE.x || 
             neighborPos.y < 0 || neighborPos.y >= SIZE.y) {
      m_chunkManager->UpdateFace(neighborPos + m_offsetPos, neighborDir, shouldRender);
    } else {
      UpdateFace(neighborPos, neighborDir, shouldRender);
    }
  }
  UpdateMesh();
}

bool Chunk::HasBlock(glm::ivec3 position) {
  return GetBlock(position) != BlockId::Air;
}

void Chunk::UpdateFace(glm::ivec3 position, Direction direction, bool shouldRender) {
  if (!HasBlock(position))
    return;
  m_faceRenderData[PosToIndex(position)][direction] = shouldRender;
}

}; // namespace game
