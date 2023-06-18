#pragma once

#include "glm-include.hpp"

namespace util {

class Camera {
private:
  glm::vec3 m_position;
  glm::vec3 m_orientation; // yaw, pitch, roll
  glm::mat4 m_projection;
  glm::mat4 m_view;
  glm::vec4 m_forward = glm::vec4(0.0, 1.0, 0.0, 1.0);
  glm::vec4 m_up = glm::vec4(0.0, 0.0, 1.0, 1.0);

public:
  Camera() = default;
  Camera(glm::vec3 position, glm::vec3 orientation, float fov, float aspect, float near, float far);
  void rotate(glm::vec3 delta);
  void translate(glm::vec3 delta);
  void update();
  glm::mat4 getProjection();
  glm::mat4 getView();
  void look(glm::vec2 delta);
  void move(glm::vec3 move_direction);
};

} // namespace util
