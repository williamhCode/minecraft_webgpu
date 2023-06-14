#pragma once

#include <webgpu/webgpu_cpp.h>

namespace wgpu_utils {

using namespace wgpu;

Adapter RequestAdapter(Instance instance, RequestAdapterOptions const *options);

Device RequestDevice(Adapter instance, DeviceDescriptor const *options);

void SetUncapturedErrorCallback(Device device);

} // namespace wgpu_utils
