#include "camera.hpp"
#include <algorithm>

namespace util {

Camera::Camera(
  glm::vec3 position, glm::vec3 orientation, float fov, float aspect, float near,
  float far
)
    : m_position(position), m_orientation(orientation) {
  m_projection = glm::perspective(fov, aspect, near, far);
  Update();
}

void Camera::Rotate(glm::vec3 delta) { m_orientation += delta; }

void Camera::Translate(glm::vec3 delta) { m_position += delta; }

void Camera::Update() {
  glm::mat4 pitch = glm::rotate(m_orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::mat4 roll = glm::rotate(m_orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 yaw = glm::rotate(m_orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 rotation = yaw * pitch * roll;

  glm::vec3 forward = rotation * m_forward;
  glm::vec3 up = rotation * m_up;

  m_view = glm::lookAt(m_position, m_position + forward, up);
  m_viewProj = m_projection * m_view;
}

glm::vec3 Camera::GetPosition() { return m_position; }

glm::vec3 Camera::GetOrientation() { return m_orientation; }

glm::mat4 Camera::GetProjection() { return m_projection; }

glm::mat4 Camera::GetView() { return m_view; }

glm::mat4 Camera::GetViewProj() { return m_viewProj; }

// left-right, vertical
void Camera::Look(glm::vec2 delta) {
  Rotate(glm::vec3(-delta.y, 0, -delta.x));
  float pi_4 = glm::radians(90.0f);
  m_orientation.x = std::max(-pi_4, std::min(m_orientation.x, pi_4));
}

// sideways, forwards, vertical
void Camera::Move(glm::vec3 move_direction) {
  move_direction = glm::rotateZ(move_direction, m_orientation.z);
  Translate(move_direction);
}

} // namespace util
