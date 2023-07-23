#include "block.hpp"
#include "game/direction.hpp"
#include "util/context.hpp"
#include "util/texture.hpp"

namespace game {

using namespace wgpu;

const std::array<BlockType, 4> g_BLOCK_TYPES = {
  // Air
  BlockType{},
  // Dirt
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::vec2 {
      return {2, 0};
    },
  },
  // Grass
  BlockType{
    .GetTextureLoc = [](Direction dir) -> glm::vec2 {
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
    .GetTextureLoc = [](Direction dir) -> glm::vec2 {
      return {3, 0};
    },
  },
};

BindGroup CreateBlocksTexture(util::Context &ctx) {
  // g_blocksTexture = util::LoadTexture(ctx, ROOT_DIR "/res/blocks.png");
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
  std::vector<BindGroupEntry> entries{
    BindGroupEntry{
      .binding = 0,
      .textureView = blocksTextureView,
    },
    BindGroupEntry{
      .binding = 1,
      .sampler = blocksSampler,
    },
  };
  BindGroupDescriptor bindGroupDesc{
    .layout = ctx.pipeline.bgl_texture,
    .entryCount = entries.size(),
    .entries = entries.data(),
  };
  return ctx.device.CreateBindGroup(&bindGroupDesc);
}

} // namespace game
