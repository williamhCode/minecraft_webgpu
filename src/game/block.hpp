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
  Dirt,
  Grass,
  Stone,
  Sand,
  Water,
};

struct BlockType {
  glm::ivec2 (*GetTextureLoc)(Direction dir);
};

const extern std::array<BlockType, 6> g_BLOCK_TYPES;

wgpu::BindGroup CreateBlocksTexture(util::Context &ctx);

}; // namespace game
