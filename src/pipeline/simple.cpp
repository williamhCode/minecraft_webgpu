#include "simple.hpp"
#include "util/webgpu-util.hpp"
#include "glm-include.hpp"
#include "game/mesh.hpp"
#include <vector>

using namespace wgpu;
using game::Vertex;

RenderPipeline CreatePipeline_simple(util::Handle &handle) {
  ShaderModule shaderModule =
    util::LoadShaderModule(SRC_DIR "/shaders/simple.wgsl", handle.device);

  // bind group layout
  std::vector<BindGroupLayout> bindGroupLayouts;
  // group 1
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
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bindGroupLayouts.push_back(handle.device.CreateBindGroupLayout(&desc));
  }
  // group 2
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
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bindGroupLayouts.push_back(handle.device.CreateBindGroupLayout(&desc));
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
      .offset = offsetof(Vertex, normal),
      .shaderLocation = 1,
    },
    {
      .format = VertexFormat::Float32x2,
      .offset = offsetof(Vertex, texCoord),
      .shaderLocation = 2,
    },
  };
  VertexBufferLayout vertexBufferLayout{
    .arrayStride = sizeof(Vertex),
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
