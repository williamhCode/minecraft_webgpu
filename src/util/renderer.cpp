#include "dawn/utils/WGPUHelpers.h"
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
#include "util/context.hpp"
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
  // init shared resources
  m_blocksTextureBindGroup = game::CreateBlocksTexture(*m_ctx);

  auto quadVertices = GetQuadVertices();
  m_quadBuffer = util::CreateVertexBuffer(
    m_ctx->device, quadVertices.size() * sizeof(QuadVertex), quadVertices.data()
  );

  Extent3D textureSize = {m_state->fbSize.x, m_state->fbSize.y, 1};

  // lighting
  Buffer sunDirBuffer =
    util::CreateUniformBuffer(m_ctx->device, sizeof(glm::vec3), &m_state->sunDir);
  m_lightingBindGroup = dawn::utils::MakeBindGroup(
    m_ctx->device, m_ctx->pipeline.lightingBGL, {{0, sunDirBuffer}}
  );

  // gbuffer pass -----------------------------------------------------
  // create textures: position (view-space), normal, color
  m_gBufferTextureViews[0] =
    util::CreateRenderTexture(m_ctx->device, textureSize, TextureFormat::RGBA16Float)
      .CreateView();
  m_gBufferTextureViews[1] =
    util::CreateRenderTexture(m_ctx->device, textureSize, TextureFormat::RGBA16Float)
      .CreateView();
  m_gBufferTextureViews[2] =
    util::CreateRenderTexture(m_ctx->device, textureSize, TextureFormat::BGRA8Unorm)
      .CreateView();

  // create depth texture
  m_depthTextureView =
    util::CreateRenderTexture(m_ctx->device, textureSize, m_ctx->depthFormat)
      .CreateView();

  // pass desc
  m_gBufferPassDesc = dawn::utils::ComboRenderPassDescriptor(
    {
      m_gBufferTextureViews[0],
      m_gBufferTextureViews[1],
      m_gBufferTextureViews[2],
    },
    m_depthTextureView
  );
  m_gBufferPassDesc.UnsetDepthStencilLoadStoreOpsForFormat(ctx->depthFormat);
  m_gBufferPassDesc.cColorAttachments[0].clearValue = {0.0, 0.0, -10000, 0.0};
  m_gBufferPassDesc.cColorAttachments[1].clearValue = {0.0, 0.0, 0.0, 0.0};
  m_gBufferPassDesc.cColorAttachments[2].clearValue = {0.5, 0.8, 0.9, 1.0};

  // water pass ---------------------------------------------------
  TextureView waterTextureView =
    util::CreateRenderTexture(m_ctx->device, textureSize, TextureFormat::BGRA8Unorm)
      .CreateView();

  // pass desc
  m_waterPassDesc =
    dawn::utils::ComboRenderPassDescriptor({waterTextureView}, m_depthTextureView);
  m_waterPassDesc.UnsetDepthStencilLoadStoreOpsForFormat(ctx->depthFormat);
  m_waterPassDesc.cDepthStencilAttachmentInfo.depthLoadOp = LoadOp::Load;
  m_waterPassDesc.cDepthStencilAttachmentInfo.depthStoreOp = StoreOp::Discard;

  // ssao pass ---------------------------------------------------
  m_ssaoBuffer = util::CreateUniformBuffer(m_ctx->device, sizeof(m_ssao), &m_ssao);

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

  Buffer kernelBuffer =
    util::CreateUniformBuffer(m_ctx->device, kernelSize, ssaoKernel.data());

  // noise texture
  std::vector<glm::vec4> ssaoNoise;
  for (unsigned int i = 0; i < 16; i++) {
    glm::mat3 rmat =
      glm::rotate(randomFloats(gen) * glm::pi<float>() * 2.0f, glm::vec3(0, 0, 1));
    glm::vec3 noise = rmat * glm::vec3(1, 0, 0);
    ssaoNoise.push_back({noise, 0.0});
  }

  TextureView noiseTextureView =
    util::CreateTexture(
      m_ctx->device, {4, 4, 1}, TextureFormat::RGBA16Float, ssaoNoise.data()
    )
      .CreateView();

  // samplers
  Sampler nearestClampSampler = m_ctx->device.CreateSampler( //
    ToPtr(SamplerDescriptor{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    })
  );

  Sampler noiseSampler = m_ctx->device.CreateSampler( //
    ToPtr(SamplerDescriptor{
      .addressModeU = AddressMode::Repeat,
      .addressModeV = AddressMode::Repeat,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    })
  );

  m_gBufferBindGroup = dawn::utils::MakeBindGroup(
    m_ctx->device, m_ctx->pipeline.gBufferBGL,
    {
      {0, m_gBufferTextureViews[0]},
      {1, m_gBufferTextureViews[1]},
      {2, m_gBufferTextureViews[2]},
      {3, nearestClampSampler},
    }
  );

  m_ssaoSamplingBindGroup = dawn::utils::MakeBindGroup(
    m_ctx->device, m_ctx->pipeline.ssaoSamplingBGL,
    {
      {0, kernelBuffer},
      {1, noiseTextureView},
      {2, noiseSampler},
      {3, m_ssaoBuffer},
    }
  );

  // ssao render textures, pre-blur and blurred
  std::array<wgpu::TextureView, 2> ssaoTextureViews;
  for (size_t i = 0; i < ssaoTextureViews.size(); i++) {
    ssaoTextureViews[i] =
      util::CreateRenderTexture(m_ctx->device, textureSize, TextureFormat::R8Unorm)
        .CreateView();
  }

  // pass desc
  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      {
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
    m_ssaoTextureBindGroups[i] = dawn::utils::MakeBindGroup(
      ctx->device, m_ctx->pipeline.ssaoTextureBGL,
      {
        {0, ssaoTextureViews[i]},
        {1, nearestClampSampler},
      }
    );
  }

  {
    static std::vector<wgpu::RenderPassColorAttachment> colorAttachments{
      {
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
  m_compositeBindGroup = dawn::utils::MakeBindGroup(
    m_ctx->device, m_ctx->pipeline.compositeBGL,
    {
      {0, ssaoTextureViews[1]},
      {1, waterTextureView},
    }
  );

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
    passEncoder.SetBindGroup(2, m_lightingBindGroup);
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
    passEncoder.SetBindGroup(0, m_state->player.camera.bindGroup);
    passEncoder.SetBindGroup(1, m_gBufferBindGroup);
    passEncoder.SetBindGroup(2, m_compositeBindGroup);
    passEncoder.SetBindGroup(3, m_lightingBindGroup);
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
