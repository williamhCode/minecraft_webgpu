#pragma once

#include "glm-include.hpp"
#include "webgpu/webgpu_cpp.h"
#include "util/handle.hpp"

namespace util {

class Camera {
private:
  Handle *m_handle;
  wgpu::Buffer m_uniformBuffer;

  glm::vec3 m_position;
  glm::vec3 m_orientation; // yaw, pitch, roll
  glm::mat4 m_projection;
  glm::mat4 m_viewProj;
  glm::mat4 m_view;
  glm::vec4 m_forward = glm::vec4(0.0, 1.0, 0.0, 1.0);
  glm::vec4 m_up = glm::vec4(0.0, 0.0, 1.0, 1.0);

public:
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
  void Rotate(glm::vec3 delta);
  void Translate(glm::vec3 delta);
  void Update();
  glm::vec3 GetPosition();
  glm::vec3 GetOrientation();
  glm::mat4 GetProjection();
  glm::mat4 GetView();
  glm::mat4 GetViewProj();
  wgpu::Buffer GetBuffer();
  void Look(glm::vec2 delta);
  void Move(glm::vec3 move_direction);
};

} // namespace util
