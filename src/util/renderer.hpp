#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"
#include "handle.hpp"

namespace util {

class Renderer {
private:
  Handle *m_handle;
  wgpu::Texture m_depthTexture;
  wgpu::TextureView m_depthTextureView;
  wgpu::CommandEncoder m_commandEncoder;
  wgpu::RenderPassEncoder m_passEncoder;

public:
  Renderer(Handle *handle, glm::uvec2 size);
  wgpu::RenderPassEncoder Begin(wgpu::Color color);
  void End();
  void Present();
};

} // namespace util
