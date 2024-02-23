#pragma once

#include <webgpu/webgpu_cpp.h>
#include "dawn/utils/WGPUHelpers.h"
#include "glm/ext/vector_float2.hpp"

#include "gfx/context.hpp"
#include "util/webgpu-util.hpp"
#include "gfx/sun.hpp"

#include <array>

// forward declaration
struct GameState;

namespace gfx {

struct QuadVertex {
  glm::vec2 position;
  glm::vec2 uv;
};

struct SSAO {
  int enabled;
  int sampleSize;
  float radius;
  float bias;

  SSAO() {
    SetDefault();
  }
  void SetDefault() {
    enabled = true;
    sampleSize = 10;
    radius = 5.0;
    bias = 0.01;
  }
};
#define WRITE_SSAO_BUFFER(field)                                                       \
  m_ctx->queue.WriteBuffer(                                                            \
    m_ssaoBuffer, offsetof(SSAO, field), &m_ssao.field, sizeof(SSAO::field)            \
  )

class Renderer {
private:
  bool wireframe = false;

  gfx::Context *m_ctx;
  GameState *m_state;

  wgpu::BindGroup m_blocksTextureBindGroup;
  wgpu::Buffer m_quadBuffer;

  // shadow
  // util::RenderPassDescriptor m_shadowPassDesc;
  std::array<util::RenderPassDescriptor, Sun::numCascades> m_shadowPassDescs;
  std::array<wgpu::BindGroup, Sun::numCascades> m_cascadeIndicesBG;

  // gbuffer
  // position (view-space), normal, color
  dawn::utils::ComboRenderPassDescriptor m_gBufferPassDesc;
  dawn::utils::ComboRenderPassDescriptor m_gBufferWirePassDesc;
  dawn::utils::ComboRenderPassDescriptor m_gBufferDepthPassDesc;

  // water
  dawn::utils::ComboRenderPassDescriptor m_waterPassDesc;

  // ssao
  SSAO m_ssao;
  wgpu::Buffer m_ssaoBuffer;

  wgpu::BindGroup m_gBufferBindGroup;
  wgpu::BindGroup m_ssaoSamplingBindGroup;
  util::RenderPassDescriptor m_ssaoPassDesc;

  // blur
  // pre-blur and blurred (used in composite)
  std::array<wgpu::BindGroup, 2> m_ssaoTextureBindGroups;
  util::RenderPassDescriptor m_blurPassDesc;

  // composite
  wgpu::BindGroup m_compositeBindGroup;
  wgpu::RenderPassDescriptor m_compositePassDesc;

public:
  Renderer(gfx::Context *ctx, GameState *state);
  void ImguiRender();
  void Render();
  void Present();
};

} // namespace gfx
