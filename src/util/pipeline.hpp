#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>

namespace util {

struct Handle;

struct Pipeline {
  wgpu::BindGroupLayout bgl_viewProj;
  wgpu::BindGroupLayout bgl_texture;
  wgpu::BindGroupLayout bgl_offset;

  wgpu::RenderPipeline rpl_gBuffer;

  Pipeline() = default;
  Pipeline(Handle &handle);
};


} // namespace util
