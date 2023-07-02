#pragma once

#include "glm/vec2.hpp"
#include <webgpu/webgpu_glfw.h>

#include <array>
#include "direction.hpp"
#include "util/handle.hpp"

namespace game {

enum BlockId {
  AIR = 0,
  DIRT = 1,
  GRASS = 2,
};

struct BlockType {
  BlockId id;
  glm::vec2 (*GetTextureLoc)(Direction dir);
};

const extern std::array<BlockType, 3> g_BLOCK_TYPES;
extern wgpu::Texture g_blocksTexture;
extern wgpu::TextureView g_blocksTextureView;
extern wgpu::Sampler g_blocksSampler;

wgpu::BindGroup InitTextures(util::Handle &handle);

}; // namespace game
