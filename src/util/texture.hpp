#pragma once

#include <filesystem>
#include <webgpu/webgpu_cpp.h>
#include "context.hpp"

namespace util {

wgpu::Texture LoadTexture(Context &ctx, std::filesystem::path path);
wgpu::Texture LoadTextureMipmap(Context &ctx, std::filesystem::path path);

} // namespace util
