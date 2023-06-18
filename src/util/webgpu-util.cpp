#include "webgpu-util.h"
#include <iostream>
#include <map>
#include <ostream>

namespace util {

using namespace wgpu;

Adapter requestAdapter(Instance instance, RequestAdapterOptions const *options) {
  struct UserData {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  auto onAdapterRequestEnded = [](
                                 WGPURequestAdapterStatus status,
                                 WGPUAdapter adapter,
                                 char const *message,
                                 void *pUserData
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

Device requestDevice(Adapter instance, DeviceDescriptor const *descriptor) {
  struct UserData {
    WGPUDevice device = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  auto onDeviceRequestEnded = [](
                                WGPURequestDeviceStatus status,
                                WGPUDevice device,
                                char const *message,
                                void *pUserData
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

void setUncapturedErrorCallback(Device device) {
  auto onUncapturedError = [](WGPUErrorType type, char const *message, void *userdata) {
    std::cout << "Device error: type " << type;
    if (message)
      std::cout << " (message: " << message << ")";
    std::cout << std::endl;
  };

  device.SetUncapturedErrorCallback(onUncapturedError, nullptr);
}

ShaderModule loadShaderModule(const fs::path &path, Device device) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return nullptr;
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  std::string shaderSource(size, ' ');
  file.seekg(0);
  file.read(shaderSource.data(), size);

  ShaderModuleWGSLDescriptor shaderCodeDesc{};
  shaderCodeDesc.nextInChain = nullptr;
  shaderCodeDesc.code = shaderSource.c_str();
  ShaderModuleDescriptor shaderDesc{.nextInChain = &shaderCodeDesc};
  return device.CreateShaderModule(&shaderDesc);
}

} // namespace util
