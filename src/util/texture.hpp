#pragma once

#include <filesystem>
#include <webgpu/webgpu_cpp.h>
#include "handle.hpp"

namespace util {

wgpu::Texture LoadTexture(Handle &handle, std::filesystem::path path);
wgpu::Texture LoadTextureMipmap(Handle &handle, std::filesystem::path path);

} // namespace util
