#include "chunk.hpp"
#include "chunk_manager.hpp"
#include "game/block.hpp"
#include "game/direction.hpp"
#include "game/mesh.hpp"
#include "glm/gtx/string_cast.hpp"
#include "util/webgpu-util.hpp"
#include <iostream>
#include <vector>

namespace game {

using namespace wgpu;

Chunk::Chunk(util::Context *ctx, ChunkManager *chunkManager, glm::ivec2 offset)
    : dirty(true), chunkOffset(offset), m_ctx(ctx), m_chunkManager(chunkManager) {
  m_worldOffset = glm::ivec3(offset * glm::ivec2(SIZE.x, SIZE.y), 0);
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

void Chunk::UpdateMesh() {
  m_faces.clear();
  m_indices.clear();

  m_waterFaces.clear();
  m_waterIndices.clear();

  size_t numFace = 0;
  size_t numWaterFace = 0;
  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    BlockId id = m_blockIdData[i_block];
    if (id == BlockId::Air) continue;
    BlockType blockType = g_BLOCK_TYPES[id];
    auto posOffset = IndexToPos(i_block);

    Cube &cube = m_cubeData[i_block];
    for (size_t i_face = 0; i_face < g_MESH_FACES.size(); i_face++) {
      if (id == BlockId::Water) {
        if (!WaterShouldRender(posOffset, (Direction)i_face)) continue;
      } else {
        if (!ShouldRender(posOffset, (Direction)i_face)) continue;
      }

      Face face = cube.faces[i_face];
      for (auto &vertex : face.vertices) {
        glm::ivec2 texLoc = blockType.GetTextureLoc((Direction)i_face);
        // convert texLoc to 1 byte (2x 4 bits), store in end of extraData
        vertex.extraData = texLoc.x | (texLoc.y << 4);
        vertex.position += m_worldOffset;
      }

      FaceIndex faceIndex;
      for (size_t i = 0; i < g_FACE_INDICES.size(); i++) {
        if (id == BlockId::Water)
          faceIndex.indices[i] = numWaterFace * 4 + g_FACE_INDICES[i];
        else
          faceIndex.indices[i] = numFace * 4 + g_FACE_INDICES[i];
      }

      if (id == BlockId::Water) {
        m_waterFaces.push_back(face);
        m_waterIndices.push_back(faceIndex);
        numWaterFace++;
      } else {
        m_faces.push_back(face);
        m_indices.push_back(faceIndex);
        numFace++;
      }
    }
  }

  m_vertexBuffer =
    util::CreateVertexBuffer(m_ctx, m_faces.size() * sizeof(Face), m_faces.data());
  m_indexBuffer = util::CreateIndexBuffer(
    m_ctx, m_indices.size() * sizeof(FaceIndex), m_indices.data()
  );
  m_waterVbo = util::CreateVertexBuffer(
    m_ctx, m_waterFaces.size() * sizeof(Face), m_waterFaces.data()
  );
  m_waterEbo = util::CreateIndexBuffer(
    m_ctx, m_waterIndices.size() * sizeof(FaceIndex), m_waterIndices.data()
  );
}

void Chunk::Render(const wgpu::RenderPassEncoder &passEncoder) {
  passEncoder.SetVertexBuffer(0, m_vertexBuffer, 0, m_vertexBuffer.GetSize());
  passEncoder.SetIndexBuffer(
    m_indexBuffer, IndexFormat::Uint32, 0, m_indexBuffer.GetSize()
  );
  passEncoder.DrawIndexed(m_indices.size() * 6);
}

void Chunk::RenderWater(const wgpu::RenderPassEncoder &passEncoder) {
  passEncoder.SetVertexBuffer(0, m_waterVbo, 0, m_waterVbo.GetSize());
  passEncoder.SetIndexBuffer(m_waterEbo, IndexFormat::Uint32, 0, m_waterEbo.GetSize());
  passEncoder.DrawIndexed(m_waterIndices.size() * 6);
}

size_t Chunk::PosToIndex(glm::ivec3 pos) {
  return pos.x + pos.y * SIZE.x + pos.z * SIZE.x * SIZE.y;
}

glm::ivec3 Chunk::IndexToPos(size_t index) {
  return glm::ivec3(
    index % SIZE.x, (index / SIZE.x) % SIZE.y, index / (SIZE.x * SIZE.y)
  );
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
    return m_chunkManager->ShouldRender(neighborPos + m_worldOffset);
  } else {
    return !HasBlock(neighborPos);
  }
}

bool Chunk::WaterShouldRender(glm::ivec3 position, Direction direction) {
  glm::ivec3 neighborPos = position + g_DIR_OFFSETS[direction];

  if (neighborPos.z < 0) {
    return false;
  } else if (neighborPos.z >= SIZE.z) {
    return true;
  }
  else if (neighborPos.x < 0 || neighborPos.x >= SIZE.x || 
           neighborPos.y < 0 || neighborPos.y >= SIZE.y) {
    return m_chunkManager->WaterShouldRender(neighborPos + m_worldOffset);
  } else {
    return !WaterHasBlock(neighborPos);
  }
}

// bool Chunk::HasNeighbor(glm::ivec3 position, Direction direction) {
//   glm::ivec3 neighborPos = position + g_DIR_OFFSETS[direction];

//   if (neighborPos.z < 0 || neighborPos.z >= SIZE.z) {
//     return false;
//   }
//   else if (neighborPos.x < 0 || neighborPos.x >= SIZE.x ||
//            neighborPos.y < 0 || neighborPos.y >= SIZE.y) {
//     return m_chunkManager->HasBlock(neighborPos + m_offsetPos);
//   } else {
//     auto index = PosToIndex(neighborPos);
//     return m_blockIdData[index] != BlockId::Air;
//   }
// }

BlockId Chunk::GetBlock(glm::ivec3 position) {
  return m_blockIdData[PosToIndex(position)];
}

void Chunk::SetBlock(glm::ivec3 position, BlockId blockID) {
  if (position.z < 0 || position.z >= SIZE.z) {
    return;
  }
  auto index = PosToIndex(position);
  m_blockIdData[index] = blockID;

  static std::array<glm::ivec2, 4> neighborOffsets = {
    glm::ivec2(0, 1),
    glm::ivec2(0, -1),
    glm::ivec2(1, 0),
    glm::ivec2(-1, 0),
  };
  std::array<bool, 4> neighborClose = {
    position.y == SIZE.y - 1,
    position.y == 0,
    position.x == SIZE.x - 1,
    position.x == 0,
  };

  for (size_t i = 0; i < 4; i++) {
    if (!neighborClose[i]) continue;
    auto neighborOffset = chunkOffset + neighborOffsets[i];
    auto chunk = m_chunkManager->GetChunk(neighborOffset);
    if (chunk) {
      (*chunk)->dirty = true;
    }
  }

  dirty = true;
}

bool Chunk::HasBlock(glm::ivec3 position) {
  auto blockId = GetBlock(position);
  return blockId != BlockId::Air && blockId != BlockId::Water;
}

bool Chunk::WaterHasBlock(glm::ivec3 position) {
  auto blockId = GetBlock(position);
  return blockId != BlockId::Air;
}

}; // namespace game
