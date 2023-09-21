#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>

namespace util {

struct Context;

struct Pipeline {
  wgpu::BindGroupLayout cameraBGL;
  wgpu::BindGroupLayout textureBGL;
  wgpu::BindGroupLayout chunkBGL;

  wgpu::BindGroupLayout gBufferBGL;
  wgpu::BindGroupLayout ssaoSamplingBGL;

  wgpu::BindGroupLayout ssaoTextureBGL;

  wgpu::BindGroupLayout compositeBGL;
  wgpu::BindGroupLayout lightingBGL;

  wgpu::RenderPipeline gBufferRPL;
  wgpu::RenderPipeline waterRPL;
  wgpu::RenderPipeline ssaoRPL;
  wgpu::RenderPipeline blurRPL;
  wgpu::RenderPipeline compositeRPL;

  Pipeline() = default;
  Pipeline(Context &ctx);
};

} // namespace util
