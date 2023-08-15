#pragma once

#include "util/context.hpp"
#include <webgpu/webgpu_cpp.h>
#include <fstream>

namespace util {

namespace fs = std::filesystem;

wgpu::Adapter RequestAdapter(wgpu::Instance &instance, wgpu::RequestAdapterOptions const *options);

wgpu::Device RequestDevice(wgpu::Adapter &instance, wgpu::DeviceDescriptor const *options);

void SetUncapturedErrorCallback(wgpu::Device &device);

wgpu::ShaderModule LoadShaderModule(const fs::path &path, wgpu::Device &device);

wgpu::Buffer CreateVertexBuffer(Context *ctx, size_t size, const void *data=nullptr);

wgpu::Buffer CreateIndexBuffer(Context *ctx, size_t size, const void *data=nullptr);

wgpu::Buffer CreateUniformBuffer(Context *ctx, size_t size, const void *data=nullptr);

} // namespace util
