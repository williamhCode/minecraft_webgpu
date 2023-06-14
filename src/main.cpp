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

/**
 * The same structure as in the shader, replicated in C++
 */
struct MyUniforms {
  std::array<float, 4> color;
  float time;
  float _pad[3];
};

ShaderModule loadShaderModule(const fs::path &path, Device device);
bool loadGeometry(
  const fs::path &path, std::vector<float> &pointData, std::vector<uint16_t> &indexData
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

  // SupportedLimits supportedLimits;
  // adapter.GetLimits(&supportedLimits);

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
      .format = VertexFormat::Float32x2,
      .offset = 0,
      .shaderLocation = 0,
    },
    {
      .format = VertexFormat::Float32x3,
      .offset = 2 * sizeof(float),
      .shaderLocation = 1,
    },
  };
  VertexBufferLayout vertexBufferLayout{
    .arrayStride = 5 * sizeof(float),
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
    .fragment = &fragmentState,
  };
  RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);
  std::cout << "Render pipeline: " << pipeline.Get() << std::endl;

  // data
  std::vector<float> pointData;
  std::vector<uint16_t> indexData;

  bool success = loadGeometry(RESOURCE_DIR "/webgpu.txt", pointData, indexData);
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
  bufferDesc.size = indexData.size() * sizeof(float);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
  Buffer indexBuffer = device.CreateBuffer(&bufferDesc);
  queue.WriteBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

  // Create uniform buffer
  bufferDesc.size = sizeof(MyUniforms);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  Buffer uniformBuffer = device.CreateBuffer(&bufferDesc);

  // Upload the initial value of the uniforms
  MyUniforms uniforms{
    .color = {0.0f, 1.0f, 0.4f, 1.0f},
    .time = 1.0f,
  };
  queue.WriteBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

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

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Update uniform buffer
    uniforms.time = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
    // Only update the 1-st float of the buffer
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
    RenderPassDescriptor renderPassDesc{
      .colorAttachmentCount = 1,
      .colorAttachments = &renderPassColorAttachment,
    };
    RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);

    renderPass.SetPipeline(pipeline);
    // Set both vertex and index buffers
    renderPass.SetVertexBuffer(0, vertexBuffer, 0, pointData.size() * sizeof(float));
    // The second argument must correspond to the choice of uint16_t or uint32_t
    // we've done when creating the index buffer.
    renderPass.SetIndexBuffer(
      indexBuffer, IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t)
    );
    // Set binding group
    renderPass.SetBindGroup(0, bindGroup, 0, nullptr);
    renderPass.DrawIndexed(indexCount, 1, 0, 0, 0);
    renderPass.End();

    CommandBufferDescriptor cmdBufferDescriptor{.label = "Command buffer"};
    CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
    queue.Submit(1, &command);

    swapChain.Present();

    // release resources
    nextTexture.Release();
  }

  // release resources
  swapChain.Release();
  device.Release();
  adapter.Release();
  instance.Release();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

ShaderModule loadShaderModule(const fs::path &path, Device device) {
  std::ifstream file(path);
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
  shaderCodeDesc.source = shaderSource.c_str();
  ShaderModuleDescriptor shaderDesc{};
  shaderDesc.nextInChain = &shaderCodeDesc;
  return device.CreateShaderModule(&shaderDesc);
}

bool loadGeometry(
  const fs::path &path, std::vector<float> &pointData, std::vector<uint16_t> &indexData
) {
  std::ifstream file(path);
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
      for (int i = 0; i < 5; ++i) {
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
