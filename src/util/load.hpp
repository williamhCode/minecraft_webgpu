#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float2.hpp"
#include <webgpu/webgpu_cpp.h>

#include <vector>

namespace util {

struct ModelVertex {
  glm::vec3 position;
  glm::vec3 normal;
  // glm::vec3 color;
  glm::vec2 uv;
};

std::vector<ModelVertex> LoadObj(const std::string &path);

} // namespace util
