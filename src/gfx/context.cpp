#include "context.hpp"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "util/webgpu-util.hpp"
#include <dawn/utils/TextureUtils.h>

#include <iostream>

namespace gfx {

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
    .compatibleSurface = surface,
    .powerPreference = PowerPreference::HighPerformance,
  };
  adapter = util::RequestAdapter(instance, &adapterOpts);

  // device limits
  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  // util::PrintLimits(supportedLimits.limits);

  RequiredLimits requiredLimits{
    .limits = supportedLimits.limits,
  };

  // device
  DeviceDescriptor deviceDesc{
    .requiredLimits = &requiredLimits,
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
    .presentMode = PresentMode::Fifo,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);

  pipeline = gfx::Pipeline(*this);
}

} // namespace gfx
