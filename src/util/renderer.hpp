#pragma once

#include "game.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"
#include "context.hpp"
#include <array>

namespace util {

struct QuadVertex {
  glm::vec2 position;
  glm::vec2 uv;
};

class Renderer {
private:
  Context *m_ctx;

  wgpu::BindGroup m_blocksTextureBindGroup;
  wgpu::TextureView m_depthTextureView;
  wgpu::Buffer m_quadBuffer;

  // gbuffer
  std::array<wgpu::TextureView, 3> m_gBufferTextureViews;
  wgpu::RenderPassDescriptor m_gBufferPassDesc;

  // ssao
  wgpu::Sampler m_sampler;
  wgpu::BindGroup m_gBufferBindGroup;
  wgpu::BindGroup m_ssaoBindGroup;
  wgpu::RenderPassDescriptor m_ssaoPassDesc;

public:
  Renderer(Context *ctx, glm::uvec2 size);
  void Render(GameState &state);
  void Present();
};

} // namespace util
