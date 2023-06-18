#include "handle.h"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "util/webgpu-util.h"

#include <iostream>
#include <sstream>

namespace util {

using namespace wgpu;

Handle Handle::init(GLFWwindow *window) {
  // instance
  Instance instance = CreateInstance();
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    std::exit(1);
  }

  // surface
  Surface surface = glfw::CreateSurfaceForWindow(instance, window);

  // adapter
  RequestAdapterOptions adapterOpts{};
  Adapter adapter = util::requestAdapter(instance, &adapterOpts);

  // device limits
  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  Limits deviceLimits = supportedLimits.limits;

  // device
  DeviceDescriptor deviceDesc{
    .label = "My Device",
    .requiredFeaturesCount = 0,
    .requiredLimits = nullptr,
    .defaultQueue{.label = "The default queue"},
  };
  Device device = util::requestDevice(adapter, &deviceDesc);
  util::setUncapturedErrorCallback(device);

  // queue
  Queue queue = device.GetQueue();

  // swap chain format
  TextureFormat swapChainFormat = TextureFormat::BGRA8Unorm;

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
  SwapChain swapChain = device.CreateSwapChain(surface, &swapChainDesc);

  // init
  return Handle{
    instance,
    surface,
    adapter,
    device,
    queue,
    swapChain,
    swapChainFormat,
    deviceLimits,
  };
}

} // namespace util
