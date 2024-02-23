#include "block.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "game/direction.hpp"
#include "gfx/context.hpp"
#include "util/texture.hpp"

namespace game {

using namespace wgpu;

const std::array<BlockType, size_t(BlockId::Last)> g_BLOCK_TYPES = {
  // Air
  BlockType{.opaque = false},
  // Water
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      return {0, 15};
    },
    .opaque = false,
  },
  // Dirt
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      return {2, 0};
    },
  },
  // Grass
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      switch (dir) {
      case Direction::TOP:
        return {0, 0};
      case Direction::BOTTOM:
        return {2, 0};
      default:
        return {1, 0};
      }
    },
  },
  // Stone
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      return {3, 0};
    },
  },
  // Sand
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      return {0, 1};
    },
  },
  // Wood
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      switch (dir) {
      case Direction::TOP:
      case Direction::BOTTOM:
        return {3, 1};
      default:
        return {2, 1};
      }
    },
  },
  // Leaf
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      return {4, 1};
    },
    .opaque = false,
    .transparency = 2,
  },
  // Glass
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      return {1, 1};
    },
    .opaque = false,
    .transparency = 3,
  },
  // light
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::uvec2 {
      return {1, 4};
    },
  },
};

BindGroup CreateBlocksTexture(gfx::Context &ctx) {
  Texture blocksTexture = util::LoadTextureMipmap(ctx, ROOT_DIR "/res/blocks.png");
  TextureViewDescriptor viewDesc{
    // 16 x 16 textures, so len([16, 8, 4, 2, 1]) = 5
    // anything after 5th mipmap blurs the different face textures
    .mipLevelCount = 5,
  };
  TextureView blocksTextureView = blocksTexture.CreateView(&viewDesc);

  SamplerDescriptor samplerDesc{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
    .mipmapFilter = MipmapFilterMode::Linear,
  };
  Sampler blocksSampler = ctx.device.CreateSampler(&samplerDesc);

  // create bind group
  return dawn::utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureBGL,
    {
      {0, blocksTextureView},
      {1, blocksSampler},
    }
  );
}

} // namespace game
