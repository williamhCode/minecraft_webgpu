#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <bit>
#include <vector>

namespace util {

using namespace wgpu;

Texture LoadTexture(Handle &handle, std::filesystem::path path) {
  int width, height, channels;
  unsigned char *pixelData =
    stbi_load(path.string().c_str(), &width, &height, &channels, 4);

  Extent3D size{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
  TextureDescriptor textureDesc{
    .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
    .dimension = TextureDimension::e2D,
    .size = size,
    .format = TextureFormat::RGBA8Unorm,
  };
  Texture texture = handle.device.CreateTexture(&textureDesc);

  ImageCopyTexture destination{
    .texture = texture,
  };
  TextureDataLayout source{
    .bytesPerRow = 4 * size.width,
    .rowsPerImage = size.height,
  };
  handle.queue.WriteTexture(
    &destination, pixelData, width * height * 4, &source, &size
  );

  stbi_image_free(pixelData);

  return texture;
}

Texture LoadTextureMipmap(Handle &handle, std::filesystem::path path) {
  int width, height, channels;
  uint8_t *pixelData = stbi_load(path.string().c_str(), &width, &height, &channels, 4);

  Extent3D size{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
  TextureDescriptor textureDesc{
    .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
    .dimension = TextureDimension::e2D,
    .size = size,
    .format = TextureFormat::RGBA8Unorm,
    .mipLevelCount = std::bit_width(std::max(size.width, size.height)),
  };
  Texture texture = handle.device.CreateTexture(&textureDesc);

  // Create image data
  Extent3D currSize = size;
  Extent3D resizedSize;
  uint8_t *currData = pixelData;
  uint8_t *resizedData = nullptr;
  for (uint32_t level = 0; level < textureDesc.mipLevelCount; ++level) {
    if (level != 0) {
      resizedSize = {
        .width = std::max(1u, currSize.width / 2),
        .height = std::max(1u, currSize.height / 2),
      };
      stbir_resize_uint8(
        currData, currSize.width, currSize.height, 0,
        resizedData, resizedSize.width, resizedSize.height, 0, 4
      );

      currSize = resizedSize;
      stbi_image_free(currData);
      currData = resizedData;
    }

    ImageCopyTexture destination{
      .texture = texture,
      .mipLevel = level,
    };
    TextureDataLayout source{
      .bytesPerRow = 4 * currSize.width,
      .rowsPerImage = currSize.height,
    };
    handle.queue.WriteTexture(
      &destination, currData, currSize.width * currSize.height * 4, &source, &currSize
    );
  }

  stbi_image_free(currData);

  return texture;
}

} // namespace util
