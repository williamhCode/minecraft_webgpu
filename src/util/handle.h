#pragma once

#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>

namespace util {

struct Handle {
  wgpu::Instance instance;
  wgpu::Surface surface;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;
  wgpu::SwapChain swapChain;
  wgpu::TextureFormat swapChainFormat;
  wgpu::Limits deviceLimits;

  static Handle init(GLFWwindow *window);
};

} // namespace util
