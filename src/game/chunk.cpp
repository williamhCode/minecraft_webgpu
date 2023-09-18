#include "chunk.hpp"
#include "chunk_manager.hpp"
#include "dawn/utils/Timer.h"
#include "game/block.hpp"
#include "game/direction.hpp"
#include "game/mesh.hpp"
#include "glm/gtx/string_cast.hpp"
#include "util/webgpu-util.hpp"
#include "game.hpp"
#include <iostream>
#include <numeric>
#include <vector>

namespace game {

using namespace wgpu;

Chunk::Chunk(
  util::Context *ctx, GameState *state, ChunkManager *chunkManager, glm::ivec2 offset
)
    : dirty(true), chunkOffset(offset), m_ctx(ctx), m_state(state),
      m_chunkManager(chunkManager) {
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
  m_opaqueData.Clear();
  // m_translucentData.Clear();
  m_waterData.Clear();

  for (size_t i_block = 0; i_block < VOLUME; i_block++) {
    BlockId id = m_blockIdData[i_block];
    if (id == BlockId::Air) continue;
    BlockType blockType = g_BLOCK_TYPES[(size_t)id];
    auto posOffset = IndexToPos(i_block);

    Cube &cube = m_cubeData[i_block];
    for (size_t i_face = 0; i_face < g_MESH_FACES.size(); i_face++) {
      if (!ShouldRender(id, posOffset, (Direction)i_face)) continue;

      Face face = cube.faces[i_face];
      for (auto &vertex : face.vertices) {
        glm::ivec2 texLoc = blockType.GetTextureLoc((Direction)i_face);
        // convert texLoc to 1 byte (2x 4 bits), store in end of extraData
        vertex.extraData = texLoc.x | (texLoc.y << 4);
        // store transparency of face (2 bits)
        vertex.extraData |= (blockType.transparency << 8);
        vertex.position += m_worldOffset;
      }

      FaceIndex faceIndex;
      for (size_t i = 0; i < g_FACE_INDICES.size(); i++) {
        faceIndex.indices[i] = GetMeshData(id).faceNum * 4 + g_FACE_INDICES[i];
      }
      GetMeshData(id).AddFace(face, faceIndex);
    }
  }

  m_opaqueData.CreateBuffers(m_ctx->device);
  // m_translucentData.CreateBuffers(m_ctx->device);
  m_waterData.CreateBuffers(m_ctx->device);
}

void Chunk::Render(const wgpu::RenderPassEncoder &passEncoder) {
  passEncoder.SetVertexBuffer(0, m_opaqueData.vbo, 0, m_opaqueData.vbo.GetSize());
  passEncoder.SetIndexBuffer(
    m_opaqueData.ebo, IndexFormat::Uint32, 0, m_opaqueData.ebo.GetSize()
  );
  passEncoder.DrawIndexed(m_opaqueData.indices.size() * 6);
}

void Chunk::RenderTranslucent(const wgpu::RenderPassEncoder &passEncoder) {
  // record time
  // auto timer = dawn::utils::CreateTimer();
  // timer->Start();

  // sort indices based on distance from current position;
  auto position = glm::vec3(m_state->player.GetPosition());
  std::vector<FaceIndex> sortedIndices = m_translucentData.indices;
  std::sort(sortedIndices.begin(), sortedIndices.end(), [&](FaceIndex a, FaceIndex b) {
    auto aPos = m_translucentData.faces[a.indices[0]].vertices[0].position;
    auto bPos = m_translucentData.faces[b.indices[0]].vertices[0].position;
    return glm::distance(aPos, position) > glm::distance(bPos, position);
  });
  wgpu::Buffer ebo = util::CreateIndexBuffer(
    m_ctx->device, sortedIndices.size() * sizeof(FaceIndex), sortedIndices.data()
  );

  // std::cout << timer->GetElapsedTime() << "\n";
  // delete timer;

  // render
  passEncoder.SetVertexBuffer(
    0, m_translucentData.vbo, 0, m_translucentData.vbo.GetSize()
  );
  // passEncoder.SetIndexBuffer(
  //   m_translucentData.ebo, IndexFormat::Uint32, 0, m_translucentData.ebo.GetSize()
  // );
  // passEncoder.DrawIndexed(m_translucentData.indices.size() * 6);

  passEncoder.SetIndexBuffer(ebo, IndexFormat::Uint32, 0, ebo.GetSize());
  passEncoder.DrawIndexed(sortedIndices.size() * 6);
}

void Chunk::RenderWater(const wgpu::RenderPassEncoder &passEncoder) {
  passEncoder.SetVertexBuffer(0, m_waterData.vbo, 0, m_waterData.vbo.GetSize());
  passEncoder.SetIndexBuffer(
    m_waterData.ebo, IndexFormat::Uint32, 0, m_waterData.ebo.GetSize()
  );
  passEncoder.DrawIndexed(m_waterData.indices.size() * 6);
}

size_t Chunk::PosToIndex(glm::ivec3 pos) {
  return pos.x + pos.y * SIZE.x + pos.z * SIZE.x * SIZE.y;
}

glm::ivec3 Chunk::IndexToPos(size_t index) {
  return glm::ivec3(
    index % SIZE.x, (index / SIZE.x) % SIZE.y, index / (SIZE.x * SIZE.y)
  );
}

bool Chunk::ShouldRender(BlockId id, glm::ivec3 position, Direction direction) {
  glm::ivec3 neighborPos = position + g_DIR_OFFSETS[direction];

  if (neighborPos.z < 0) {
    return false;
  } else if (neighborPos.z >= SIZE.z) {
    return true;
  }
  else if (neighborPos.x < 0 || neighborPos.x >= SIZE.x || 
           neighborPos.y < 0 || neighborPos.y >= SIZE.y) {
    return m_chunkManager->ShouldRender(id, neighborPos + m_worldOffset);
  } else {
    return ShouldRender(id, neighborPos);
  }
}

bool Chunk::ShouldRender(BlockId id, glm::ivec3 position) {
  auto neighborId = GetBlock(position);
  switch (id) {
  // blocks that don't render when it's facing its own type (blocks 'link' together)
  case BlockId::Water:
  case BlockId::Glass:
    return !g_BLOCK_TYPES[(size_t)neighborId].opaque && neighborId != id;
  default:
    return !g_BLOCK_TYPES[(size_t)neighborId].opaque;
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

}; // namespace game
