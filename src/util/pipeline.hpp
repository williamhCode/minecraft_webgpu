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
  wgpu::BindGroupLayout bgl_ssaoSampling;

  wgpu::BindGroupLayout bgl_ssaoTexture;

  wgpu::RenderPipeline rpl_gBuffer;
  wgpu::RenderPipeline rpl_ssao;
  wgpu::RenderPipeline rpl_blur;
  wgpu::RenderPipeline rpl_final;

  Pipeline() = default;
  Pipeline(Context &ctx);
};


} // namespace util
