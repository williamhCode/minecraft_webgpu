#pragma once

#include "glm-include.hpp"
#include "webgpu/webgpu_cpp.h"
#include "context.hpp"
#include "frustum.hpp"

namespace util {

class Camera {
private:
  Context *m_ctx;
  constexpr static glm::vec4 m_forward = glm::vec4(1.0, 0.0, 0.0, 1.0);
  constexpr static glm::vec4 m_up = glm::vec4(0.0, 0.0, 1.0, 1.0);
  glm::mat4 m_view;
  glm::mat4 m_projection;
  wgpu::Buffer m_viewBuffer;
  wgpu::Buffer m_projectionBuffer;
  wgpu::Buffer m_inverseViewBuffer;
  wgpu::Buffer m_viewPosBuffer;

public:
  wgpu::BindGroup bindGroup;
  glm::vec3 position;
  glm::vec3 direction;
  glm::vec3 orientation; // pitch, roll, yaw
  float fov;

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
  Frustum GetFrustum() {
    return Frustum(m_projection * m_view);
  }
};

} // namespace util
