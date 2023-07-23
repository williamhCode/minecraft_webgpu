#include "game.hpp"
#include "renderer.hpp"
#include "game/block.hpp"
#include <iostream>
#include <ostream>
#include <random>
#include <vector>
#include "glm/gtx/compatibility.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

namespace util {

using namespace wgpu;

std::vector<QuadVertex> GetQuadVertices() {
  return {
    {{-1.0, -1.0}, {0.0, 1.0}},
    {{1.0, -1.0}, {1.0, 1.0}},
    {{-1.0, 1.0}, {0.0, 0.0}},

    {{1.0, -1.0}, {1.0, 1.0}},
    {{1.0, 1.0}, {1.0, 0.0}},
    {{-1.0, 1.0}, {0.0, 0.0}},
  };
}

Renderer::Renderer(Context *ctx, GameState *state) : m_ctx(ctx), m_state(state) {
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

  Extent3D textureSize = {m_state->fbSize.x, m_state->fbSize.y, 1};
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
      .format = m_ctx->depthFormat,
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
        .clearValue = {0.0, 0.0, -10000, 0.0},
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
  {
    BufferDescriptor bufferDesc{
      .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
      .size = sizeof(m_ssao),
    };
    m_ssaoBuffer = m_ctx->device.CreateBuffer(&bufferDesc);
    m_ctx->queue.WriteBuffer(m_ssaoBuffer, 0, &m_ssao, sizeof(m_ssao));
  }

  // kernel uniform
  std::default_random_engine gen;
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0);

  size_t kSize = 64;
  std::vector<glm::vec4> ssaoKernel(kSize);
  for (size_t i = 0; i < kSize; ++i) {
    auto sample =
      glm::normalize(glm::vec3(
        randomFloats(gen) * 2.0 - 1.0, randomFloats(gen) * 2.0 - 1.0, randomFloats(gen)
      )) *
      randomFloats(gen);

    float scale = float(i) / kSize;
    scale = glm::lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;
    ssaoKernel[i] = {sample, 0.0};
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
    // auto noise = glm::normalize(
    //   glm::vec3(randomFloats(gen) * 2.0 - 1.0, randomFloats(gen) * 2.0 - 1.0, 0.0)
    // );
    glm::mat3 rmat =
      glm::rotate(randomFloats(gen) * glm::pi<float>() * 2.0f, glm::vec3(0, 0, 1));
    glm::vec3 noise = rmat * glm::vec3(1, 0, 0);
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
  Sampler nearestClampSampler;
  {
    SamplerDescriptor samplerDesc{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    };
    nearestClampSampler = m_ctx->device.CreateSampler(&samplerDesc);
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
        .sampler = nearestClampSampler,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.bgl_gBuffer,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_gBufferBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  // ssao sampling bind group
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
      BindGroupEntry{
        .binding = 3,
        .buffer = m_ssaoBuffer,
        .size = sizeof(SSAO),
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.bgl_ssaoSampling,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_ssaoSamplingBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
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
        .clearValue = {0.0, 0.0, 0.0, 0.0},
      },
    };
    m_ssaoPassDesc = {
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
    };
  }

  // blur pass ---------------------------------------------------
  // ssao texture bindgroup
  {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .textureView = ssaoTextureView,
      },
      BindGroupEntry{
        .binding = 1,
        .sampler = nearestClampSampler,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.bgl_ssaoTexture,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_ssaoTexureBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  // blur texture
  TextureView blurTextureView;
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = textureSize,
      .format = TextureFormat::R8Unorm,
    };
    Texture blurTexture = m_ctx->device.CreateTexture(&textureDesc);
    blurTextureView = blurTexture.CreateView();
  }

  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      RenderPassColorAttachment{
        .view = blurTextureView,
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 0.0},
      },
    };
    m_blurPassDesc = {
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
    };
  }

  // final pass ---------------------------------------------------
  // blur texture bindgroup
  {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .textureView = blurTextureView,
      },
      BindGroupEntry{
        .binding = 1,
        .sampler = nearestClampSampler,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.bgl_ssaoTexture,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_ssaoFinalTexureBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  {
    m_finalPassDesc = {
      .colorAttachmentCount = 1,
      .colorAttachments = nullptr,
    };
  }
}

void Renderer::Render() {
  // imgui
  if (!m_state->focused) {
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool isFirstFrame = true;
    if (isFirstFrame) {
      ImGui::SetNextWindowPos(ImVec2(m_state->size.x - 300, 30));
      ImGui::SetNextWindowSize(ImVec2(270, 200));
      isFirstFrame = false;
    }
    ImGui::Begin("Options");
    {
      bool tempEnabled = m_ssao.enabled;
      if (ImGui::Checkbox("##Hidden", &tempEnabled)) {
        m_ssao.enabled = tempEnabled;
        WRITE_SSAO_BUFFER(enabled);
      }
      ImGui::SameLine();
      if (ImGui::CollapsingHeader("SSAO")) {
        // children so reset button is centered
        if (ImGui::SliderInt("Sample Size", &m_ssao.sampleSize, 1, 64)) {
          WRITE_SSAO_BUFFER(sampleSize);
        }
        if (ImGui::SliderFloat("Radius", &m_ssao.radius, 0.0, 15.0)) {
          WRITE_SSAO_BUFFER(radius);
        }
        if (ImGui::SliderFloat("Bias", &m_ssao.bias, 0.0, 1.0)) {
          WRITE_SSAO_BUFFER(bias);
        }
        // center the button relative to the sliders
        if (ImGui::Button("Reset")) {
          m_ssao.SetDefault();
          m_ctx->queue.WriteBuffer(m_ssaoBuffer, 0, &m_ssao, sizeof(m_ssao));
        }
      }
    }
    ImGui::End();

    ImGui::Render();
  }

  // rendering
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
  m_finalPassDesc.colorAttachments = &colorAttachment;

  CommandEncoder commandEncoder = m_ctx->device.CreateCommandEncoder();
  // gbuffer pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_gBufferPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.rpl_gBuffer);
    passEncoder.SetBindGroup(0, m_state->player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_blocksTextureBindGroup);
    m_state->chunkManager->Render(passEncoder);
    passEncoder.End();
  }
  // ssao pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_ssaoPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.rpl_ssao);
    passEncoder.SetBindGroup(0, m_state->player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_gBufferBindGroup);
    passEncoder.SetBindGroup(2, m_ssaoSamplingBindGroup);
    passEncoder.SetVertexBuffer(0, m_quadBuffer);
    passEncoder.Draw(6);
    passEncoder.End();
  }
  // ssao-blur pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_blurPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.rpl_blur);
    passEncoder.SetBindGroup(0, m_ssaoTexureBindGroup);
    passEncoder.SetVertexBuffer(0, m_quadBuffer);
    passEncoder.Draw(6);
    passEncoder.End();
  }
  // final pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_finalPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.rpl_final);
    passEncoder.SetBindGroup(0, m_gBufferBindGroup);
    passEncoder.SetBindGroup(1, m_ssaoFinalTexureBindGroup);
    passEncoder.SetVertexBuffer(0, m_quadBuffer);
    passEncoder.Draw(6);
    if (!m_state->focused)
      ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), passEncoder.Get());
    passEncoder.End();
  }

  CommandBuffer command = commandEncoder.Finish();
  m_ctx->queue.Submit(1, &command);
}

void Renderer::Present() {
  m_ctx->swapChain.Present();
}

} // namespace util
