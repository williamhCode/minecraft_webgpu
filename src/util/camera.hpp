#pragma once

#include "glm-include.hpp"
#include "webgpu/webgpu_cpp.h"
#include "util/context.hpp"

namespace util {

class Camera {
private:
  Context *m_ctx;
  glm::vec4 m_forward = glm::vec4(0.0, 1.0, 0.0, 1.0);
  glm::vec4 m_up = glm::vec4(0.0, 0.0, 1.0, 1.0);
  glm::mat4 m_projection;
  wgpu::Buffer m_viewBuffer;
  wgpu::Buffer m_projectionBuffer;

public:
  wgpu::BindGroup bindGroup;
  glm::vec3 position;
  glm::vec3 direction;
  glm::vec3 orientation; // yaw, pitch, roll

  Camera() = default;
  Camera(
    Context *ctx,
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
