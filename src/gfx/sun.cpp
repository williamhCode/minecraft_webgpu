#include "sun.hpp"

#include "dawn/utils/WGPUHelpers.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float2.hpp"

#include "game.hpp"
#include "util/webgpu-util.hpp"

namespace gfx {

Sun::Sun(gfx::Context *ctx, GameState *state, glm::vec3 dir)
    : m_ctx(ctx), m_state(state), m_dir(dir) {
  m_sunDirBuffer = util::CreateUniformBuffer(m_ctx->device, sizeof(glm::vec3), &dir);

  glm::vec2 depth_range(1, 1024);
  auto halfLength = areaLength / 2;
  for (size_t i = 0; i < numCascades; i++) {
    m_projs[i] = glm::ortho(-halfLength, halfLength, -halfLength, halfLength, depth_range.x, depth_range.y);
    halfLength /= 2;
    // depth_range.y /= 2;
  }

  m_sunViewProjsBuffer =
    util::CreateStorageBuffer(m_ctx->device, sizeof(glm::mat4) * numCascades);

  bindGroup = dawn::utils::MakeBindGroup(
    ctx->device, ctx->pipeline.sunBGL,
    {
      {0, m_sunDirBuffer},
      {1, m_sunViewProjsBuffer},
    }
  );

  InvokeUpdate();
}

void Sun::UpdateViews() {
  const float distance = 512;

  auto centerPos = m_state->player.GetPosition();
  centerPos.z = 100;
  auto eyePos = centerPos + m_dir * distance;
  auto view = glm::lookAt(eyePos, centerPos, m_state->player.camera.up);

  for (size_t i = 0; i < numCascades; i++) {
    m_viewProjs[i] = m_projs[i] * view;
    auto centerView = m_viewProjs[i] * glm::vec4(centerPos, 1.0);
    auto centerFixed = glm::vec3(centerView.xy() - 0.5f, centerView.z);
    auto centerFixedW = (glm::inverse(m_viewProjs[i]) * glm::vec4(centerFixed, 1.0)).xyz();

    eyePos = centerFixedW + m_dir * distance;
    m_views[i] = glm::lookAt(eyePos, centerFixedW, m_state->player.camera.up);
  }
  // center the shadow map
}

void Sun::InvokeUpdate() {
  shouldUpdate = true;
}

bool Sun::ShouldRender() {
  if (shouldRender) {
    shouldRender = false;
    return true;
  }
  return false;
}

void Sun::Update() {
  timeSinceUpdate += m_state->dt;

  if (timeSinceUpdate > minTime && shouldUpdate) {
    UpdateViews();
    for (size_t i = 0; i < numCascades; i++) {
      m_viewProjs[i] = m_projs[i] * m_views[i];
      auto stride = sizeof(glm::mat4);
      m_ctx->queue.WriteBuffer(m_sunViewProjsBuffer, stride * i, &m_viewProjs[i], stride);
    }

    timeSinceUpdate = 0;
    shouldUpdate = false;

    shouldRender = true;
  }
}

void Sun::SetDir(glm::vec3 dir) {
  m_dir = dir;
  m_ctx->queue.WriteBuffer(m_sunDirBuffer, 0, &m_dir, sizeof(m_dir));
}

util::Frustum Sun::GetFrustum(int cascadeLevel) {
  return util::Frustum(m_viewProjs[cascadeLevel]);
}

} // namespace gfx

