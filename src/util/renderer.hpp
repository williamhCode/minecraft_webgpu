#pragma once

#include "dawn/utils/WGPUHelpers.h"
#include "glm/ext/vector_uint2.hpp"
#include "glm/ext/vector_float2.hpp"
#include "webgpu/webgpu_cpp.h"
#include "context.hpp"
#include <array>

// forward declaration
struct GameState;

namespace util {

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
    sampleSize = 20;
    radius = 5;
    bias = 0.01;
  }
};
#define WRITE_SSAO_BUFFER(field)                                                       \
  m_ctx->queue.WriteBuffer(                                                            \
    m_ssaoBuffer, offsetof(SSAO, field), &m_ssao.field, sizeof(SSAO::field)            \
  )

class Renderer {
private:
  Context *m_ctx;
  GameState *m_state;

  wgpu::BindGroup m_blocksTextureBindGroup;
  wgpu::TextureView m_depthTextureView;
  wgpu::Buffer m_quadBuffer;

  // gbuffer
  // position (view-space), normal, color
  std::array<wgpu::TextureView, 3> m_gBufferTextureViews;
  dawn::utils::ComboRenderPassDescriptor m_gBufferPassDesc;

  // water
  dawn::utils::ComboRenderPassDescriptor m_waterPassDesc;

  // ssao
  SSAO m_ssao;
  wgpu::Buffer m_ssaoBuffer;

  wgpu::BindGroup m_gBufferBindGroup;
  wgpu::BindGroup m_ssaoSamplingBindGroup;
  wgpu::RenderPassDescriptor m_ssaoPassDesc;

  // blur
  // pre-blur and blurred (used in composite)
  std::array<wgpu::BindGroup, 2> m_ssaoTextureBindGroups;
  wgpu::RenderPassDescriptor m_blurPassDesc;

  // composite
  wgpu::BindGroup m_waterTextureBindGroup;
  wgpu::RenderPassDescriptor m_compositePassDesc;

public:
  Renderer(Context *ctx, GameState *state);
  void ImguiRender();
  void Render();
  void Present();
};

} // namespace util
