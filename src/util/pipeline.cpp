#include "pipeline.hpp"
#include "util/handle.hpp"
#include "util/webgpu-util.hpp"
#include "game/mesh.hpp"
#include "glm-include.hpp"
#include "game/mesh.hpp"
#include <vector>

namespace util {

using namespace wgpu;
using game::Vertex;

Pipeline::Pipeline(Handle &handle) {
  ShaderModule shaderModule =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/g_buffer.wgsl", handle.device);

  // view, projection
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
      BindGroupLayoutEntry{
        .binding = 1,
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
    bgl_viewProj = handle.device.CreateBindGroupLayout(&desc);
  }
  // texture
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::Float,
          .viewDimension = TextureViewDimension::e2D,
        }},
      BindGroupLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .sampler{
          .type = SamplerBindingType::Filtering,
        }},
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_texture = handle.device.CreateBindGroupLayout(&desc);
  }
  // offset
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Vertex,
        .buffer{
          .type = BufferBindingType::Uniform,
          .minBindingSize = sizeof(glm::vec3),
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_offset = handle.device.CreateBindGroupLayout(&desc);
  }

  std::vector<BindGroupLayout> bindGroupLayouts{
    bgl_viewProj,
    bgl_texture,
    bgl_offset,
  };
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
    // .cullMode = CullMode::Back,
    .cullMode = CullMode::None,
  };

  // Depth Stencil State ---------------------------
  DepthStencilState depthStencilState{
    .format = TextureFormat::Depth24Plus,
    .depthWriteEnabled = true,
    .depthCompare = CompareFunction::Less,
  };

  // Fragment State --------------------------------
  // BlendState blendState{
  //   .color{
  //     .operation = BlendOperation::Add,
  //     .srcFactor = BlendFactor::SrcAlpha,
  //     .dstFactor = BlendFactor::OneMinusSrcAlpha,
  //   },
  // };
  std::vector<ColorTargetState> targets{
    // position
    ColorTargetState{
      .format = TextureFormat::RGBA16Float,
    },
    // normal
    ColorTargetState{
      .format = TextureFormat::RGBA16Float,
    },
    // albedo
    ColorTargetState{
      .format = TextureFormat::BGRA8Unorm,
    },
  };
  FragmentState fragmentState{
    .module = shaderModule,
    .entryPoint = "fs_main",
    .targetCount = targets.size(),
    .targets = targets.data(),
  };

  // finally, create Render Pipeline
  RenderPipelineDescriptor pipelineDesc{
    .layout = pipelineLayout,
    .vertex = vertexState,
    .primitive = primitiveState,
    .depthStencil = &depthStencilState,
    .fragment = &fragmentState,
  };

  rpl_gBuffer = handle.device.CreateRenderPipeline(&pipelineDesc);
}

} // namespace util
