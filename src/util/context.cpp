#include "context.hpp"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "util/webgpu-util.hpp"
#include <dawn/utils/TextureUtils.h>

#include <iostream>
#include <sstream>

namespace util {

using namespace wgpu;

Context::Context(GLFWwindow *window, glm::uvec2 size) {
  // instance
  instance = CreateInstance();
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    std::exit(1);
  }

  // surface
  surface = glfw::CreateSurfaceForWindow(instance, window);

  // adapter
  RequestAdapterOptions adapterOpts{
    .powerPreference = PowerPreference::HighPerformance};
  adapter = util::RequestAdapter(instance, &adapterOpts);

  // device limits
  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  deviceLimits = supportedLimits.limits;

  // device
  DeviceDescriptor deviceDesc{
    .label = "My Device",
    .requiredFeaturesCount = 0,
    .requiredLimits = nullptr,
    .defaultQueue{.label = "The default queue"},
  };
  device = util::RequestDevice(adapter, &deviceDesc);
  util::SetUncapturedErrorCallback(device);

  // queue
  queue = device.GetQueue();

  // swap chain format
  swapChainFormat = TextureFormat::BGRA8Unorm;
  depthFormat = TextureFormat::Depth24Plus;

  // swap chain
  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = size.x,
    .height = size.y,
    .presentMode = PresentMode::Immediate,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);

  pipeline = Pipeline(*this);
}

wgpu::Buffer
Context::CreateBuffer(wgpu::BufferUsage usage, size_t size, const void *data) {
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | usage,
    .size = size,
  };
  Buffer buffer = device.CreateBuffer(&bufferDesc);
  if (data) queue.WriteBuffer(buffer, 0, data, size);
  return buffer;
}

wgpu::Buffer Context::CreateVertexBuffer(size_t size, const void *data) {
  return CreateBuffer(BufferUsage::Vertex, size, data);
}

wgpu::Buffer Context::CreateIndexBuffer(size_t size, const void *data) {
  return CreateBuffer(BufferUsage::Index, size, data);
}

wgpu::Buffer Context::CreateUniformBuffer(size_t size, const void *data) {
  return CreateBuffer(BufferUsage::Uniform, size, data);
}

wgpu::Texture Context::CreateTexture(
  wgpu::Extent3D size, wgpu::TextureFormat format, const void *data
) {
  TextureDescriptor textureDesc{
    .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
    .size = size,
    .format = format,
  };
  Texture texture = device.CreateTexture(&textureDesc);
  if (data) {
    auto texelBlockSize = dawn::utils::GetTexelBlockSizeInBytes(format);
    ImageCopyTexture destination{
      .texture = texture,
    };
    TextureDataLayout dataLayout{
      .bytesPerRow = size.width * texelBlockSize,
      .rowsPerImage = size.height,
    };
    queue.WriteTexture(
      &destination, data, size.width * size.height * texelBlockSize, &dataLayout, &size
    );
  }
  return texture;
}

wgpu::Texture
Context::CreateRenderTexture(wgpu::Extent3D size, wgpu::TextureFormat format) {
  TextureDescriptor textureDesc{
    .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
    .size = size,
    .format = format,
  };
  return device.CreateTexture(&textureDesc);
}

wgpu::Texture Context::CreateDepthTexture(wgpu::Extent3D size) {
  TextureDescriptor textureDesc{
    .usage = TextureUsage::RenderAttachment,
    .size = size,
    .format = depthFormat,
  };
  return device.CreateTexture(&textureDesc);
}

} // namespace util
