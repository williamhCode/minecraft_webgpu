#include "camera.hpp"
#include "glm/gtx/string_cast.hpp"
#include "util/webgpu-util.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include <algorithm>
#include <iostream>
#include <vector>

namespace util {

using namespace wgpu;

Camera::Camera(
  gfx::Context *ctx,
  glm::vec3 position,
  glm::vec3 orientation,
  float fov,
  float aspect,
  float near,
  float far
)
    : m_ctx(ctx), position(position), orientation(orientation), fov(fov) {
  m_projection = glm::perspective(fov, aspect, near, far);

  // create bind group
  size_t size = sizeof(glm::mat4);
  m_viewBuffer = util::CreateUniformBuffer(m_ctx->device, size);
  m_projectionBuffer = util::CreateUniformBuffer(m_ctx->device, size, &m_projection);
  m_inverseViewBuffer = util::CreateUniformBuffer(m_ctx->device, size);

  bindGroup = dawn::utils::MakeBindGroup(
    ctx->device, ctx->pipeline.cameraBGL,
    {
      {0, m_viewBuffer},
      {1, m_projectionBuffer},
      {2, m_inverseViewBuffer},
    }
  );

  Update();
}

void Camera::Update() {
  glm::mat4 pitch = glm::rotate(orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::mat4 roll = glm::rotate(orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 yaw = glm::rotate(orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 rotation = yaw * pitch * roll;

  direction = rotation * m_forward;
  m_view = glm::lookAt(position, position + direction, m_up);
  glm::mat4 inverseView = glm::inverse(m_view);

  m_ctx->queue.WriteBuffer(m_viewBuffer, 0, &m_view, sizeof(m_view));
  m_ctx->queue.WriteBuffer(m_inverseViewBuffer, 0, &inverseView, sizeof(inverseView));
}

} // namespace util
