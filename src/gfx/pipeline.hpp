#pragma once

#include <webgpu/webgpu_cpp.h>
#include <vector>

namespace util {
  struct Context;
}

namespace gfx {

struct Pipeline {
  wgpu::BindGroupLayout cameraBGL;
  wgpu::BindGroupLayout textureBGL;
  wgpu::BindGroupLayout chunkBGL;
  wgpu::BindGroupLayout lightingBGL;

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
  Pipeline(util::Context &ctx);
};

} // namespace util
