#include "simple.hpp"
#include "util/webgpu-util.hpp"
#include "glm-include.hpp"
#include "game/mesh.hpp"
#include <vector>

namespace util {

using namespace wgpu;
using game::Vertex;

RenderPipeline CreatePipelineSimple(util::Handle &handle) {
  ShaderModule shaderModule =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/simple.wgsl", handle.device);

  // bind group layout
  std::vector<BindGroupLayout> bindGroupLayouts;
  // group 0
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
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
  // group 1
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
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
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::Float,
          .viewDimension = TextureViewDimension::e2D,
        }
      },
      BindGroupLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .sampler{
          .type = SamplerBindingType::Filtering,
        }
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
    VertexAttribute{
      .format = VertexFormat::Float32x3,
      .offset = offsetof(Vertex, position),
      .shaderLocation = 0,
    },
    VertexAttribute{
      .format = VertexFormat::Float32x3,
      .offset = offsetof(Vertex, normal),
      .shaderLocation = 1,
    },
    VertexAttribute{
      .format = VertexFormat::Float32x2,
      .offset = offsetof(Vertex, uv),
      .shaderLocation = 2,
    },
    VertexAttribute{
      .format = VertexFormat::Float32x2,
      .offset = offsetof(Vertex, texLoc),
      .shaderLocation = 3,
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
    .cullMode = CullMode::Back,
    // .cullMode = CullMode::None,
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

} // namespace util
