#pragma once

#include <filesystem>
#include <webgpu/webgpu_cpp.h>
#include "gfx/context.hpp"

namespace util {

wgpu::Texture LoadTexture(wgpu::Device &device, std::filesystem::path path);
wgpu::Texture LoadTextureMipmap(gfx::Context &ctx, std::filesystem::path path);

} // namespace util
