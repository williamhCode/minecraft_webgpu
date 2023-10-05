#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "gfx/pipeline.hpp"
#include <webgpu/webgpu_cpp.h>

#include <GLFW/glfw3.h>

namespace util {

struct Pipeline;

struct Context {
  wgpu::Instance instance;
  wgpu::Surface surface;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;
  wgpu::SwapChain swapChain;

  wgpu::TextureFormat swapChainFormat;
  wgpu::TextureFormat depthFormat;

  gfx::Pipeline pipeline;

  Context() = default;
  Context(GLFWwindow *window, glm::uvec2 size);
};

} // namespace util
