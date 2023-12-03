#pragma once

#include "gfx/context.hpp"
#include <functional>
#include <optional>
#include <vector>
#include <array>
#include <webgpu/webgpu_cpp.h>
#include <fstream>

namespace util {

namespace fs = std::filesystem;

// clang-format off
wgpu::Adapter RequestAdapter(wgpu::Instance &instance, wgpu::RequestAdapterOptions const *options);

wgpu::Device RequestDevice(wgpu::Adapter &instance, wgpu::DeviceDescriptor const *options);

void SetUncapturedErrorCallback(wgpu::Device &device);

wgpu::ShaderModule LoadShaderModule(const fs::path &path, wgpu::Device &device);

void PrintLimits(wgpu::Limits const &limits);

wgpu::Buffer CreateBuffer(wgpu::Device &device, wgpu::BufferUsage usage, size_t size, const void *data = nullptr);
wgpu::Buffer CreateVertexBuffer(wgpu::Device &device, size_t size, const void *data = nullptr);
wgpu::Buffer CreateIndexBuffer(wgpu::Device &device, size_t size, const void *data = nullptr);
wgpu::Buffer CreateUniformBuffer(wgpu::Device &device, size_t size, const void *data = nullptr);

wgpu::Texture CreateTexture(wgpu::Device &device, wgpu::Extent3D size, wgpu::TextureFormat format, const void *data = nullptr);
wgpu::Texture CreateRenderTexture(wgpu::Device &device, wgpu::Extent3D size, wgpu::TextureFormat format);

struct RenderPassDescriptor : public wgpu::RenderPassDescriptor{
  RenderPassDescriptor(
    std::vector<wgpu::RenderPassColorAttachment> colorAttachments = {},
    wgpu::RenderPassDepthStencilAttachment depthStencilAttachment = wgpu::RenderPassDepthStencilAttachment{}
  );
  ~RenderPassDescriptor() = default;

  RenderPassDescriptor(const RenderPassDescriptor& otherRenderPass);
  const RenderPassDescriptor& operator=(const RenderPassDescriptor& otherRenderPass);

  std::vector<wgpu::RenderPassColorAttachment> cColorAttachments;
  wgpu::RenderPassDepthStencilAttachment cDepthStencilAttachmentInfo = {};
};
// clang-format on

namespace BlendState {
const wgpu::BlendState AlphaBlending = {
  .color{
    .operation = wgpu::BlendOperation::Add,
    .srcFactor = wgpu::BlendFactor::SrcAlpha,
    .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
  },
  .alpha{
    .operation = wgpu::BlendOperation::Add,
    .srcFactor = wgpu::BlendFactor::One,
    .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
  },
};
}

} // namespace util

template <typename T>
const T *ToPtr(const T &&value) {
  return &value;
}

template <typename T, size_t N>
constexpr const T *ToPtr(const T (&&a)[N]) {
  return a;
}
