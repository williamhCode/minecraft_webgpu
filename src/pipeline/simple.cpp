#include "simple.hpp"
#include "util/webgpu-util.hpp"
#include "glm-include.hpp"

using namespace wgpu;

RenderPipeline createPipeline_simple(util::Handle &handle) {
  ShaderModule shaderModule =
    util::loadShaderModule(RESOURCE_DIR "/shader.wgsl", handle.device);

  // bind group layout
  BindGroupLayout bindGroupLayouts[2];
  {
    BindGroupLayoutEntry bindingLayout{
      .binding = 0,
      .visibility = ShaderStage::Vertex,
      .buffer{
        .type = BufferBindingType::Uniform,
        .minBindingSize = sizeof(glm::mat4),
      },
    };
    BindGroupLayoutDescriptor bindGroupLayoutDesc{
      .entryCount = 1,
      .entries = &bindingLayout,
    };
    bindGroupLayouts[0] = handle.device.CreateBindGroupLayout(&bindGroupLayoutDesc);
  }
  {
    BindGroupLayoutEntry bindingLayout{
      .binding = 0,
      .visibility = ShaderStage::Vertex,
      .buffer{
        .type = BufferBindingType::Uniform,
        .minBindingSize = sizeof(glm::mat4),
      },
    };
    BindGroupLayoutDescriptor bindGroupLayoutDesc{
      .entryCount = 1,
      .entries = &bindingLayout,
    };
    bindGroupLayouts[1] = handle.device.CreateBindGroupLayout(&bindGroupLayoutDesc);
  }

  PipelineLayoutDescriptor layoutDesc{
    .bindGroupLayoutCount = 2,
    .bindGroupLayouts = bindGroupLayouts,
  };
  PipelineLayout pipelineLayout = handle.device.CreatePipelineLayout(&layoutDesc);

  // Vertex State ---------------------------------
  VertexAttribute vertexAttribs[] = {
    {
      .format = VertexFormat::Float32x3,
      .offset = 0,
      .shaderLocation = 0,
    },
    {
      .format = VertexFormat::Float32x3,
      .offset = 3 * sizeof(float),
      .shaderLocation = 1,
    },
  };
  VertexBufferLayout vertexBufferLayout{
    .arrayStride = 6 * sizeof(float),
    .stepMode = VertexStepMode::Vertex,
    .attributeCount = 2,
    .attributes = vertexAttribs,
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
    .stencilReadMask = 0,
    .stencilWriteMask = 0,
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
