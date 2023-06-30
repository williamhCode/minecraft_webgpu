#include "renderer.hpp"
#include <iostream>
#include <ostream>

namespace util {

using namespace wgpu;

Renderer::Renderer(Handle *handle, glm::uvec2 size) : m_handle(handle) {
  TextureDescriptor depthTextureDesc{
    .usage = TextureUsage::RenderAttachment,
    .size = {size.x, size.y},
    .format = TextureFormat::Depth24Plus,
  };
  m_depthTexture = handle->device.CreateTexture(&depthTextureDesc);

  TextureViewDescriptor depthTextureViewDesc{};
  m_depthTextureView = m_depthTexture.CreateView(&depthTextureViewDesc);
}

RenderPassEncoder Renderer::Begin(Color color) {
  TextureView nextTexture = m_handle->swapChain.GetCurrentTextureView();
  if (!nextTexture) {
    std::cerr << "Cannot acquire next swap chain texture" << std::endl;
    std::exit(1);
  }

  CommandEncoderDescriptor commandEncoderDesc{};
  m_commandEncoder = m_handle->device.CreateCommandEncoder(&commandEncoderDesc);

  RenderPassColorAttachment colorAttachment{
    .view = nextTexture,
    .loadOp = LoadOp::Clear,
    .storeOp = StoreOp::Store,
    .clearValue = color,
  };
  RenderPassDepthStencilAttachment depthStencilAttachment{
    .view = m_depthTextureView,
    .depthLoadOp = LoadOp::Clear,
    .depthStoreOp = StoreOp::Store,
    .depthClearValue = 1.0f,
    .depthReadOnly = false,
  };
  RenderPassDescriptor renderPassDesc{
    .colorAttachmentCount = 1,
    .colorAttachments = &colorAttachment,
    .depthStencilAttachment = &depthStencilAttachment,
  };
  m_passEncoder = m_commandEncoder.BeginRenderPass(&renderPassDesc);
  return m_passEncoder;
}

void Renderer::End() {
  m_passEncoder.End();
  CommandBufferDescriptor cmdBufferDescriptor{};
  CommandBuffer command = m_commandEncoder.Finish(&cmdBufferDescriptor);
  m_handle->queue.Submit(1, &command);
}

void Renderer::Present() {
  m_handle->swapChain.Present();
}

} // namespace util
