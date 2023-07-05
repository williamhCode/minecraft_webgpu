#pragma once

#include "game.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"
#include "context.hpp"
#include <array>

namespace util {

class Renderer {
private:
  Context *m_ctx;

  wgpu::BindGroup m_bg_blocksTexture;
  wgpu::TextureView m_depthTextureView;
  // GBuffer
  std::array<wgpu::TextureView, 3> m_gBufferTextureViews;
  wgpu::Sampler m_sampler;

  wgpu::RenderPassDescriptor m_gBufferPassDesc;

public:
  Renderer(Context *ctx, glm::uvec2 size);
  void Render(GameState &state);
  void Present();
};

} // namespace util
