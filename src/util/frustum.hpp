#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float4.hpp"
#include <array>

namespace util {

struct AABB {
  glm::vec3 min; // Minimum point (bottom, left, near)
  glm::vec3 max; // Maximum point (top, right, far)
};

struct Frustum {
  // left, right, bottom, top, near, far
  std::array<glm::vec4, 6> planes;

  Frustum() = default;
  Frustum(glm::mat4 viewProj);
  bool Intersects(const AABB &aabb);
};

} // namespace util
