#pragma once

#include <webgpu/webgpu_cpp.h>
#include "util/handle.hpp"
#include <vector>

namespace util {

std::vector<wgpu::RenderPipeline> CreatePipelines(util::Handle &handle);

} // namespace util
