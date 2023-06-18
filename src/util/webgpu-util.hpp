#pragma once

#include <webgpu/webgpu_cpp.h>
#include <fstream>

namespace util {

namespace fs = std::filesystem;

wgpu::Adapter requestAdapter(wgpu::Instance instance, wgpu::RequestAdapterOptions const *options);

wgpu::Device requestDevice(wgpu::Adapter instance, wgpu::DeviceDescriptor const *options);

void setUncapturedErrorCallback(wgpu::Device device);

wgpu::ShaderModule loadShaderModule(const fs::path &path, wgpu::Device device);

} // namespace util
