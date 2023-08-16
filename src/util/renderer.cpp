#include "game.hpp"
#include "renderer.hpp"
#include "game/block.hpp"
#include <cfloat>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>
#include "glm/gtx/compatibility.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"
#include "util/webgpu-util.hpp"

namespace util {

using namespace wgpu;

std::vector<QuadVertex> GetQuadVertices() {
  return {
    {{-1.0, -1.0}, {0.0, 1.0}}, {{1.0, -1.0}, {1.0, 1.0}}, {{-1.0, 1.0}, {0.0, 0.0}},
    {{1.0, -1.0}, {1.0, 1.0}},  {{1.0, 1.0}, {1.0, 0.0}},  {{-1.0, 1.0}, {0.0, 0.0}},
  };
}

Renderer::Renderer(Context *ctx, GameState *state) : m_ctx(ctx), m_state(state) {
  m_blocksTextureBindGroup = game::CreateBlocksTexture(*m_ctx);

  // init quad buffer
  auto quadVertices = GetQuadVertices();
  m_quadBuffer = m_ctx->CreateVertexBuffer(quadVertices.size() * sizeof(QuadVertex), quadVertices.data());

  Extent3D textureSize = {m_state->fbSize.x, m_state->fbSize.y, 1};
  // gbuffer pass -----------------------------------------------------
  // create textures: position (view-space), normal, color
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

  // pass desc
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

  // water pass ---------------------------------------------------
  TextureView waterTextureView;
  {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = textureSize,
      .format = TextureFormat::BGRA8Unorm,
    };
    Texture waterTexture = m_ctx->device.CreateTexture(&textureDesc);
    waterTextureView = waterTexture.CreateView();
  }

  // pass desc
  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      RenderPassColorAttachment{
        .view = waterTextureView,
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 0.0},
      },
    };
    static RenderPassDepthStencilAttachment depthStencilAttachment{
      .view = m_depthTextureView,
      .depthLoadOp = LoadOp::Load,
      .depthStoreOp = StoreOp::Discard,
    };
    m_waterPassDesc = {
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
      .depthStencilAttachment = &depthStencilAttachment,
    };
  }

  // ssao pass ---------------------------------------------------
  m_ssaoBuffer = m_ctx->CreateUniformBuffer(sizeof(m_ssao), &m_ssao);

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

  Buffer kernelBuffer = m_ctx->CreateUniformBuffer(kernelSize, ssaoKernel.data());

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
      &destination, ssaoNoise.data(), sizeof(glm::vec4) * ssaoNoise.size(), &source,
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
      .layout = m_ctx->pipeline.gBufferBGL,
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
      .layout = m_ctx->pipeline.ssaoSamplingBGL,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_ssaoSamplingBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  // ssao render textures, pre-blur and blurred
  std::array<wgpu::TextureView, 2> ssaoTextureViews;
  for (size_t i = 0; i < ssaoTextureViews.size(); i++) {
    TextureDescriptor textureDesc{
      .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
      .size = textureSize,
      .format = TextureFormat::R8Unorm,
    };
    Texture ssaoTexture = m_ctx->device.CreateTexture(&textureDesc);
    ssaoTextureViews[i] = ssaoTexture.CreateView();
  }

  // pass desc
  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      RenderPassColorAttachment{
        .view = ssaoTextureViews[0],
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {1.0, 0.0, 0.0, 0.0},
      },
    };
    m_ssaoPassDesc = {
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
    };
  }

  // blur pass ---------------------------------------------------
  // ssao texture bindgroups
  for (size_t i = 0; i < m_ssaoTextureBindGroups.size(); i++) {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .textureView = ssaoTextureViews[i],
      },
      BindGroupEntry{
        .binding = 1,
        .sampler = nearestClampSampler,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.ssaoTextureBGL,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_ssaoTextureBindGroups[i] = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      RenderPassColorAttachment{
        .view = ssaoTextureViews[1],
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

  // composite pass ---------------------------------------------------
  // water texture bindgroup
  {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .textureView = waterTextureView,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.waterTextureBGL,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    m_waterTextureBindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  {
    m_compositePassDesc = {
      .colorAttachmentCount = 1,
      .colorAttachments = nullptr,
    };
  }
}

void Renderer::ImguiRender() {
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if (m_state->showStats) {
    ImGui::Begin("Statistics");
    {
      const int NUM_FRAMES = 200;
      static float frameTimes[NUM_FRAMES] = {};
      static int frameTimeIndex = 0;

      frameTimes[frameTimeIndex] = m_state->dt;
      frameTimeIndex = (frameTimeIndex + 1) % NUM_FRAMES;

      ImGui::Text("FPS: %.3f", m_state->fps);
      ImGui::Text("Frame Times");
      ImGui::PlotHistogram(
        "##Frame Times", frameTimes, NUM_FRAMES, frameTimeIndex, nullptr, 0.0f,
        0.033333f, ImVec2(ImGui::GetWindowSize().x - 20, 150)
      );
    }
    ImGui::End();
  }

  if (!m_state->focused) {
    static bool isFirstFrame = true;
    if (isFirstFrame) {
      // ImGui::SetNextWindowPos(ImVec2(m_state->size.x - 300, 30));
      // ImGui::SetNextWindowSize(ImVec2(270, 200));
      isFirstFrame = false;
    }
    ImGui::Begin("Options");
    {
      // ssao options ---------------------------------------------
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
        if (ImGui::DragFloat("Radius##ssao", &m_ssao.radius, 0.5, 0.0, FLT_MAX)) {
          WRITE_SSAO_BUFFER(radius);
        }
        if (ImGui::SliderFloat("Bias", &m_ssao.bias, 0.0, 5.0)) {
          WRITE_SSAO_BUFFER(bias);
        }
        // center the button relative to the sliders
        if (ImGui::Button("Reset")) {
          m_ssao.SetDefault();
          m_ctx->queue.WriteBuffer(m_ssaoBuffer, 0, &m_ssao, sizeof(m_ssao));
        }
      }

      // chunk options ---------------------------------------------
      if (ImGui::CollapsingHeader("Chunk")) {
        ImGui::SliderInt("Radius##chunk", &m_state->chunkManager->radius, 0, 64);
        if (ImGui::DragInt("Max Gens", &m_state->chunkManager->max_gens, 1, 1, 100)) {
          if (m_state->chunkManager->max_gens < 1) {
            m_state->chunkManager->max_gens = 1;
          }
        }
      }
    }
    ImGui::End();
  }
  ImGui::Render();
}

void Renderer::Render() {
  ImguiRender();

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
  m_compositePassDesc.colorAttachments = &colorAttachment;

  CommandEncoder commandEncoder = m_ctx->device.CreateCommandEncoder();
  // gbuffer pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_gBufferPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.gBufferRPL);
    passEncoder.SetBindGroup(0, m_state->player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_blocksTextureBindGroup);
    m_state->chunkManager->Render(passEncoder);
    passEncoder.End();
  }
  // water pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_waterPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.waterRPL);
    passEncoder.SetBindGroup(0, m_state->player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_blocksTextureBindGroup);
    m_state->chunkManager->RenderWater(passEncoder);
    passEncoder.End();
  }
  // ssao pass
  {
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&m_ssaoPassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.ssaoRPL);
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
    passEncoder.SetPipeline(m_ctx->pipeline.blurRPL);
    passEncoder.SetBindGroup(0, m_ssaoTextureBindGroups[0]);
    passEncoder.SetVertexBuffer(0, m_quadBuffer);
    passEncoder.Draw(6);
    passEncoder.End();
  }
  // composite pass
  {
    RenderPassEncoder passEncoder =
      commandEncoder.BeginRenderPass(&m_compositePassDesc);
    passEncoder.SetPipeline(m_ctx->pipeline.compositeRPL);
    passEncoder.SetBindGroup(0, m_gBufferBindGroup);
    passEncoder.SetBindGroup(1, m_ssaoTextureBindGroups[1]);
    passEncoder.SetBindGroup(2, m_waterTextureBindGroup);
    passEncoder.SetVertexBuffer(0, m_quadBuffer);
    passEncoder.Draw(6);
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
