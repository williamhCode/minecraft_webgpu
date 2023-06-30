#pragma once

#include "glm-include.hpp"
#include "webgpu/webgpu_cpp.h"
#include "util/handle.hpp"

namespace util {

struct Camera {
private:
  Handle *m_handle;
  glm::vec4 m_forward = glm::vec4(0.0, 1.0, 0.0, 1.0);
  glm::vec4 m_up = glm::vec4(0.0, 0.0, 1.0, 1.0);

public:
  wgpu::Buffer uniformBuffer;
  glm::vec3 position;
  glm::vec3 orientation; // yaw, pitch, roll
  glm::mat4 projection;
  glm::mat4 viewProj;
  glm::mat4 view;

  Camera() = default;
  Camera(
    Handle *handle,
    glm::vec3 position,
    glm::vec3 orientation,
    float fov,
    float aspect,
    float near,
    float far
  );
  void Update();
};

} // namespace util
