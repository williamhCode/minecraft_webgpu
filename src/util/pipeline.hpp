#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>

namespace util {

struct Context;

struct Pipeline {
  wgpu::BindGroupLayout bgl_viewProj;
  wgpu::BindGroupLayout bgl_texture;
  wgpu::BindGroupLayout bgl_offset;

  wgpu::BindGroupLayout bgl_gBuffer;
  wgpu::BindGroupLayout bgl_ssao;

  wgpu::RenderPipeline rpl_gBuffer;
  wgpu::RenderPipeline rpl_ssao;

  Pipeline() = default;
  Pipeline(Context &ctx);
};


} // namespace util
