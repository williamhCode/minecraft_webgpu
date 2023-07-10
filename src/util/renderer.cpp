#include "renderer.hpp"
#include "game/block.hpp"
#include <iostream>
#include <ostream>
#include <random>
#include <vector>
#include "glm/gtx/compatibility.hpp"

namespace util {

using namespace wgpu;

std::vector<QuadVertex> GetQuadVertices() {
  // {{-1.0, -1.0}, {0.0, 1.0}},
  // {{ 1.0, -1.0}, {1.0, 1.0}},
  // {{ 1.0,  1.0}, {1.0, 0.0}},
  // {{-1.0,  1.0}, {0.0, 0.0}},

  return {
    {{-1.0, -1.0}, {0.0, 1.0}},
    {{1.0, -1.0}, {1.0, 1.0}},
    {{-1.0, 1.0}, {0.0, 0.0}},

    {{1.0, -1.0}, {1.0, 1.0}},
    {{1.0, 1.0}, {1.0, 0.0}},
    {{-1.0, 1.0}, {0.0, 0.0}},
  };
}

Renderer::Renderer(Context *ctx, glm::uvec2 FBSize) : m_ctx(ctx) {
  m_blocksTextureBindGroup = game::CreateBlocksTexture(*m_ctx);

  // init quad buffer
  {
    auto quadVertices = GetQuadVertices();
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
      .size = quadVertices.size() * sizeof(QuadVertex),
    };
    m_quadBuffer = m_ctx->device.CreateBuffer(&bufferDesc);
    m_ctx->queue.WriteBuffer(m_quadBuffer, 0, quadVertices.data(), bufferDesc.size);
  }

  Extent3D textureSize = {FBSize.x, FBSize.y, 1};
  // gbuffer pass -----------------------------------------------------
  // create textures: position, normal, color
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = textureSize,
      .format = TextureFormat::RGBA16Float,
    };
    Texture positionTexture = m_ctx->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[0] = positionTexture.CreateView();
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = textureSize,
      .format = TextureFormat::RGBA16Float,
    };
    Texture normalTexture = m_ctx->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[1] = normalTexture.CreateView();
  }
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = textureSize,
      .format = TextureFormat::BGRA8Unorm,
    };
    Texture colorTexture = m_ctx->device.CreateTexture(&textureDesc);
    m_gBufferTextureViews[2] = colorTexture.CreateView();
  }

  // create depth texture
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment,
      .size = textureSize,
      .format = TextureFormat::Depth24Plus,
    };
    Texture depthTexture = m_ctx->device.CreateTexture(&textureDesc);
    m_depthTextureView = depthTexture.CreateView();
  }

  // render pass
  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      RenderPassColorAttachment{
        .view = m_gBufferTextureViews[0],
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 0.0},
      },
      RenderPassColorAttachment{
        .view = m_gBufferTextureViews[1],
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 0.0},
      },
      RenderPassColorAttachment{
        .view = m_gBufferTextureViews[2],
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.5, 0.8, 0.9, 1.0},
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

  // ssao pass ---------------------------------------------------
  // kernel uniform
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
  std::default_random_engine generator;
  std::vector<glm::vec4> ssaoKernel;
  size_t kSize = 45;
  for (size_t i = 0; i < kSize; ++i) {
    glm::vec3 sample(
      randomFloats(generator) * 2.0 - 1.0,
      randomFloats(generator) * 2.0 - 1.0,
      randomFloats(generator)
    );
    sample = glm::normalize(sample);
    sample *= randomFloats(generator);
    float scale = float(i) / kSize;

    scale = glm::lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;
    ssaoKernel.push_back({sample, 0.0});
  }
  auto kernelSize = sizeof(glm::vec4) * ssaoKernel.size();

  Buffer kernelBuffer;
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
      .size = kernelSize,
    };
    kernelBuffer = m_ctx->device.CreateBuffer(&bufferDesc);
    m_ctx->queue.WriteBuffer(kernelBuffer, 0, ssaoKernel.data(), kernelSize);
  }

  // noise texture
  std::vector<glm::vec4> ssaoNoise;
  for (unsigned int i = 0; i < 16; i++) {
    auto noise = glm::normalize(glm::vec3(
      randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0
    ));
    ssaoNoise.push_back({noise, 0.0});
  }

  TextureView noiseTextureView;
  {
    Extent3D size{4, 4, 1};
    TextureDescriptor textureDesc{
      .usage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
      .size = size,
      .format = TextureFormat::RGBA16Float,
    };
    Texture noiseTexture = m_ctx->device.CreateTexture(&textureDesc);

    ImageCopyTexture destination{
      .texture = noiseTexture,
    };
    TextureDataLayout source{
      .bytesPerRow = static_cast<uint32_t>(sizeof(glm::vec4)) * size.width,
      .rowsPerImage = size.height,
    };
    m_ctx->queue.WriteTexture(
      &destination,
      ssaoNoise.data(),
      sizeof(glm::vec4) * ssaoNoise.size(),
      &source,
      &size
    );

    noiseTextureView = noiseTexture.CreateView();
  }

  // samplers
  Sampler gBufferSampler;
  {
    SamplerDescriptor samplerDesc{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    };
    gBufferSampler = m_ctx->device.CreateSampler(&samplerDesc);
  }
  Sampler noiseSampler;
  {
    SamplerDescriptor samplerDesc{
      .addressModeU = AddressMode::Repeat,
      .addressModeV = AddressMode::Repeat,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    };
    noiseSampler = m_ctx->device.CreateSampler(&samplerDesc);
  }

  // gbuffer bind group
  {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .textureView = m_gBufferTextureViews[0],
      },
      BindGroupEntry{
        .binding = 1,
        .textureView = m_gBufferTextureViews[1],
      },
      BindGroupEntry{
        .binding = 2,
        .textureView = m_gBufferTextureViews[2],
      },
      BindGroupEntry{
        .binding = 3,
        .sampler = gBufferSampler,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.bgl_gBuffer,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_gBufferBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  // ssao bind group
  {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .buffer = kernelBuffer,
        .size = kernelSize,
      },
      BindGroupEntry{
        .binding = 1,
        .textureView = noiseTextureView,
      },
      BindGroupEntry{
        .binding = 2,
        .sampler = noiseSampler,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.bgl_ssao,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_ssaoBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  // ssao render texture
  TextureView ssaoTextureView;
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = textureSize,
      .format = TextureFormat::R8Unorm,
    };
    Texture ssaoTexture = m_ctx->device.CreateTexture(&textureDesc);
    ssaoTextureView = ssaoTexture.CreateView();
  }

  // render pass
  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      RenderPassColorAttachment{
        .view = ssaoTextureView,
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 1.0},
      },
    };
    m_ssaoPassDesc = {
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
    };
  }
}

void Renderer::Render(GameState &state) {
  TextureView nextTexture = m_ctx->swapChain.GetCurrentTextureView();
  if (!nextTexture) {
    std::cerr << "Cannot acquire next swap chain texture" << std::endl;
    std::exit(1);
  }

  RenderPassColorAttachment colorAttachment{
    .view = nextTexture,
    .loadOp = LoadOp::Clear,
    .storeOp = StoreOp::Store,
    .clearValue = {0.0, 0.0, 0.0, 1.0},
  };
  m_ssaoPassDesc.colorAttachments = &colorAttachment;

  CommandEncoder commandEncoder = m_ctx->device.CreateCommandEncoder();
  // gbuffer pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_gBufferPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.rpl_gBuffer);
    passEncoder.SetBindGroup(0, state.player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_blocksTextureBindGroup);
    state.chunkManager->Render(passEncoder);
    passEncoder.End();
  }
  // ssao pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_ssaoPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.rpl_ssao);
    passEncoder.SetBindGroup(0, state.player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_gBufferBindGroup);
    passEncoder.SetBindGroup(2, m_ssaoBindGroup);
    passEncoder.SetVertexBuffer(0, m_quadBuffer);
    passEncoder.Draw(6);
    passEncoder.End();
  }

  CommandBuffer command = commandEncoder.Finish();
  m_ctx->queue.Submit(1, &command);
}

void Renderer::Present() {
  m_ctx->swapChain.Present();
}

} // namespace util
