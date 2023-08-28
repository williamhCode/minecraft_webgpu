#include "webgpu-util.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "util/context.hpp"
#include <iostream>
#include <map>
#include <ostream>

namespace util {
// using namespace util;

using namespace wgpu;

Adapter RequestAdapter(Instance &instance, RequestAdapterOptions const *options) {
  struct UserData {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  auto onAdapterRequestEnded = [](
                                 WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                 char const *message, void *pUserData
                               ) {
    UserData &userData = *reinterpret_cast<UserData *>(pUserData);
    if (status == WGPURequestAdapterStatus_Success) {
      userData.adapter = adapter;
    } else {
      std::cout << "Could not get WebGPU adapter: " << message << std::endl;
    }
    userData.requestEnded = true;
  };

  instance.RequestAdapter(options, onAdapterRequestEnded, &userData);

  assert(userData.requestEnded);

  return userData.adapter;
}

Device RequestDevice(Adapter &instance, DeviceDescriptor const *descriptor) {
  struct UserData {
    WGPUDevice device = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  auto onDeviceRequestEnded = [](
                                WGPURequestDeviceStatus status, WGPUDevice device,
                                char const *message, void *pUserData
                              ) {
    UserData &userData = *reinterpret_cast<UserData *>(pUserData);
    if (status == WGPURequestDeviceStatus_Success) {
      userData.device = device;
    } else {
      std::cout << "Could not get WebGPU adapter: " << message << std::endl;
    }
    userData.requestEnded = true;
  };

  instance.RequestDevice(descriptor, onDeviceRequestEnded, &userData);

  assert(userData.requestEnded);

  return userData.device;
}

void SetUncapturedErrorCallback(Device &device) {
  auto onUncapturedError = [](WGPUErrorType type, char const *message, void *userdata) {
    std::cout << "Device error: type " << type;
    if (message) std::cout << " (message: " << message << ")";
    std::cout << std::endl;
  };

  device.SetUncapturedErrorCallback(onUncapturedError, nullptr);
}

ShaderModule LoadShaderModule(const fs::path &path, Device &device) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open shader file" + path.string());
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  std::string shaderSource(size, ' ');
  file.seekg(0);
  file.read(shaderSource.data(), size);

  return dawn::utils::CreateShaderModule(device, shaderSource.c_str());
}

wgpu::Buffer CreateBuffer(
  wgpu::Device &device, wgpu::BufferUsage usage, size_t size, const void *data
) {
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | usage,
    .size = size,
  };
  Buffer buffer = device.CreateBuffer(&bufferDesc);
  if (data) device.GetQueue().WriteBuffer(buffer, 0, data, size);
  return buffer;
}

wgpu::Buffer CreateVertexBuffer(wgpu::Device &device, size_t size, const void *data) {
  return CreateBuffer(device, BufferUsage::Vertex, size, data);
}

wgpu::Buffer CreateIndexBuffer(wgpu::Device &device, size_t size, const void *data) {
  return CreateBuffer(device, BufferUsage::Index, size, data);
}

wgpu::Buffer CreateUniformBuffer(wgpu::Device &device, size_t size, const void *data) {
  return CreateBuffer(device, BufferUsage::Uniform, size, data);
}

wgpu::Texture CreateTexture(
  wgpu::Device &device,
  wgpu::Extent3D size,
  wgpu::TextureFormat format,
  const void *data
) {
  TextureDescriptor textureDesc{
    .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
    .size = size,
    .format = format,
  };
  Texture texture = device.CreateTexture(&textureDesc);
  if (data) {
    auto texelBlockSize = dawn::utils::GetTexelBlockSizeInBytes(format);
    ImageCopyTexture destination{
      .texture = texture,
    };
    TextureDataLayout dataLayout{
      .bytesPerRow = size.width * texelBlockSize,
      .rowsPerImage = size.height,
    };
    device.GetQueue().WriteTexture(
      &destination, data, size.width * size.height * texelBlockSize, &dataLayout, &size
    );
  }
  return texture;
}

wgpu::Texture CreateRenderTexture(
  wgpu::Device &device, wgpu::Extent3D size, wgpu::TextureFormat format
) {
  TextureDescriptor textureDesc{
    .usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding,
    .size = size,
    .format = format,
  };
  return device.CreateTexture(&textureDesc);
}

} // namespace util
