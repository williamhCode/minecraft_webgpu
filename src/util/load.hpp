#pragma once

#include <webgpu/webgpu_cpp.h>
#include "handle.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

#include <fstream>
#include <vector>
#include <sstream>

namespace util {

struct ModelVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 uv;
};

std::vector<ModelVertex> LoadObj(const std::string &path);

} // namespace util
