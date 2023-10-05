#include "pipeline.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "game/chunk.hpp"
#include "util/context.hpp"
#include "gfx/renderer.hpp"
#include "util/webgpu-util.hpp"
#include "game/mesh.hpp"
#include "glm-include.hpp"
#include "game/mesh.hpp"
#include <vector>

namespace gfx {

using namespace wgpu;
using VertexAttribs = game::Chunk::VertexAttribs;

Pipeline::Pipeline(util::Context &ctx) {
  // chunk vbo layout
  VertexBufferLayout chunkVBL;
  {
    static std::vector<VertexAttribute> vertexAttributes{
      {VertexFormat::Uint32, offsetof(VertexAttribs, data1), 0},
      {VertexFormat::Uint32, offsetof(VertexAttribs, data2), 1},
    };
    chunkVBL = {
      .arrayStride = sizeof(VertexAttribs),
      .attributeCount = vertexAttributes.size(),
      .attributes = vertexAttributes.data(),
    };
  }

  // cameraLayout
  cameraBGL = dawn::utils::MakeBindGroupLayout(
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
  // chunk layout (world pos)
  chunkBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
    }
  );
  // lighting layout (sunDir, sunViewProj)
  lightingBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, BufferBindingType::Uniform},
      // {1, ShaderStage::Vertex, BufferBindingType::Uniform},
    }
  );

  // shadow pipeline --------------------------------------------------
  // ShaderModule shaderVertShadow =
  //   util::LoadShaderModule(ROOT_DIR "/res/shaders/vert_shadow.wgsl", ctx.device);

  // shadowRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
  //   .layout = dawn::utils::MakePipelineLayout(
  //     ctx.device,
  //     {
  //       lightingBGL,
  //       chunkBGL,
  //     }
  //   ),
  //   .vertex =
  //     VertexState{
  //       .module = shaderVertShadow,
  //       .entryPoint = "vs_main",
  //       .bufferCount = 1,
  //       .buffers = &chunkVBL,
  //     },
  //   .primitive =
  //     PrimitiveState{
  //       .cullMode = CullMode::Back,
  //     },
  //   .depthStencil = ToPtr(DepthStencilState{
  //     .format = ctx.depthFormat,
  //     .depthWriteEnabled = true,
  //     .depthCompare = CompareFunction::Less,
  //   }),
  // }));

  // g_buffer pipeline -------------------------------------------------
  ShaderModule shaderGBuffer =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/g_buffer.wgsl", ctx.device);

  gBufferRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        cameraBGL,
        textureBGL,
        chunkBGL,
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
    .fragment = ToPtr(FragmentState{
      .module = shaderGBuffer,
      .entryPoint = "fs_main",
      .targetCount = 3,
      .targets = ToPtr<ColorTargetState>({
        {.format = TextureFormat::RGBA16Float}, // position
        {.format = TextureFormat::RGBA16Float}, // normal
        {
          .format = TextureFormat::BGRA8Unorm,
          .blend = &util::BlendState::ALPHA_BLENDING,
        }, // albedo
      }),
    }),
  }));

  // water pipeline --------------------------------------------------
  ShaderModule shaderWater =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/water.wgsl", ctx.device);

  waterRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        cameraBGL,
        textureBGL,
        lightingBGL,
        chunkBGL,
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
    .fragment = ToPtr(FragmentState{
      .module = shaderWater,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = ToPtr<ColorTargetState>({
        {.format = TextureFormat::BGRA8Unorm},
      }),
    }),
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
        cameraBGL,
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
    .fragment = ToPtr(FragmentState{
      .module = shaderFragSsao,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = ToPtr<ColorTargetState>({
        {.format = TextureFormat::R8Unorm},
      }),
    }),
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
    .fragment = ToPtr(FragmentState{
      .module = shaderFragBlur,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = ToPtr<ColorTargetState>({
        {.format = TextureFormat::R8Unorm},
      }),
    }),
  }));

  // composite pipeline --------------------------------------------------
  ShaderModule shaderFragComposite =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/frag_composite.wgsl", ctx.device);

  compositeBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {1, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
    }
  );

  compositeRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        cameraBGL,
        gBufferBGL,
        compositeBGL,
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
    .fragment = ToPtr(FragmentState{
      .module = shaderFragComposite,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = ToPtr<ColorTargetState>({
        {.format = TextureFormat::BGRA8Unorm},
      }),
    }),
  }));
}

} // namespace util
