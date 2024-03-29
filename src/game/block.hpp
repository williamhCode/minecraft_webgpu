#pragma once

#include "glm/vec2.hpp"
#include <webgpu/webgpu_glfw.h>
#include <stdint.h>

#include <array>
#include "direction.hpp"
#include "gfx/context.hpp"

namespace game {

enum class BlockId : uint8_t {
  Air = 0,
  Water,
  // opaque
  Dirt,
  Grass,
  Stone,
  Sand,
  Wood,
  // transparent
  Leaf,
  Glass,
  // special
  Light,

  Last,
};

// enum class RenderType : uint8_t {
//   None = 0,
//   Solid,
//   Leaves,
//   Transparent,
//   Water,
// };

struct BlockType {
  glm::uvec2 (*GetTextureLoc)(Direction dir);
  bool opaque = true;
  // 0 = default,
  // 1 = translucent,
  // 2 = transparent,
  // 3 = fully transparent (for ssao),
  unsigned char transparency = 0;
};

const extern std::array<BlockType, (size_t)BlockId::Last> g_BLOCK_TYPES;

wgpu::BindGroup CreateBlocksTexture(gfx::Context &ctx);

}; // namespace game
