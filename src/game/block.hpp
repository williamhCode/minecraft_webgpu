#pragma once

#include "glm/vec2.hpp"
#include <webgpu/webgpu_glfw.h>
#include <stdint.h>

#include <array>
#include "direction.hpp"
#include "util/context.hpp"

namespace game {

enum class BlockId : uint8_t {
  Air = 0,
  // opaque
  Dirt,
  Grass,
  Stone,
  Sand,
  // transparent
  Leaf,
  Glass,

  Water,
};

// enum class RenderType : uint8_t {
//   None = 0,
//   Solid,
//   Leaves,
//   Transparent,
//   Water,
// };

struct BlockType {
  glm::ivec2 (*GetTextureLoc)(Direction dir);
  bool transparent = false;
};

const extern std::array<BlockType, 8> g_BLOCK_TYPES;
// const extern std::array<

wgpu::BindGroup CreateBlocksTexture(util::Context &ctx);

}; // namespace game
