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
    static auto depthTexture = m_handle->device.CreateTexture(&textureDesc);
    m_depthTextureView = depthTexture.CreateView();
  }

  // SSAO
  // Create Textures: position, normal, color
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::RGBA16Float,
    };
    static auto positionTexture = m_handle->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[0] = positionTexture.CreateView();
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::RGBA16Float,
    };
    static auto normalTexture = m_handle->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[1] = normalTexture.CreateView();
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::BGRA8Unorm,
    };
    static auto colorTexture = m_handle->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[2] = colorTexture.CreateView();
  }

  // render pass
  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      RenderPassColorAttachment{
        .view = m_gBufferTextureViews[0],
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
      },
      RenderPassColorAttachment{
        .view = m_gBufferTextureViews[1],
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
      },
      RenderPassColorAttachment{
        .view = m_gBufferTextureViews[2],
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
      },
    };
    static RenderPassDepthStencilAttachment depthStencilAttachment{
      .view = m_depthTextureView,
      .depthLoadOp = LoadOp::Clear,
      .depthStoreOp = StoreOp::Store,
      .depthClearValue = 1.0f,
    };
    m_gBufferPassDesc = {
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
      .depthStencilAttachment = &depthStencilAttachment,
    };
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
