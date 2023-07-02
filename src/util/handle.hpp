#pragma once

#include "util/pipeline.hpp"
#include <webgpu/webgpu_cpp.h>

#include <GLFW/glfw3.h>

namespace util {

struct Pipeline;

struct Handle {
  wgpu::Instance instance;
  wgpu::Surface surface;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;
  wgpu::SwapChain swapChain;
  wgpu::TextureFormat swapChainFormat;
  wgpu::Limits deviceLimits;

  util::Pipeline pipeline;

  Handle() = default;
  Handle(GLFWwindow *window);
};

} // namespace util
