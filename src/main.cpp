// An optional library that makes displaying enum values much easier
// #include "magic_enum.hpp"

#include <GLFW/glfw3.h>

#include "webgpu-utils.h"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace wgpu;
namespace fs = std::filesystem;

struct MyUniforms {
    std::array<float, 16> projectionMatrix;
    std::array<float, 16> viewMatrix;
    std::array<float, 16> modelMatrix;
    std::array<float, 4> color;
    float time;
    float _pad[3];
};

ShaderModule loadShaderModule(const fs::path &path, Device device);
bool loadGeometry(
  const fs::path &path, std::vector<float> &pointData, std::vector<uint16_t> &indexData,
  int dimensions
);

int main() {
  Instance instance = CreateInstance();
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return 1;
  }

  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
  if (!window) {
    std::cerr << "Could not open window!" << std::endl;
    return 1;
  }

  std::cout << "Requesting adapter..." << std::endl;
  Surface surface = glfw::CreateSurfaceForWindow(instance, window);
  RequestAdapterOptions adapterOpts{};
  Adapter adapter = wgpu_utils::RequestAdapter(instance, &adapterOpts);
  std::cout << "Got adapter: " << adapter.Get() << std::endl;

  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  Limits deviceLimits = supportedLimits.limits;

  std::cout << "Requesting device..." << std::endl;
  DeviceDescriptor deviceDesc{
    .label = "My Device",
    .requiredFeaturesCount = 0,
    .requiredLimits = nullptr,
    .defaultQueue{.label = "The default queue"},
  };
  Device device = wgpu_utils::RequestDevice(adapter, &deviceDesc);
  std::cout << "Got device: " << device.Get() << std::endl;
  wgpu_utils::SetUncapturedErrorCallback(device);

  Queue queue = device.GetQueue();

  std::cout << "Creating swapchain device..." << std::endl;
#ifdef WEBGPU_BACKEND_WGPU
  TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
#else
  TextureFormat swapChainFormat = TextureFormat::BGRA8Unorm;
#endif

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = static_cast<uint32_t>(width),
    .height = static_cast<uint32_t>(height),
    .presentMode = PresentMode::Fifo,
  };
  SwapChain swapChain = device.CreateSwapChain(surface, &swapChainDesc);
  std::cout << "Swapchain: " << swapChain.Get() << std::endl;

  std::cout << "Creating shader module..." << std::endl;
  ShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", device);
  std::cout << "Shader module: " << shaderModule.Get() << std::endl;

  // create render pipeline ------------------------------------------------ //
  std::cout << "Creating render pipeline..." << std::endl;

  // PipelineLayout -----------------------------
  BindGroupLayoutEntry bindingLayout{
    // The binding index as used in the @binding attribute in the shader
    .binding = 0,
    .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
    .buffer{
      .type = BufferBindingType::Uniform,
      .hasDynamicOffset = true,
      .minBindingSize = sizeof(MyUniforms),
    },
  };
  BindGroupLayoutDescriptor bindGroupLayoutDesc{
    .entryCount = 1,
    .entries = &bindingLayout,
  };
  BindGroupLayout bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);
  PipelineLayoutDescriptor layoutDesc{
    .bindGroupLayoutCount = 1,
    .bindGroupLayouts = &bindGroupLayout,
  };
  PipelineLayout pipelineLayout = device.CreatePipelineLayout(&layoutDesc);

  // Vertex State ---------------------------------
  std::vector<VertexAttribute> vertexAttribs{
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
    .attributeCount = static_cast<uint32_t>((vertexAttribs.size())),
    .attributes = vertexAttribs.data(),
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
  TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
  DepthStencilState depthStencilState{
    .format = depthTextureFormat,
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
    .format = swapChainFormat,
    .blend = &blendState,
    .writeMask = ColorWriteMask::All,
  };
  FragmentState fragmentState{
    .module = shaderModule,
    .entryPoint = "fs_main",
    .constantCount = 0,
    .constants = nullptr,
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
  RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);
  std::cout << "Render pipeline: " << pipeline.Get() << std::endl;

  // data
  std::vector<float> pointData;
  std::vector<uint16_t> indexData;

  bool success = loadGeometry(RESOURCE_DIR "/pyramid.txt", pointData, indexData, 3);
  if (!success) {
    std::cerr << "Could not load geometry!" << std::endl;
    return 1;
  }

  int indexCount = static_cast<int>(indexData.size());

  // Create vertex buffer
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
    .size = pointData.size() * sizeof(float),
  };
  Buffer vertexBuffer = device.CreateBuffer(&bufferDesc);
  queue.WriteBuffer(vertexBuffer, 0, pointData.data(), bufferDesc.size);

  // Create index buffer
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
  bufferDesc.size = indexData.size() * sizeof(float);
  Buffer indexBuffer = device.CreateBuffer(&bufferDesc);
  queue.WriteBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

  // Create uniform buffer
  uint32_t uniformStride = std::max(
    (uint32_t)sizeof(MyUniforms), (uint32_t)deviceLimits.minUniformBufferOffsetAlignment
  );
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  bufferDesc.size = sizeof(MyUniforms);
  // bufferDesc.size = uniformStride + sizeof(MyUniforms);
  Buffer uniformBuffer = device.CreateBuffer(&bufferDesc);

  // Upload the initial value of the uniforms
  MyUniforms uniforms{
    .color = {0.0f, 1.0f, 0.4f, 1.0f},
    // .color = {1.0f, 1.0f, 1.0f, 1.0f},
    .time = 1.0f,
  };
  queue.WriteBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

  // Upload second value
  //  uniforms.time = -1.0f;
  // uniforms.color = { 1.0f, 1.0f, 1.0f, 0.7f };
  // queue.WriteBuffer(uniformBuffer, uniformStride, &uniforms, sizeof(MyUniforms));

  // Create a binding
  BindGroupEntry binding{
    .binding = 0,
    .buffer = uniformBuffer,
    .offset = 0,
    .size = sizeof(MyUniforms),
  };
  // A bind group contains one or multiple bindings
  BindGroupDescriptor bindGroupDesc{
    .layout = bindGroupLayout,
    // There must be as many bindings as declared in the layout!
    .entryCount = bindGroupLayoutDesc.entryCount,
    .entries = &binding,
  };
  BindGroup bindGroup = device.CreateBindGroup(&bindGroupDesc);

  // Create the depth texture
  TextureDescriptor depthTextureDesc{
    .usage = TextureUsage::RenderAttachment,
    .size = {swapChainDesc.width, swapChainDesc.height},
    .format = depthTextureFormat,
    .viewFormatCount = 1,
    .viewFormats = &depthTextureFormat,
  };
  Texture depthTexture = device.CreateTexture(&depthTextureDesc);

  // Create the view of the depth texture manipulated by the rasterizer
  TextureViewDescriptor depthTextureViewDesc{
    .format = depthTextureFormat,
    .baseMipLevel = 0,
    .mipLevelCount = 1,
    .baseArrayLayer = 0,
    .arrayLayerCount = 1,
    .aspect = TextureAspect::DepthOnly,
  };
  TextureView depthTextureView = depthTexture.CreateView(&depthTextureViewDesc);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Update uniform buffer
    uniforms.time = static_cast<float>(glfwGetTime());
    queue.WriteBuffer(
      uniformBuffer, offsetof(MyUniforms, time), &uniforms.time,
      sizeof(MyUniforms::time)
    );

    TextureView nextTexture = swapChain.GetCurrentTextureView();
    if (!nextTexture) {
      std::cerr << "Cannot acquire next swap chain texture" << std::endl;
      return 1;
    }

    CommandEncoderDescriptor commandEncoderDesc{.label = "Render encoder"};
    CommandEncoder encoder = device.CreateCommandEncoder(&commandEncoderDesc);

    RenderPassColorAttachment renderPassColorAttachment{
      .view = nextTexture,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = Color{0.05, 0.05, 0.05, 1.0},
    };
    RenderPassDepthStencilAttachment depthStencilAttachment{
      .view = depthTextureView,
      .depthLoadOp = LoadOp::Clear,
      .depthStoreOp = StoreOp::Store,
      .depthClearValue = 1.0f,
      .depthReadOnly = false,
      // .stencilLoadOp = LoadOp::Clear,
      // .stencilStoreOp = StoreOp::Store,
      // .stencilClearValue = 0,
      // .stencilReadOnly = true,
    };
    RenderPassDescriptor renderPassDesc{
      .colorAttachmentCount = 1,
      .colorAttachments = &renderPassColorAttachment,
      .depthStencilAttachment = &depthStencilAttachment,
    };
    RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);

    renderPass.SetPipeline(pipeline);

    renderPass.SetVertexBuffer(0, vertexBuffer, 0, pointData.size() * sizeof(float));
    renderPass.SetIndexBuffer(
      indexBuffer, IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t)
    );

    uint32_t dynamicOffset = 0;

    // Set binding group
    // dynamicOffset = 0 * uniformStride;
    renderPass.SetBindGroup(0, bindGroup, 1, &dynamicOffset);
    renderPass.DrawIndexed(indexCount, 1, 0, 0, 0);

    // Set binding group with a different uniform offset
    // dynamicOffset = 1 * uniformStride;
    // renderPass.SetBindGroup(0, bindGroup, 1, &dynamicOffset);
    // renderPass.DrawIndexed(indexCount, 1, 0, 0, 0);

    renderPass.End();

    CommandBufferDescriptor cmdBufferDescriptor{.label = "Command buffer"};
    CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
    queue.Submit(1, &command);

    swapChain.Present();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

ShaderModule loadShaderModule(const fs::path &path, Device device) {
  std::ifstream file{path};
  if (!file.is_open()) {
    return nullptr;
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  std::string shaderSource(size, ' ');
  file.seekg(0);
  file.read(shaderSource.data(), size);

  ShaderModuleWGSLDescriptor shaderCodeDesc{};
  shaderCodeDesc.nextInChain = nullptr;
  shaderCodeDesc.code = shaderSource.c_str();
  ShaderModuleDescriptor shaderDesc{.nextInChain = &shaderCodeDesc};
  return device.CreateShaderModule(&shaderDesc);
}

bool loadGeometry(
  const fs::path &path, std::vector<float> &pointData, std::vector<uint16_t> &indexData,
  int dimensions
) {
  std::ifstream file{path};
  if (!file.is_open()) {
    return false;
  }

  pointData.clear();
  indexData.clear();

  enum class Section {
    None,
    Points,
    Indices,
  };
  Section currentSection = Section::None;

  float value;
  uint16_t index;
  std::string line;
  while (!file.eof()) {
    getline(file, line);

    // overcome the `CRLF` problem
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (line == "[points]") {
      currentSection = Section::Points;
    } else if (line == "[indices]") {
      currentSection = Section::Indices;
    } else if (line[0] == '#' || line.empty()) {
      // Do nothing, this is a comment
    } else if (currentSection == Section::Points) {
      std::istringstream iss(line);
      // Get x, y, r, g, b
      for (int i = 0; i < dimensions + 3; ++i) {
        iss >> value;
        pointData.push_back(value);
      }
    } else if (currentSection == Section::Indices) {
      std::istringstream iss(line);
      // Get corners #0 #1 and #2
      for (int i = 0; i < 3; ++i) {
        iss >> index;
        indexData.push_back(index);
      }
    }
  }
  return true;
}
