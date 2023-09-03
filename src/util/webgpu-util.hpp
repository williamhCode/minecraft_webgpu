#pragma once

#include "util/context.hpp"
#include <vector>
#include <webgpu/webgpu_cpp.h>
#include <fstream>

namespace util {

namespace fs = std::filesystem;

wgpu::Adapter RequestAdapter(wgpu::Instance &instance, wgpu::RequestAdapterOptions const *options);

wgpu::Device RequestDevice(wgpu::Adapter &instance, wgpu::DeviceDescriptor const *options);

void SetUncapturedErrorCallback(wgpu::Device &device);

wgpu::ShaderModule LoadShaderModule(const fs::path &path, wgpu::Device &device);

wgpu::Buffer CreateBuffer(wgpu::Device &device, wgpu::BufferUsage usage, size_t size, const void *data = nullptr);
wgpu::Buffer CreateVertexBuffer(wgpu::Device &device, size_t size, const void *data = nullptr);
wgpu::Buffer CreateIndexBuffer(wgpu::Device &device, size_t size, const void *data = nullptr);
wgpu::Buffer CreateUniformBuffer(wgpu::Device &device, size_t size, const void *data = nullptr);

wgpu::Texture CreateTexture(wgpu::Device &device, wgpu::Extent3D size, wgpu::TextureFormat format, const void *data = nullptr);
wgpu::Texture CreateRenderTexture(wgpu::Device &device, wgpu::Extent3D size, wgpu::TextureFormat format);

} // namespace util

template <typename T>
const T *ToPtr(T &&value) {
  return &value;
}
