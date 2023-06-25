#include "block.hpp"
#include "game/direction.hpp"
#include "util/handle.hpp"
#include "util/texture.hpp"

namespace game {

using namespace wgpu;

const std::array<BlockType, 3> g_BLOCK_TYPES = {
  BlockType{
    .id = BlockId::AIR,
  },
  BlockType{
    .id = BlockId::DIRT,
    .GetTextureLoc = [](Direction dir) -> glm::vec2 {
      return {2, 0};
    },
  },
  BlockType{
    .id = BlockId::GRASS,
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
};

wgpu::Texture g_blocksTexture;
wgpu::TextureView g_blocksTextureView;
wgpu::Sampler g_blocksSampler;

void InitTextures(util::Handle &handle) {
  g_blocksTexture = util::LoadTexture(handle, ROOT_DIR "/res/blocks.png");
  TextureViewDescriptor viewDesc{
    .format = g_blocksTexture.GetFormat(),
    .dimension = TextureViewDimension::e2D,
    .mipLevelCount = g_blocksTexture.GetMipLevelCount(),
    .arrayLayerCount = g_blocksTexture.GetDepthOrArrayLayers(),
  };
  g_blocksTextureView = g_blocksTexture.CreateView(&viewDesc);

  SamplerDescriptor samplerDesc{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .addressModeW = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
    .mipmapFilter = MipmapFilterMode::Linear,
  };
  g_blocksSampler = handle.device.CreateSampler(&samplerDesc);
}

} // namespace game
