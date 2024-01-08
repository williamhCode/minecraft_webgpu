#pragma once

#include <webgpu/webgpu_cpp.h>

namespace gfx {

struct Context;

struct Pipeline {
  // shared bind-group-layouts
  wgpu::BindGroupLayout cameraBGL;
  wgpu::BindGroupLayout textureBGL;
  wgpu::BindGroupLayout chunkBGL;
  wgpu::BindGroupLayout sunBGL;

  wgpu::BindGroupLayout shadowBGL;

  wgpu::BindGroupLayout gBufferBGL;
  wgpu::BindGroupLayout ssaoSamplingBGL;

  wgpu::BindGroupLayout ssaoTextureBGL;

  wgpu::BindGroupLayout compositeBGL;

  // render pipelines
  wgpu::RenderPipeline shadowRPL;
  wgpu::RenderPipeline gBufferRPL;
  wgpu::RenderPipeline waterRPL;
  wgpu::RenderPipeline ssaoRPL;
  wgpu::RenderPipeline blurRPL;
  wgpu::RenderPipeline compositeRPL;

  Pipeline() = default;
  Pipeline(gfx::Context &ctx);
};

} // namespace gfx
