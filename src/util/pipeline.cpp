#include "pipeline.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "util/context.hpp"
#include "util/renderer.hpp"
#include "util/webgpu-util.hpp"
#include "game/mesh.hpp"
#include "glm-include.hpp"
#include "game/mesh.hpp"
#include <vector>

namespace util {

using namespace wgpu;
using game::Vertex;

Pipeline::Pipeline(Context &ctx) {
  // chunk vbo layout
  VertexBufferLayout chunkVBL;
  {
    static std::vector<VertexAttribute> vertexAttributes{
      {VertexFormat::Float32x3, offsetof(Vertex, position), 0},
      {VertexFormat::Float32x3, offsetof(Vertex, normal), 1},
      {VertexFormat::Float32x2, offsetof(Vertex, uv), 2},
      {VertexFormat::Uint32, offsetof(Vertex, extraData), 3},
    };
    chunkVBL = {
      .arrayStride = sizeof(Vertex),
      .attributeCount = vertexAttributes.size(),
      .attributes = vertexAttributes.data(),
    };
  }

  // view, projection layout
  viewProjBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
      {1, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
      {2, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
    }
  );
  // texture layout
  textureBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::Float},
      {1, ShaderStage::Fragment, SamplerBindingType::Filtering},
    }
  );

  // water pipeline --------------------------------------------------
  ShaderModule shaderWater =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/water.wgsl", ctx.device);

  waterRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        textureBGL,
      }
    ),
    .vertex =
      VertexState{
        .module = shaderWater,
        .entryPoint = "vs_main",
        .bufferCount = 1,
        .buffers = &chunkVBL,
      },
    .primitive =
      PrimitiveState{
        .topology = PrimitiveTopology::TriangleList,
      },
    .depthStencil = ToPtr(DepthStencilState{
      .format = ctx.depthFormat,
      .depthWriteEnabled = true,
      .depthCompare = CompareFunction::Less,
    }),
    .fragment = ToPtr([&] {
      static std::vector<ColorTargetState> targets{
        {.format = TextureFormat::BGRA8Unorm},
      };
      return FragmentState{
        .module = shaderWater,
        .entryPoint = "fs_main",
        .targetCount = targets.size(),
        .targets = targets.data(),
      };
    }()),
  }));

  // g_buffer pipeline -------------------------------------------------
  ShaderModule shaderGBuffer =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/g_buffer.wgsl", ctx.device);

  gBufferRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        textureBGL,
      }
    ),
    .vertex =
      VertexState{
        .module = shaderGBuffer,
        .entryPoint = "vs_main",
        .bufferCount = 1,
        .buffers = &chunkVBL,
      },
    .primitive =
      PrimitiveState{
        .cullMode = CullMode::Back,
      },
    .depthStencil = ToPtr(DepthStencilState{
      .format = ctx.depthFormat,
      .depthWriteEnabled = true,
      .depthCompare = CompareFunction::Less,
    }),
    .fragment = ToPtr([&] {
      static std::vector<ColorTargetState> targets{
        {.format = TextureFormat::RGBA16Float}, // position
        {.format = TextureFormat::RGBA16Float}, // normal
        {
          .format = TextureFormat::BGRA8Unorm,
          .blend = &UBlendState::ALPHA_BLENDING,
        }, // albedo
      };
      return FragmentState{
        .module = shaderGBuffer,
        .entryPoint = "fs_main",
        .targetCount = targets.size(),
        .targets = targets.data(),
      };
    }()),
  }));

  // ssao pipeline -------------------------------------------------
  ShaderModule shaderVertQuad =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/vert_quad.wgsl", ctx.device);
  ShaderModule shaderFragSsao =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/frag_ssao.wgsl", ctx.device);

  std::vector<VertexAttribute> vertexAttributes{
    {VertexFormat::Float32x2, 0, 0},
    {VertexFormat::Float32x2, sizeof(glm::vec2), 1},
  };
  VertexBufferLayout quadVertexBufferLayout{
    .arrayStride = sizeof(glm::vec2) * 2,
    .stepMode = VertexStepMode::Vertex,
    .attributeCount = vertexAttributes.size(),
    .attributes = vertexAttributes.data(),
  };

  gBufferBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {1, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {2, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {3, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
    }
  );

  ssaoSamplingBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, BufferBindingType::Uniform},
      {1, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {2, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
      {3, ShaderStage::Fragment, BufferBindingType::Uniform},
    }
  );

  ssaoRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        gBufferBGL,
        ssaoSamplingBGL,
      }
    ),
    .vertex =
      VertexState{
        .module = shaderVertQuad,
        .entryPoint = "vs_main",
        .bufferCount = 1,
        .buffers = &quadVertexBufferLayout,
      },
    .primitive = PrimitiveState{},
    .fragment = ToPtr([&] {
      static std::vector<ColorTargetState> targets{
        {.format = TextureFormat::R8Unorm}, // ssao texture
      };
      return FragmentState{
        .module = shaderFragSsao,
        .entryPoint = "fs_main",
        .targetCount = targets.size(),
        .targets = targets.data(),
      };
    }()),
  }));

  // blur pipeline --------------------------------------------------
  ShaderModule shaderFragBlur =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/frag_blur.wgsl", ctx.device);

  ssaoTextureBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {1, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
    }
  );

  blurRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(ctx.device, {ssaoTextureBGL}),
    .vertex =
      VertexState{
        .module = shaderVertQuad,
        .entryPoint = "vs_main",
        .bufferCount = 1,
        .buffers = &quadVertexBufferLayout,
      },
    .primitive = PrimitiveState{},
    .fragment = ToPtr([&] {
      static std::vector<ColorTargetState> targets{
        {.format = TextureFormat::R8Unorm}, // ssao texture
      };
      return FragmentState{
        .module = shaderFragBlur,
        .entryPoint = "fs_main",
        .targetCount = targets.size(),
        .targets = targets.data(),
      };
    }()),
  }));

  // composite pipeline --------------------------------------------------
  ShaderModule shaderFragComposite =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/frag_composite.wgsl", ctx.device);

  waterTextureBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device, {{0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat}}
  );

  lightingBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device, {{0, ShaderStage::Fragment, BufferBindingType::Uniform}}
  );

  compositeRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        gBufferBGL,
        ssaoTextureBGL,
        waterTextureBGL,
        lightingBGL,
      }
    ),
    .vertex =
      VertexState{
        .module = shaderVertQuad,
        .entryPoint = "vs_main",
        .bufferCount = 1,
        .buffers = &quadVertexBufferLayout,
      },
    .primitive = PrimitiveState{},
    .fragment = ToPtr([&] {
      static std::vector<ColorTargetState> targets{
        {.format = TextureFormat::BGRA8Unorm},
      };
      return FragmentState{
        .module = shaderFragComposite,
        .entryPoint = "fs_main",
        .targetCount = targets.size(),
        .targets = targets.data(),
      };
    }()),
  }));
}

} // namespace util
