#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "util/pipeline.hpp"
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
  wgpu::Limits deviceLimits;

  wgpu::TextureFormat swapChainFormat;
  wgpu::TextureFormat depthFormat;

  util::Pipeline pipeline;

  Context() = default;
  Context(GLFWwindow *window, glm::uvec2 size);

  wgpu::Buffer
  CreateBuffer(wgpu::BufferUsage usage, size_t size, const void *data = nullptr);
  wgpu::Buffer CreateVertexBuffer(size_t size, const void *data = nullptr);
  wgpu::Buffer CreateIndexBuffer(size_t size, const void *data = nullptr);
  wgpu::Buffer CreateUniformBuffer(size_t size, const void *data = nullptr);
};

} // namespace util
