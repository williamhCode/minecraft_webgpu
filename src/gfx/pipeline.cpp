#include "pipeline.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "game/chunk.hpp"
#include "gfx/context.hpp"
#include "util/webgpu-util.hpp"
#include <vector>

namespace gfx {

using namespace wgpu;
using VertexAttribs = game::Chunk::VertexAttribs;

Pipeline::Pipeline(gfx::Context &ctx) {
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
  sunBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, BufferBindingType::Uniform},
      {1, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::ReadOnlyStorage},
      {2, ShaderStage::Fragment, BufferBindingType::Uniform},
    }
  );

  // shadow pipeline --------------------------------------------------
  ShaderModule shaderShadow =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/shadow.wgsl", ctx.device);

  shadowBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device, {{0, ShaderStage::Vertex, BufferBindingType::Uniform}}
  );

  shadowRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        shadowBGL,
        sunBGL,
        textureBGL,
        chunkBGL,
      }
    ),
    .vertex =
      VertexState{
        .module = shaderShadow,
        .entryPoint = "vs_main",
        .bufferCount = 1,
        .buffers = &chunkVBL,
      },
    .primitive =
      PrimitiveState{
        .cullMode = CullMode::Back,
      },
    .depthStencil = ToPtr(DepthStencilState{
      .format = TextureFormat::Depth32Float,
      .depthWriteEnabled = true,
      .depthCompare = CompareFunction::Less,
    }),
    .fragment = ToPtr(FragmentState{
      .module = shaderShadow,
      .entryPoint = "fs_main",
    }),
  }));

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
          .blend = &util::BlendState::AlphaBlending,
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
        sunBGL,
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
      {2, ShaderStage::Fragment, TextureSampleType::Depth,
       TextureViewDimension::e2DArray},
      {3, ShaderStage::Fragment, SamplerBindingType::Comparison},
    }
  );

  compositeRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        cameraBGL,
        gBufferBGL,
        sunBGL,
        compositeBGL,
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

} // namespace gfx
