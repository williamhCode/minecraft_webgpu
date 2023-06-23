#pragma once

#include <webgpu/webgpu_cpp.h>
#include "util/handle.hpp"

namespace util {

wgpu::RenderPipeline CreatePipelineSimple(util::Handle &handle);

}
