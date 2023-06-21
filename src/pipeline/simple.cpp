#include "simple.hpp"
#include "util/webgpu-util.hpp"
#include "glm-include.hpp"
#include <vector>

using namespace wgpu;

struct VertexAttributes {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 uv;
};

RenderPipeline createPipeline_simple(util::Handle &handle) {
  ShaderModule shaderModule =
    util::LoadShaderModule(SRC_DIR "/shaders/simple.wgsl", handle.device);

  // bind group layout
  std::vector<BindGroupLayout> bindGroupLayouts;
  {
    BindGroupLayoutEntry entry{
      .binding = 0,
      .visibility = ShaderStage::Vertex,
      .buffer{
        .type = BufferBindingType::Uniform,
        .minBindingSize = sizeof(glm::mat4),
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = 1,
      .entries = &entry,
    };
    bindGroupLayouts.push_back(handle.device.CreateBindGroupLayout(&desc));
  }
  {
    std::vector<BindGroupLayoutEntry> entries{
      {
        .binding = 0,
        .visibility = ShaderStage::Vertex,
        .buffer{
          .type = BufferBindingType::Uniform,
          .minBindingSize = sizeof(glm::mat4),
        },
      },
      {
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::Float,
          .viewDimension = TextureViewDimension::e2D,
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bindGroupLayouts.push_back(handle.device.CreateBindGroupLayout(&desc)
    );
  }

  PipelineLayoutDescriptor layoutDesc{
    .bindGroupLayoutCount = bindGroupLayouts.size(),
    .bindGroupLayouts = bindGroupLayouts.data(),
  };
  PipelineLayout pipelineLayout = handle.device.CreatePipelineLayout(&layoutDesc);

  // Vertex State ---------------------------------
  std::vector<VertexAttribute> vertexAttributes = {
    {
      .format = VertexFormat::Float32x3,
      .offset = 0,
      .shaderLocation = 0,
    },
    {
      .format = VertexFormat::Float32x3,
      .offset = offsetof(VertexAttributes, normal),
      .shaderLocation = 1,
    },
    {
      .format = VertexFormat::Float32x3,
      .offset = offsetof(VertexAttributes, color),
      .shaderLocation = 2,
    },
    {
      .format = VertexFormat::Float32x2,
      .offset = offsetof(VertexAttributes, uv),
      .shaderLocation = 3,
    },
  };
  VertexBufferLayout vertexBufferLayout{
    .arrayStride = sizeof(VertexAttributes),
    .stepMode = VertexStepMode::Vertex,
    .attributeCount = vertexAttributes.size(),
    .attributes = vertexAttributes.data(),
  };
  VertexState vertexState{
    .module = shaderModule,
    .entryPoint = "vs_main",
    .bufferCount = 1,
    .buffers = &vertexBufferLayout,
  };

  // Primitve State --------------------------------
  PrimitiveState primitiveState{
    .topology = PrimitiveTopology::TriangleList,
    .stripIndexFormat = IndexFormat::Undefined,
    .frontFace = FrontFace::CCW,
    .cullMode = CullMode::None,
  };

  // Depth Stencil State ---------------------------
  DepthStencilState depthStencilState{
    .format = TextureFormat::Depth24Plus,
    .depthWriteEnabled = true,
    .depthCompare = CompareFunction::Less,
  };

  // Fragment State --------------------------------
  BlendState blendState{
    .color{
      .operation = BlendOperation::Add,
      .srcFactor = BlendFactor::SrcAlpha,
      .dstFactor = BlendFactor::OneMinusSrcAlpha,
    },
    .alpha{
      .operation = BlendOperation::Add,
      .srcFactor = BlendFactor::Zero,
      .dstFactor = BlendFactor::One,
    },
  };
  ColorTargetState colorTarget{
    .format = handle.swapChainFormat,
    .blend = &blendState,
    .writeMask = ColorWriteMask::All,
  };
  FragmentState fragmentState{
    .module = shaderModule,
    .entryPoint = "fs_main",
    .targetCount = 1,
    .targets = &colorTarget,
  };

  // finally, create Render Pipeline
  RenderPipelineDescriptor pipelineDesc{
    .layout = pipelineLayout,
    .vertex = vertexState,
    .primitive = primitiveState,
    .depthStencil = &depthStencilState,
    .fragment = &fragmentState,
  };

  return handle.device.CreateRenderPipeline(&pipelineDesc);
}
