#include "renderer.hpp"
#include "game/block.hpp"
#include <iostream>
#include <ostream>
#include <vector>

namespace util {

using namespace wgpu;

Renderer::Renderer(Context *ctx, glm::uvec2 size) : m_ctx(ctx) {
  m_bg_blocksTexture = game::CreateBlocksTexture(*m_ctx);

  // create depth texture
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment,
      .size = {size.x, size.y},
      .format = TextureFormat::Depth24Plus,
    };
    static Texture depthTexture = m_ctx->device.CreateTexture(&textureDesc);
    m_depthTextureView = depthTexture.CreateView();
  }

  // SSAO
  // create textures: position, normal, color
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::RGBA16Float,
    };
    static Texture positionTexture = m_ctx->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[0] = positionTexture.CreateView();
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::RGBA16Float,
    };
    static Texture normalTexture = m_ctx->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[1] = normalTexture.CreateView();
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = {size.x, size.y},
      .format = TextureFormat::BGRA8Unorm,
    };
    static Texture colorTexture = m_ctx->device.CreateTexture(&textureDesc);
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
    m_sampler = m_ctx->device.CreateSampler(&samplerDesc);
  }
}

void Renderer::Render(GameState &state) {
  TextureView nextTexture = m_ctx->swapChain.GetCurrentTextureView();
  if (!nextTexture) {
    std::cerr << "Cannot acquire next swap chain texture" << std::endl;
    std::exit(1);
  }

  CommandEncoder commandEncoder = m_ctx->device.CreateCommandEncoder();
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_gBufferPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.rpl_gBuffer);
    passEncoder.SetBindGroup(0, state.player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_bg_blocksTexture);
    state.chunkManager->Render(passEncoder);
    passEncoder.End();
  }

  CommandBufferDescriptor cmdBufferDescriptor{};
  CommandBuffer command = commandEncoder.Finish(&cmdBufferDescriptor);
  m_ctx->queue.Submit(1, &command);
}

void Renderer::Present() {
  m_ctx->swapChain.Present();
}

} // namespace util
