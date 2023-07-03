#include "renderer.hpp"
#include <iostream>
#include <ostream>
#include <vector>

namespace util {

using namespace wgpu;

Renderer::Renderer(Handle *handle, glm::uvec2 size) : m_handle(handle) {
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment,
      .size = {size.x, size.y},
      .format = TextureFormat::Depth24Plus,
    };
    m_depthTexture = m_handle->device.CreateTexture(&textureDesc);
    m_depthTextureView = m_depthTexture.CreateView();
  }

  // SSAO
  // Create Textures: position, normal, color
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::RGBA16Float,
    };
    m_positionTexture = m_handle->device.CreateTexture(&textureDesc);
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::RGBA16Float,
    };
    m_normalTexture = m_handle->device.CreateTexture(&textureDesc);
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::BGRA8Unorm,
    };
    m_colorTexture = m_handle->device.CreateTexture(&textureDesc);
  }
  // Create sampler
  {
    SamplerDescriptor samplerDesc{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    };
    m_sampler = m_handle->device.CreateSampler(&samplerDesc);
  }

  // render pass
  {
    m_colorAttachments = {
      RenderPassColorAttachment{
        .view = m_positionTexture.CreateView(),
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
      },
      RenderPassColorAttachment{
        .view = m_normalTexture.CreateView(),
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
      },
      RenderPassColorAttachment{
        .view = m_colorTexture.CreateView(),
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
      },
    };
    m_depthStencilAttachment = {
      .view = m_depthTextureView,
      .depthLoadOp = LoadOp::Clear,
      .depthStoreOp = StoreOp::Store,
      .depthClearValue = 1.0f,
    };
    m_gBufferPassDesc = {
      .colorAttachmentCount = m_colorAttachments.size(),
      .colorAttachments = m_colorAttachments.data(),
      .depthStencilAttachment = &m_depthStencilAttachment,
    };
  }
}

RenderPassEncoder Renderer::Begin(Color color) {
  TextureView nextTexture = m_handle->swapChain.GetCurrentTextureView();
  if (!nextTexture) {
    std::cerr << "Cannot acquire next swap chain texture" << std::endl;
    std::exit(1);
  }

  CommandEncoderDescriptor commandEncoderDesc{};
  m_commandEncoder = m_handle->device.CreateCommandEncoder(&commandEncoderDesc);

  // RenderPassColorAttachment colorAttachment{
  //   .view = nextTexture,
  //   .loadOp = LoadOp::Clear,
  //   .storeOp = StoreOp::Store,
  //   .clearValue = color,
  // };
  // RenderPassDepthStencilAttachment depthStencilAttachment{
  //   .view = m_depthTextureView,
  //   .depthLoadOp = LoadOp::Clear,
  //   .depthStoreOp = StoreOp::Store,
  //   .depthClearValue = 1.0f,
  // };
  // RenderPassDescriptor renderPassDesc{
  //   .colorAttachmentCount = 1,
  //   .colorAttachments = &colorAttachment,
  //   .depthStencilAttachment = &depthStencilAttachment,
  // };
  m_passEncoder = m_commandEncoder.BeginRenderPass(&m_gBufferPassDesc);
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
