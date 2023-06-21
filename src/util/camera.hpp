#pragma once

#include "glm-include.hpp"

namespace util {

class Camera {
private:
  glm::vec3 m_position;
  glm::vec3 m_orientation; // yaw, pitch, roll
  glm::mat4 m_projection;
  glm::mat4 m_viewProj;
  glm::mat4 m_view;
  glm::vec4 m_forward = glm::vec4(0.0, 1.0, 0.0, 1.0);
  glm::vec4 m_up = glm::vec4(0.0, 0.0, 1.0, 1.0);

public:
  Camera() = default;
  Camera(glm::vec3 position, glm::vec3 orientation, float fov, float aspect, float near, float far);
  void Rotate(glm::vec3 delta);
  void Translate(glm::vec3 delta);
  void Update();
  glm::vec3 GetPosition();
  glm::vec3 GetOrientation();
  glm::mat4 GetProjection();
  glm::mat4 GetView();
  glm::mat4 GetViewProj();
  void Look(glm::vec2 delta);
  void Move(glm::vec3 move_direction);
};

} // namespace util
