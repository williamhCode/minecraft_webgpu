#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>

namespace gfx {

struct Context;

struct Pipeline {
  wgpu::BindGroupLayout cameraBGL;
  wgpu::BindGroupLayout textureBGL;
  wgpu::BindGroupLayout chunkBGL;
  wgpu::BindGroupLayout sunBGL;

  wgpu::BindGroupLayout gBufferBGL;
  wgpu::BindGroupLayout ssaoSamplingBGL;

  wgpu::BindGroupLayout ssaoTextureBGL;

  wgpu::BindGroupLayout compositeBGL;

  wgpu::RenderPipeline shadowRPL;
  wgpu::RenderPipeline gBufferRPL;
  wgpu::RenderPipeline waterRPL;
  wgpu::RenderPipeline ssaoRPL;
  wgpu::RenderPipeline blurRPL;
  wgpu::RenderPipeline compositeRPL;

  Pipeline() = default;
  Pipeline(gfx::Context &ctx);
};

} // namespace util
