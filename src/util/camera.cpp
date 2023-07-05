#include "camera.hpp"
#include <algorithm>
#include <vector>

namespace util {

using namespace wgpu;

Camera::Camera(
  Context *ctx,
  glm::vec3 position,
  glm::vec3 orientation,
  float fov,
  float aspect,
  float near,
  float far
)
    : m_ctx(ctx), position(position), orientation(orientation) {
  // create bind group
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
    .size = sizeof(glm::mat4),
  };
  m_viewBuffer = ctx->device.CreateBuffer(&bufferDesc);
  m_projectionBuffer = ctx->device.CreateBuffer(&bufferDesc);

  {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .buffer = m_viewBuffer,
        .size = sizeof(glm::mat4),
      },
      BindGroupEntry{
        .binding = 1,
        .buffer = m_projectionBuffer,
        .size = sizeof(glm::mat4),
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = m_ctx->pipeline.bgl_viewProj,
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bindGroup = m_ctx->device.CreateBindGroup(&bindGroupDesc);
  }

  m_projection = glm::perspective(fov, aspect, near, far);
  m_ctx->queue.WriteBuffer(m_projectionBuffer, 0, &m_projection, sizeof(m_projection));

  Update();
}

void Camera::Update() {
  glm::mat4 pitch = glm::rotate(orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::mat4 roll = glm::rotate(orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 yaw = glm::rotate(orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 rotation = yaw * pitch * roll;

  direction = rotation * m_forward;
  glm::vec3 up = rotation * m_up;
  glm::mat4 view = glm::lookAt(position, position + direction, up);

  m_ctx->queue.WriteBuffer(m_viewBuffer, 0, &view, sizeof(view));
}

} // namespace util
