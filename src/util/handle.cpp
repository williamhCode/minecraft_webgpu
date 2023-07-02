#include "handle.hpp"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "util/webgpu-util.hpp"

#include <iostream>
#include <sstream>

namespace util {

using namespace wgpu;

Handle::Handle(GLFWwindow *window) {
  // instance
  instance = CreateInstance();
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    std::exit(1);
  }

  // surface
  surface = glfw::CreateSurfaceForWindow(instance, window);

  // adapter
  RequestAdapterOptions adapterOpts{};
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

  // swap chain
  int FBWidth, FBHeight;
  glfwGetFramebufferSize(window, &FBWidth, &FBHeight);
  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = static_cast<uint32_t>(FBWidth),
    .height = static_cast<uint32_t>(FBHeight),
    .presentMode = PresentMode::Fifo,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);

  pipeline = Pipeline(*this);
}

} // namespace util
