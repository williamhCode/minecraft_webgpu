#pragma once

#include "glm/vec2.hpp"
#include <webgpu/webgpu_glfw.h>
#include <stdint.h>

#include <array>
#include "direction.hpp"
#include "util/context.hpp"

namespace game {

enum BlockId : uint8_t {
  Air = 0,
  Dirt = 1,
  Grass = 2,
  Stone = 3,
};

struct BlockType {
  glm::vec2 (*GetTextureLoc)(Direction dir);
};

const extern std::array<BlockType, 4> g_BLOCK_TYPES;

wgpu::BindGroup CreateBlocksTexture(util::Context &ctx);

}; // namespace game
