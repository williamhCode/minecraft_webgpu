// #include "webgpu-release.h"

// An optional library that makes displaying enum values much easier
#include "magic_enum.hpp"

#include <GLFW/glfw3.h>

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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
    const fs::path &path, std::vector<float> &pointData,
    std::vector<uint16_t> &indexData
);

int main(int, char **) {
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

  struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestAdapterStatus_Success) {
			userData.adapter = adapter;
		} else {
			std::cout << "Could not get WebGPU adapter: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

  instance.RequestAdapter(&adapterOpts, onAdapterRequestEnded, &userData);

  std::cout << "Got adapter: " << userData.adapter << std::endl;

  SupportedLimits supportedLimits;
  adapter.getLimits(&supportedLimits);

  // Don't forget to = Default
  // RequiredLimits requiredLimits = Default;
  // requiredLimits.limits.maxVertexAttributes = 2;
  // requiredLimits.limits.maxVertexBuffers = 1;
  // requiredLimits.limits.maxBufferSize = 6 * 5 * sizeof(float);
  // requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
  // requiredLimits.limits.minStorageBufferOffsetAlignment =
  //     supportedLimits.limits.minStorageBufferOffsetAlignment;
  // requiredLimits.limits.minUniformBufferOffsetAlignment =
  //     supportedLimits.limits.minUniformBufferOffsetAlignment;
  // requiredLimits.limits.maxInterStageShaderComponents = 3;

  std::cout << "Requesting device..." << std::endl;
  DeviceDescriptor deviceDesc;
  // deviceDesc.label = "My Device";
  // deviceDesc.requiredFeaturesCount = 0;
  // // deviceDesc.requiredLimits = &requiredLimits;
  // deviceDesc.requiredLimits = nullptr;
  // deviceDesc.defaultQueue.label = "The default queue";
  Device device = adapter.requestDevice(deviceDesc);
  std::cout << "Got device: " << device << std::endl;

  // Add an error callback for more debug info
  auto h = device.setUncapturedErrorCallback([](ErrorType type, char const *message) {
    std::cout << "Device error: type " << type;
    if (message)
      std::cout << " (message: " << message << ")";
    std::cout << std::endl;
  });

  Queue queue = device.getQueue();

  std::cout << "Creating swapchain device..." << std::endl;
#ifdef WEBGPU_BACKEND_WGPU
  TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
#else
  TextureFormat swapChainFormat = TextureFormat::BGRA8Unorm;
#endif

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  SwapChainDescriptor swapChainDesc;
  swapChainDesc.width = width;
  swapChainDesc.height = height;
  swapChainDesc.usage = TextureUsage::RenderAttachment;
  swapChainDesc.format = swapChainFormat;
  swapChainDesc.presentMode = PresentMode::Fifo;
  SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);
  std::cout << "Swapchain: " << swapChain << std::endl;

  std::cout << "Creating shader module..." << std::endl;
  ShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", device);
  std::cout << "Shader module: " << shaderModule << std::endl;

  // create render pipeline
  std::cout << "Creating render pipeline..." << std::endl;
  RenderPipelineDescriptor pipelineDesc;

  // We now have 2 attributes
  std::vector<VertexAttribute> vertexAttribs(2);

  // Position attribute
  vertexAttribs[0].shaderLocation = 0;
  vertexAttribs[0].format = VertexFormat::Float32x2;
  vertexAttribs[0].offset = 0;

  // Color attribute
  vertexAttribs[1].shaderLocation = 1;
  vertexAttribs[1].format = VertexFormat::Float32x3; // different type!
  vertexAttribs[1].offset = 2 * sizeof(float);       // non null offset!

  VertexBufferLayout vertexBufferLayout;
  vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
  vertexBufferLayout.attributes = vertexAttribs.data();
  vertexBufferLayout.arrayStride = 5 * sizeof(float);
  vertexBufferLayout.stepMode = VertexStepMode::Vertex;

  pipelineDesc.vertex.bufferCount = 1;
  pipelineDesc.vertex.buffers = &vertexBufferLayout;

  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = "vs_main";
  pipelineDesc.vertex.constantCount = 0;
  pipelineDesc.vertex.constants = nullptr;

  pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
  pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
  pipelineDesc.primitive.frontFace = FrontFace::CCW;
  pipelineDesc.primitive.cullMode = CullMode::None;

  // Fragment shader
  FragmentState fragmentState;
  pipelineDesc.fragment = &fragmentState;
  fragmentState.module = shaderModule;
  fragmentState.entryPoint = "fs_main";
  fragmentState.constantCount = 0;
  fragmentState.constants = nullptr;

  // Configure blend state
  BlendState blendState = Default;
  blendState.color.srcFactor = BlendFactor::SrcAlpha;
  blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
  blendState.color.operation = BlendOperation::Add;
  blendState.alpha.srcFactor = BlendFactor::Zero;
  blendState.alpha.dstFactor = BlendFactor::One;
  blendState.alpha.operation = BlendOperation::Add;

  ColorTargetState colorTarget;
  colorTarget.format = swapChainFormat;
  colorTarget.blend = &blendState;
  colorTarget.writeMask = ColorWriteMask::All;

  // We have only one target because our render pass has only one output color
  // attachment.
  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTarget;

  pipelineDesc.depthStencil = nullptr;

  pipelineDesc.multisample.count = 1;
  pipelineDesc.multisample.mask = ~0u;
  pipelineDesc.multisample.alphaToCoverageEnabled = false;

  // Create binding layout (don't forget to = Default)
  // BindGroupLayoutEntry bindingLayout = Default;
  BindGroupLayoutEntry bindingLayout;
  // The binding index as used in the @binding attribute in the shader
  bindingLayout.binding = 0;
  // The stage that needs to access this resource
  bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
  bindingLayout.buffer.type = BufferBindingType::Uniform;
  bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

  // Create a bind group layout
  BindGroupLayoutDescriptor bindGroupLayoutDesc;
  bindGroupLayoutDesc.entryCount = 1;
  bindGroupLayoutDesc.entries = &bindingLayout;
  BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

  // Create the pipeline layout
  PipelineLayoutDescriptor layoutDesc;
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&bindGroupLayout;
  PipelineLayout layout = device.createPipelineLayout(layoutDesc);
  pipelineDesc.layout = layout;

  RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);
  std::cout << "Render pipeline: " << pipeline << std::endl;

  // data
  std::vector<float> pointData;
  std::vector<uint16_t> indexData;

  bool success = loadGeometry(RESOURCE_DIR "/webgpu.txt", pointData, indexData);
  if (!success) {
    std::cerr << "Could not load geometry!" << std::endl;
    return 1;
  }

  // Create vertex buffer
  BufferDescriptor bufferDesc;
  bufferDesc.size = pointData.size() * sizeof(float);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  bufferDesc.mappedAtCreation = false;
  Buffer vertexBuffer = device.createBuffer(bufferDesc);
  queue.writeBuffer(vertexBuffer, 0, pointData.data(), bufferDesc.size);

  int indexCount = static_cast<int>(indexData.size());

  // Create index buffer
  // (we reuse the bufferDesc initialized for the vertexBuffer)
  bufferDesc.size = indexData.size() * sizeof(float);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
  bufferDesc.mappedAtCreation = false;
  Buffer indexBuffer = device.createBuffer(bufferDesc);
  queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

  // Create uniform buffer
  // The buffer will only contain 1 float with the value of uTime
  bufferDesc.size = sizeof(MyUniforms);
  // Make sure to flag the buffer as BufferUsage::Uniform
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  bufferDesc.mappedAtCreation = false;
  Buffer uniformBuffer = device.createBuffer(bufferDesc);

  // Upload the initial value of the uniforms
  MyUniforms uniforms;
  uniforms.time = 1.0f;
  uniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};
  queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

  // Create a binding
  BindGroupEntry binding{};
  // The index of the binding (the entries in bindGroupDesc can be in any order)
  binding.binding = 0;
  // The buffer it is actually bound to
  binding.buffer = uniformBuffer;
  // We can specify an offset within the buffer, so that a single buffer can hold
  // multiple uniform blocks.
  binding.offset = 0;
  // And we specify again the size of the buffer.
  binding.size = sizeof(MyUniforms);

  // A bind group contains one or multiple bindings
  BindGroupDescriptor bindGroupDesc;
  bindGroupDesc.layout = bindGroupLayout;
  // There must be as many bindings as declared in the layout!
  bindGroupDesc.entryCount = bindGroupLayoutDesc.entryCount;
  bindGroupDesc.entries = &binding;
  BindGroup bindGroup = device.createBindGroup(bindGroupDesc);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Update uniform buffer
    uniforms.time = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
    // Only update the 1-st float of the buffer
    queue.writeBuffer(
        uniformBuffer, offsetof(MyUniforms, time), &uniforms.time,
        sizeof(MyUniforms::time)
    );

    TextureView nextTexture = swapChain.getCurrentTextureView();
    if (!nextTexture) {
      std::cerr << "Cannot acquire next swap chain texture" << std::endl;
      return 1;
    }

    CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Command Encoder";
    CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

    RenderPassDescriptor renderPassDesc;

    RenderPassColorAttachment renderPassColorAttachment;
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = LoadOp::Clear;
    renderPassColorAttachment.storeOp = StoreOp::Store;
    renderPassColorAttachment.clearValue = Color{0.05, 0.05, 0.05, 1.0};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;
    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

    renderPass.setPipeline(pipeline);

    // Set both vertex and index buffers
    renderPass.setVertexBuffer(0, vertexBuffer, 0, pointData.size() * sizeof(float));
    // The second argument must correspond to the choice of uint16_t or uint32_t
    // we've done when creating the index buffer.
    renderPass.setIndexBuffer(
        indexBuffer, IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t)
    );
    // Set binding group
    renderPass.setBindGroup(0, bindGroup, 0, nullptr);

    renderPass.drawIndexed(indexCount, 1, 0, 0, 0);

    renderPass.end();

    CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label = "Command buffer";
    CommandBuffer command = encoder.finish(cmdBufferDescriptor);
    queue.submit(command);

    swapChain.present();

    // release resources
    wgpuTextureViewRelease(nextTexture);
  }

  // release resources
  wgpuSwapChainRelease(swapChain);
  wgpuDeviceRelease(device);
  wgpuAdapterRelease(adapter);
  wgpuInstanceRelease(instance);
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
  shaderCodeDesc.chain.next = nullptr;
  shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
  shaderCodeDesc.source = shaderSource.c_str();
  ShaderModuleDescriptor shaderDesc{};
  shaderDesc.nextInChain = &shaderCodeDesc.chain;
  return device.createShaderModule(shaderDesc);
}

bool loadGeometry(
    const fs::path &path, std::vector<float> &pointData,
    std::vector<uint16_t> &indexData
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
